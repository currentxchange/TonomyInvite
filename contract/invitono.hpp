// === Invitono Smart Contract Header === \
#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using std::string;

// --- Invitono contract to manage multi-level referrals --- \
CONTRACT invitono : public contract {
public:
  using contract::contract;

  // --- User Actions --- \
  ACTION registeruser(name user, name inviter);       // User registers with an invite code
  ACTION claimreward(name user);                      // User claims reward based on score
  ACTION setconfig(name admin, uint32_t min_age_days, uint32_t rate_seconds, bool enabled); // Admin sets config
  ACTION resetuser(name user);                        // Dev-only: reset a user

  // === Adopter Table === \
  /*/ Tracks each registered user and referral stats /*/
  TABLE adopter {
    name        account;          // WAX account
    name        invitedby;        // Who invited them
    time_point  lastupdated;      // Last time their score was updated
    uint32_t    score = 0;        // Referral score
    bool        claimed = false;  // Whether they've claimed

    uint64_t primary_key() const { return account.value; }
    uint64_t by_score() const { return (uint64_t{UINT32_MAX} - score); } // Sort descending
  };

  typedef multi_index<"adopters"_n, adopter,
    indexed_by<"byscore"_n, const_mem_fun<adopter, uint64_t, &adopter::by_score>>
  > adopters_table; //END adopters_table

  // === Config Singleton === \
  /*/ Contract configuration values /*/
  TABLE config {
    uint32_t min_account_age_days = 30; // Minimum age to register (in days)
    uint32_t invite_rate_seconds = 3600; // Seconds between score updates
    bool     enabled = true;             // Toggle contract
    name     admin;                      // Contract admin
  };

  typedef singleton<"config"_n, config> config_table; //END config_table

  // === Stats Singleton === \
  /*/ Global stats tracked by contract /*/
  TABLE stats {
    uint64_t total_referrals = 0; // Total referral connections
    uint64_t total_users = 0;     // Total users registered
    name     last_registered;     // Last registered user
  };

  typedef singleton<"stats"_n, stats> stats_singleton; //END stats_singleton

private:
  // --- Internal Score Updater --- \
  void update_scores(name direct_inviter); // Updates inviter and their inviter
}; //END invitono
