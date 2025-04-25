#include "invitono.hpp"

struct [[eosio::table]] config {
    uint32_t min_account_age_days;
    uint32_t invite_rate_seconds;
    bool enabled;
    name admin;
    uint16_t max_referral_depth;     // Maximum levels deep for referral rewards
    std::vector<uint8_t> level_multipliers;  // Points multiplier for each level
    uint8_t default_multiplier;      // Default multiplier for levels beyond the array
};

// === Register User ===
// Registers a user with a referral code and applies multi-level scoring
void invitono::registeruser(name user, name inviter) {
  require_auth(user);

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
  time_point_sec creation_date = get_account_creation(user);
  time_point_sec now = time_point_sec(current_time_point());
  check((now.sec_since_epoch() - creation_date.sec_since_epoch()) >= cfg.min_account_age_days * 86400,
        "Account not old enough to register");

  adopters.emplace(user, [&](auto& row) {
    row.account = user;
    row.invitedby = inviter;
    row.lastupdated = current_time_point();
    row.score = 0;
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
    time_point now = current_time_point();

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
            if ((now.sec_since_epoch() - itr->lastupdated.sec_since_epoch()) >= cfg.invite_rate_seconds) {
                // Use array value if available, otherwise use default_multiplier
                uint8_t multiplier = (level <= cfg.level_multipliers.size()) 
                    ? cfg.level_multipliers[level - 1] 
                    : cfg.default_multiplier;
                
                adopters.modify(itr, same_payer, [&](auto& row) {
                    row.score += multiplier;
                    row.lastupdated = now;
                });
            }
        }
    }
}

// === Claim Reward ===
// Mints tokens based on invite score (1 TOKEN per point)
void invitono::claimreward(name user) {
  require_auth(user);

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
  
  // Convert to asset with overflow check
  check(position <= UINT32_MAX / 10000, "Position overflow");
  asset reward = asset(static_cast<int64_t>(position) * 10000, symbol("TOKEN", 4));

  // Note: Balance check requires token contract integration
  // Assuming standard eosio.token contract
  action(
    permission_level{get_self(), "active"_n},
    "eosio.token"_n,
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
    std::vector<uint8_t> multipliers,
    uint8_t default_mult
) {
    config_table conf(get_self(), get_self().value);

    // Validate new parameters
    check(max_depth > 0 && max_depth <= 100, "Invalid depth (1-100)");
    check(multipliers.size() <= max_depth, "Too many multipliers");
    check(default_mult > 0, "Default multiplier must be positive");
    for(const auto& mult : multipliers) {
        check(mult > 0, "Multipliers must be positive");
    }

    // Handle first-time initialization
    if (!conf.exists()) {
        require_auth(get_self());
        conf.set(config{
            .min_account_age_days = min_age_days,
            .invite_rate_seconds = rate_seconds,
            .enabled = enabled,
            .admin = admin,
            .max_referral_depth = max_depth,
            .level_multipliers = multipliers,
            .default_multiplier = default_mult
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
        .level_multipliers = multipliers,
        .default_multiplier = default_mult
    }, get_self());
}

// === Reset User ===
// Dev-only action to remove a user (for testing)
void invitono::resetuser(name user) {
  require_auth(get_self());

  adopters_table adopters(get_self(), get_self().value);
  auto itr = adopters.find(user.value);
  if (itr != adopters.end()) {
    adopters.erase(itr);
  }
}

// === Get Account Creation ===
// Placeholder: Ideally, integrate with system contract or oracle
time_point_sec invitono::get_account_creation(name user) {
  // This is a mock implementation
  // In production, query the eosio system contract or use an oracle
  check(is_account(user), "Account does not exist");
  // Mock creation date (e.g., now minus 60 days for testing)
  return time_point_sec(current_time_point().sec_since_epoch() - 60 * 86400);
}