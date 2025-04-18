#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using std::string;

// --- Invitono contract to manage multi-level referrals ---
CONTRACT invitono : public contract {
public:
  using contract::contract;

  // --- User Actions ---
  [[eosio::action]] void registeruser(name user, name inviter); // User registers with an invite code
  [[eosio::action]] void claimreward(name user);                // User claims reward based on score
  [[eosio::action]] void setconfig(
      name admin, 
      uint32_t min_age_days, 
      uint32_t rate_seconds, 
      bool enabled,
      uint16_t max_depth,
      std::vector<uint8_t> multipliers,
      uint8_t default_mult
  ); // Admin sets config
  [[eosio::action]] void resetuser(name user);                 // Dev-only: reset a user

  // === Adopter Table ===
  /* Tracks each registered user and referral stats */
  TABLE adopter {
    name        account;          // WAX account
    name        invitedby;        // Who invited them
    time_point  lastupdated;      // Last time their score was updated
    uint32_t    score = 0;        // Referral score
    bool        claimed = false;  // Whether they've claimed

    uint64_t primary_key() const { return account.value; }
    uint64_t by_score() const { return static_cast<uint64_t>(UINT32_MAX - score); } // Sort descending
  };

  typedef multi_index<"adopters"_n, adopter,
    indexed_by<"byscore"_n, const_mem_fun<adopter, uint64_t, &adopter::by_score>>
  > adopters_table;

  // === Config Singleton ===
  /* Contract configuration values */
  TABLE config {
    uint32_t min_account_age_days = 30; // Minimum age to register (in days)
    uint32_t invite_rate_seconds = 3600; // Seconds between score updates
    bool     enabled = true;             // Toggle contract
    name     admin;                      // Contract admin
    uint16_t max_referral_depth = 5;     // Maximum levels deep for referral rewards (default 5)
    std::vector<uint8_t> level_multipliers; // Points multiplier for each level
    uint8_t default_multiplier = 1;      // Default multiplier for levels beyond array
  };

  typedef singleton<"config"_n, config> config_table;

  // === Stats Singleton ===
  /* Global stats tracked by contract */
  TABLE stats {
    uint64_t total_referrals = 0; // Total referral connections
    uint64_t total_users = 0;     // Total users registered
    name     last_registered;     // Last registered user
  };

  typedef singleton<"stats"_n, stats> stats_table;

  // === Account Table ===
  /* Tracks account metadata from eosio system */
  TABLE account {
    name         account_name;
    time_point_sec creation_date;

    uint64_t primary_key() const { return account_name.value; }
  };
  typedef eosio::multi_index<"accounts"_n, account> accounts_table;

private:
  // --- Internal Score Updater ---
  void update_scores(name direct_inviter); // Updates inviter and their inviter

  // --- Get Account Metadata ---
  time_point_sec get_account_creation(name user);
};