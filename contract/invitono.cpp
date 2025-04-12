// === Invitono Smart Contract Implementation === \\

//eosio-cpp -abigen -I include -R resource -contract invitono -o invitono.wasm contract/invitono.cpp

#include "invitono.hpp"
#include <eosio.system/eosio.system.hpp>
#include <eosio/system.hpp>
#include <eosio.token/eosio.token.hpp>

// === Register User === \
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

  // CHECK : Configuration check
  config_table conf(get_self(), get_self().value);
  auto cfg = conf.get();
  check(cfg.enabled, "Registration is currently disabled");

  // CHECK : Verify account age
  auto acct = get_account(user);
  time_point_sec now = current_time_point().sec_since_epoch();
  check((now.sec_since_epoch() - acct.creation_date.sec_since_epoch()) >= cfg.min_account_age_days * 86400,
    "Account not old enough to register");

  adopters.emplace(user, [&](auto& row) {
    row.account = user;
    row.invitedby = inviter;
    row.lastupdated = time_point_sec(0);
    row.score = 0;
    row.claimed = false;
  });

  // Update global stats
  stats_singleton stats(get_self(), get_self().value);
  auto current = stats.exists() ? stats.get() : stats{};
  current.total_users += 1;
  current.total_referrals += 1;
  current.last_registered = user;
  stats.set(current, get_self());

  update_scores(inviter); // Updates inviter & their uplines
}//END registeruser()


// === Update Scores === \
// Applies +1 score to inviter and their upline if cooldown has passed
void invitono::update_scores(name direct_inviter) {
  adopters_table adopters(get_self(), get_self().value);
  config_table conf(get_self(), get_self().value);
  auto cfg = conf.get();
  time_point now = current_time_point();

  std::vector<name> upline;

  auto inviter_itr = adopters.find(direct_inviter.value);
  if (inviter_itr != adopters.end()) {
    upline.push_back(inviter_itr->account);

    if (inviter_itr->invitedby != name("")) {
      auto parent_itr = adopters.find(inviter_itr->invitedby.value);
      if (parent_itr != adopters.end()) {
        upline.push_back(parent_itr->account);
      }
    }
  }

  for (auto& upline_account : upline) {
    auto itr = adopters.find(upline_account.value);
    if (itr != adopters.end()) {
      if ((now.sec_since_epoch() - itr->lastupdated.sec_since_epoch()) >= cfg.invite_rate_seconds) {
        adopters.modify(itr, same_payer, [&](auto& row) {
          row.score += 1;
          row.lastupdated = now;
        });
      }
    }
  }
}//END update_scores()


// === Claim Reward === \
// Mints tokens based on invite score (1 TOKEN per point)
void invitono::claimreward(name user) {
  require_auth(user);

  // Add config check
  config_table conf(get_self(), get_self().value);
  auto cfg = conf.get();
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

  // Calculate reward with overflow check
  check(score <= UINT32_MAX / 10000, "Score overflow");
  asset reward = asset(score * 10000, symbol("TOKEN", 4));
  
  // Add balance check
  asset contract_balance = eosio::token::get_balance("eosio.token"_n, get_self(), reward.symbol.code());
  check(contract_balance >= reward, "Insufficient contract balance");

  // Send reward
  action(
    permission_level{get_self(), "active"_n},
    "eosio.token"_n,
    "transfer"_n,
    std::make_tuple(get_self(), user, reward, std::string("Invitono referral reward"))
  ).send();
}//END claimreward()


// === Set Config === \
// Admin sets contract-wide configuration
void invitono::setconfig(name admin, uint32_t min_age_days, uint32_t rate_seconds, bool enabled) {
  config_table conf(get_self(), get_self().value);
  
  // Handle first-time initialization
  if (!conf.exists()) {
    require_auth(get_self());
    conf.set(config{
      .min_account_age_days = min_age_days,
      .invite_rate_seconds = rate_seconds,
      .enabled = enabled,
      .admin = admin
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
    .admin = admin
  }, get_self());
}//END setconfig()


// === Reset User === \
// Dev-only action to remove a user (for testing)
void invitono::resetuser(name user) {
  require_auth(get_self());

  adopters_table adopters(get_self(), get_self().value);
  auto itr = adopters.find(user.value);
  if (itr != adopters.end()) {
    adopters.erase(itr);
  }
}//END resetuser()


// === Get Account Metadata === \
// ! HACK : This is a fake implementation to show intention.
// Replace with inline action or oracles to pull this info on-chain
struct account_info {
  time_point_sec creation_date;
};

account_info get_account(name user) {
  accounts_table accounts("eosio"_n, "eosio"_n);
  auto acct = accounts.get(user.value, "Account not found");
  
  return account_info{
    .creation_date = acct.creation_date
  };
}//END get_account()

// At the top after other includes
TABLE account {
    name         account_name;
    time_point   creation_date;
    
    uint64_t primary_key()const { return account_name.value; }
};
typedef eosio::multi_index<"accounts"_n, account> accounts_table;


