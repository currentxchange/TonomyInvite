#include "invitono.hpp"

// === Register User ===
// Registers a user with a referral code and applies multi-level scoring
void invitono::registeruser(name user, name inviter) {
  check(has_auth(user) || has_auth(get_self()), "Only the user can invite");

  check(is_account(inviter), "Inviter account does not exist");
  check(user != inviter, "Cannot invite yourself");

  adopters_table adopters(get_self(), get_self().value);
  auto existing = adopters.find(user.value);
  check(existing == adopters.end(), "User already registered");

  auto inviter_itr = adopters.find(inviter.value);
  check(inviter_itr != adopters.end(), "Inviter must be registered first");

  // CHECK: Configuration check
  config_table conf(get_self(), get_self().value);
  auto cfg = conf.get_or_default();
  check(cfg.enabled, "Registration is currently disabled");

  // CHECK: Verify account age
  time_point_sec now = time_point_sec(current_time_point());
  time_point_sec creation_date = get_account_creation_time(user);
  check((now.sec_since_epoch() - creation_date.sec_since_epoch()) >= cfg.min_account_age_days * 86400,
        "Account must be at least " + std::to_string(cfg.min_account_age_days) + " days old to register");

  adopters.emplace(user, [&](auto& row) {
    row.account = user;
    row.invitedby = inviter;
    row.lastupdated = current_time_point().sec_since_epoch();
    row.score = 1;
    row.claimed = false;
  });

  // Update global stats
  stats_table stats(get_self(), get_self().value);
  auto current = stats.get_or_default();
  current.total_users += 1;
  current.total_referrals += 1;
  current.last_registered = user;
  stats.set(current, get_self());

  update_scores(inviter); // Updates inviter & their uplines
}

// === Update Scores ===
// Applies +1 score to inviter and their upline if cooldown has passed
void invitono::update_scores(name direct_inviter) {
    adopters_table adopters(get_self(), get_self().value);
    config_table conf(get_self(), get_self().value);
    auto cfg = conf.get_or_default();

    std::vector<std::pair<name, uint16_t>> upline;
    uint16_t current_level = 1;

    // Build upline chain up to max_referral_depth
    auto current_itr = adopters.find(direct_inviter.value);
    while (current_itr != adopters.end() && current_level <= cfg.max_referral_depth) {
        upline.push_back({current_itr->account, current_level});
        
        if (current_itr->invitedby == name{}) break;
        current_itr = adopters.find(current_itr->invitedby.value);
        current_level++;
    }

    // Update scores with level multipliers
    for (const auto& [account, level] : upline) {
        auto itr = adopters.find(account.value);
        if (itr != adopters.end()) {
            if ((current_time_point().sec_since_epoch() - itr->lastupdated) >= cfg.invite_rate_seconds) {

                adopters.modify(itr, same_payer, [&](auto& row) {
                    row.score += 1;
                    row.lastupdated = current_time_point().sec_since_epoch();
                });
            }
        }
    }
}

// === Claim Reward ===
// Mints tokens based on invite score (1 TOKEN per point)
void invitono::claimreward(name user) {
  check(has_auth(user) || has_auth(get_self()), "Only the user or contract can claim rewards");

  // Add config check
  config_table conf(get_self(), get_self().value);
  auto cfg = conf.get_or_default();
  check(cfg.enabled, "Contract is currently disabled");

  adopters_table adopters(get_self(), get_self().value);
  auto itr = adopters.find(user.value);
  check(itr != adopters.end(), "User not found");
  check(!itr->claimed, "Already claimed rewards");

  uint32_t score = itr->score;
  check(score > 0, "No rewards to claim");

  // Modify before transfer to prevent reentrancy
  adopters.modify(itr, same_payer, [&](auto& row) {
    row.claimed = true;
  });

  // Calculate position in tetrahedral series
  uint32_t position = calculate_tetrahedral_position(score);
  
  // Convert to asset with overflow check using symbol precision and reward rate
  uint8_t precision = cfg.reward_symbol.precision();
  // reward_rate is in hundredths (e.g., 100 = 1.00 token)
  int64_t amount = (static_cast<int64_t>(position) * static_cast<int64_t>(pow(10, precision)) * cfg.reward_rate) / 100;
  check(position <= UINT32_MAX / static_cast<uint32_t>(pow(10, precision)) * 100 / cfg.reward_rate, "Position overflow");
  asset reward = asset(amount, cfg.reward_symbol);

  // Note: Balance check requires token contract integration
  // Assuming standard eosio.token contract
  action(
    permission_level{get_self(), "active"_n},
    cfg.token_contract,
    "transfer"_n,
    std::make_tuple(get_self(), user, reward, std::string("Invitono tetrahedral position reward"))
  ).send();
}

// === Set Config ===
// Admin sets contract-wide configuration
void invitono::setconfig(
    name admin, 
    uint32_t min_age_days, 
    uint32_t rate_seconds, 
    bool enabled,
    uint16_t max_depth,
    uint16_t multiplier,
    name token_contract,
    symbol reward_symbol,
    uint32_t reward_rate
) {
    config_table conf(get_self(), get_self().value);

    // Validate new parameters
    check(max_depth > 0 && max_depth <= 100, "Invalid depth (1-100)");
    check(multiplier > 0 && multiplier <= 1000, "Invalid multiplier (1-1000)");
    check(is_account(admin), "New admin account does not exist");
    check(is_account(token_contract), "Token contract account does not exist");
    check(reward_symbol.is_valid(), "Invalid reward symbol");
    check(reward_rate > 0, "Reward rate must be positive");

    // Handle first-time initialization
    if (!conf.exists()) {
        require_auth(get_self());
        conf.set(config{
            .min_account_age_days = min_age_days,
            .invite_rate_seconds = rate_seconds,
            .enabled = enabled,
            .admin = admin,
            .max_referral_depth = max_depth,
            .multiplier = multiplier,
            .token_contract = token_contract,
            .reward_symbol = reward_symbol,
            .reward_rate = reward_rate
        }, get_self());
        return;
    }

    // Normal admin updates
    auto current = conf.get();
    require_auth(current.admin);

    // Add validation
    check(min_age_days > 0, "Minimum age must be positive");
    check(rate_seconds > 0, "Rate must be positive");

    conf.set(config{
        .min_account_age_days = min_age_days,
        .invite_rate_seconds = rate_seconds,
        .enabled = enabled,
        .admin = admin,
        .max_referral_depth = max_depth,
        .multiplier = multiplier,
        .token_contract = token_contract,
        .reward_symbol = reward_symbol,
        .reward_rate = reward_rate
    }, get_self());
}

// === Delete User ===
// Dev-only action to remove a user (for testing)
void invitono::deleteuser(name user) {
  require_auth(get_self());

  adopters_table adopters(get_self(), get_self().value);
  auto itr = adopters.find(user.value);
  if (itr != adopters.end()) {
    adopters.erase(itr);
  }
}