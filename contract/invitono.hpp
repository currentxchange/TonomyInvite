#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>
#include <eosio/permission.hpp> 


using namespace eosio;
using std::string;

// --- Invitono contract to manage multi-level referrals ---
CONTRACT invitono : public contract {
public:
  using contract::contract;

  // --- User Actions ---
  ACTION registeruser(name user, name inviter); // User registers with an invite code
  ACTION claimreward(name user);                // User claims reward based on score
  ACTION setconfig(
      name admin, 
      uint32_t min_age_days, 
      uint32_t rate_seconds, 
      bool enabled,
      uint16_t max_depth,
      uint16_t multiplier,
      name token_contract,
      symbol reward_symbol,
      uint32_t reward_rate
  ); // Admin sets config
  ACTION deleteuser(name user);                 // Dev-only: delete a user

  // === Adopter Table ===
  /* Tracks each registered user and referral stats */
  TABLE adopter {
    name        account;          // WAX account
    name        invitedby;        // Who invited them
    uint32_t    lastupdated;      // Last time their score was updated (seconds since epoch)
    uint32_t    score = 0;        // Referral score
    bool        claimed = false;  // Whether they've claimed

    uint64_t primary_key() const { return account.value; }
    uint64_t by_score() const { return static_cast<uint64_t>(UINT32_MAX - score); } // Sort descending
  };

  using adopters_table = multi_index<"adopters"_n, adopter,
    indexed_by<"byscore"_n, const_mem_fun<adopter, uint64_t, &adopter::by_score>>
  >;

  // === Config Singleton ===
  /* Contract configuration values */
  TABLE config {
    uint32_t min_account_age_days = 30; // Minimum age to register (in days)
    uint32_t invite_rate_seconds = 3600; // Seconds between score updates
    bool     enabled = true;             // Toggle contract
    name     admin;                      // Contract admin
    uint16_t max_referral_depth = 5;     // Maximum levels deep for referral rewards (default 5)
    uint16_t multiplier = 100;           // Score multiplier (100 = 1.0x)
    name     token_contract;             // Token contract account
    symbol   reward_symbol;              // Token symbol for rewards
    uint32_t reward_rate = 100;          // Tokens per point in hundredths (100 = 1.00 token)
  };

  using config_table = singleton<"config"_n, config>;

  // === Stats Singleton ===
  /* Global stats tracked by contract */
  TABLE stats {
    uint64_t total_referrals = 0; // Total referral connections
    uint64_t total_users = 0;     // Total users registered
    name     last_registered;     // Last registered user
  };

  using stats_table = singleton<"stats"_n, stats>;

private:
  // --- Internal Score Updater ---
  void update_scores(name direct_inviter); // Updates inviter and their inviter

  // --- Tetrahedral Series Array ---
  const std::vector<uint32_t> TETRAHEDRAL = {1, 4, 10, 20, 35, 56, 84, 120, 165, 220, 286, 364, 455, 560, 680, 816, 969, 1140, 1330, 1540, 1771, 2024, 2300, 2600, 999999999};

  // --- Tetrahedral Series Position Calculation ---
  uint32_t calculate_tetrahedral_position(uint32_t score) {
    // Find the largest n where T(n) <= score
    for (size_t i = 0; i < TETRAHEDRAL.size(); i++) {
      if (TETRAHEDRAL[i] > score) {
        return i; // Return the index where we exceeded the score
      }
    }
    return TETRAHEDRAL.size() - 1; // Return last position if score is very large
  }
};