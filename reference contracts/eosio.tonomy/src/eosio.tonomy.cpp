#include <eosio.tonomy/eosio.tonomy.hpp>

namespace eosiotonomy
{
   void bios::check_sender(name sender)
   {
      check(eosio::get_sender() == sender, "You cannot call this eosio action directly, call from the " + sender.to_string() + " contract");
   }

   void bios::newaccount(name creator,
                         name name,
                         ignore<authority> owner,
                         ignore<authority> active)
   {
      check_sender(tonomy_system_name);
   }

   void bios::updateauth(ignore<name> account,
                         ignore<name> permission,
                         ignore<name> parent,
                         ignore<authority> auth)
   {
      check_sender(tonomy_system_name);
   }

   void bios::deleteauth(ignore<name> account,
                         ignore<name> permission)
   {
      check_sender(tonomy_system_name);
   }

   void bios::linkauth(ignore<name> account,
                       ignore<name> code,
                       ignore<name> type,
                       ignore<name> requirement)
   {
      check_sender(tonomy_system_name);
   }

   void bios::unlinkauth(ignore<name> account,
                         ignore<name> code,
                         ignore<name> type)
   {
      check_sender(tonomy_system_name);
   }

   void bios::canceldelay(ignore<permission_level> canceling_auth, ignore<checksum256> trx_id)
   {
      check_sender(tonomy_system_name);
   }

   void bios::setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code)
   {
      if (eosio::get_sender() == ""_n)
      {
         // this is easier to allow the transition to this contract
         // https://t.me/antelopedevs/337128
         require_auth(tonomy_system_name);
      }
      else
      {
         check_sender(tonomy_system_name);
      }
   }

   void bios::setabi(name account, const std::vector<char> &abi)
   {
      if (eosio::get_sender() == ""_n)
      {
         // this is easier to allow the transition to this contract
         // https://t.me/antelopedevs/337128
         require_auth(tonomy_system_name);
      }
      else
      {
         check_sender(tonomy_system_name);
      }
      abi_hash_table table(get_self(), get_self().value);
      auto itr = table.find(account.value);
      if (itr == table.end())
      {
         table.emplace(account, [&](auto &row)
                       {
         row.owner = account;
         row.hash  = eosio::sha256(const_cast<char*>(abi.data()), abi.size()); });
      }
      else
      {
         table.modify(itr, eosio::same_payer, [&](auto &row)
                      { row.hash = eosio::sha256(const_cast<char *>(abi.data()), abi.size()); });
      }
   }

   void bios::onerror(ignore<uint128_t>, ignore<std::vector<char>>)
   {
      check(false, "the onerror action cannot be called directly");
   }

   void bios::setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight)
   {
      check_sender(tonomy_system_name);
      set_resource_limits(account, ram_bytes, net_weight, cpu_weight);
   }

   void bios::setprods(const std::vector<eosio::producer_authority> &schedule)
   {
      check_sender(tonomy_system_name);
      set_proposed_producers(schedule);
   }

   void bios::setparams(const eosio::blockchain_parameters &params)
   {
      check_sender(tonomy_system_name);
      set_blockchain_parameters(params);
   }

   void bios::reqauth(name from)
   {
      // TODO is this function needed?
      require_auth(from);
   }

   void bios::setpriv(name account, uint8_t is_priv)
   {
      check_sender(tonomy_system_name);
      set_privileged(account, is_priv);
   }

   void bios::activate(const eosio::checksum256 &feature_digest)
   {
      check_sender(tonomy_system_name);
      preactivate_feature(feature_digest);
   }

   void bios::reqactivated(const eosio::checksum256 &feature_digest)
   {
      // TODO is this function needed?
      check(is_feature_activated(feature_digest), "protocol feature is not activated");
   }

}