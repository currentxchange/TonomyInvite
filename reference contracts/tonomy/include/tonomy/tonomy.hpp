#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include "native.hpp"

namespace tonomysystem
{
   using eosio::action_wrapper;
   using eosio::asset;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::print;
   using eosio::public_key;
   using eosio::singleton;
   using std::string;

   // Create an enum type and an eosio type for enums
   // https://eosio.stackexchange.com/questions/4950/store-enum-value-in-table
   enum enum_account_type
   {
      Person,
      Organization,
      App,
      Gov,
      Service
   };
   typedef uint8_t account_type;

   enum enum_account_status
   {
      Creating_Status,
      Active_Status,
      Deactivated_Status,
      Upgrading_Status
   };
   typedef uint8_t account_status;

   enum enum_permission_level_name
   {
      Owner,
      Active,
      Password,
      Pin,
      Biometric,
      Local
   };
   typedef uint8_t permission_level_name;

   /**
    * The `eosio.tonomy` is the first sample of system contract provided by `block.one` through the EOSIO platform. It is a minimalist system contract because it only supplies the actions that are absolutely critical to bootstrap a chain and nothing more. This allows for a chain agnostic approach to bootstrapping a chain.
    *
    * Just like in the `eosio.system` sample contract implementation, there are a few actions which are not implemented at the contract level (`newaccount`, `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, `canceldelay`, `onerror`, `setabi`, `setcode`), they are just declared in the contract so they will show in the contract's ABI and users will be able to push those actions to the chain via the account holding the `eosio.system` contract, but the implementation is at the EOSIO core level. They are referred to as EOSIO native actions.
    */
   class [[eosio::contract("tonomy")]] tonomy : public native
   {
   public:
      uint64_t inital_ram_bytes = 6000;
      uint64_t initial_cpu_weight_allocation = 1000;
      uint64_t initial_net_weight_allocation = 1000;

      static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
      static constexpr eosio::name token_contract_name = "eosio.token"_n;

      /**
       * Constructor for the contract, which initializes the _accounts table
       */
      tonomy(name receiver, name code, eosio::datastream<const char *> ds);

      /**
       * Create a new account for a person
       *
       * @details Creates a new account for a person.
       *
       * @param username_hash - hash of the username of the account
       * @param password_key - public key generated from the account's password
       * @param password_salt - salt used to generate the password_key with the password
       */
      [[eosio::action]] void newperson(
          checksum256 username_hash,
          public_key password_key,
          checksum256 password_salt);

      /**
       * Manually sets the details of an app (admin only)
       *
       * @param account_name - name of the account
       * @param app_name - name of the app
       * @param description - description of the app
       * @param username_hash - hash of the username
       * @param logo_url - url to the logo of the app
       * @param origin - domain associated with the app
       */
      [[eosio::action]] void adminsetapp(
          name account_name,
          string app_name,
          string description,
          checksum256 username_hash,
          string logo_url,
          string origin);

      /**
       * Create a new account for an app and registers it's details
       *
       * @details Creates a new account for an app and registers it's details.
       *
       * @param name - name of the app
       * @param description - description of the app
       * @param username_hash - hash of the username
       * @param logo_url - url to the logo of the app
       * @param origin - domain associated with the app
       * @param password_key - public key generated from the account's password
       */
      [[eosio::action]] void newapp(
          string app_name,
          string description,
          checksum256 username_hash,
          string logo_url,
          string origin,
          public_key key);

      /**
       * Adds a new key to a person's account to log into an app with
       *
       * @param account - account of the person
       * @param app - account of the app to authorize the key to
       * @param parent - parent permission of the new permission
       * @param key - public key to authorize
       */
      [[eosio::action]] void loginwithapp(
          name account,
          name app,
          name parent,
          public_key key);

      /**
       * Update a key of a person
       *
       * @param account - name of the account to update
       * @param permission - permission level of the key to update
       * @param key - public key to update
       */
      [[eosio::action]] void updatekeyper(name account,
                                          permission_level_name permission,
                                          public_key key,
                                          bool link_auth = false);

      /**
       * Set the resource parameters for the system
       * @param ram_price - the price of RAM (bytes per token)
       * @param total_ram_available - the total amount of RAM available (bytes)
       * @param ram_fee - the fee RAM purchases in fraction (0.01 = 1% fee)
       */
      [[eosio::action]] void setresparams(double ram_price, uint64_t total_ram_available, double ram_fee);

      /**
       * Buy RAM action allows an app to purchase RAM.
       * It checks the account type of the app, ensures the RAM is being purchased with the correct token,
       * and that the amount of tokens being used for the purchase is positive.
       * It then calculates the amount of RAM to purchase based on the current RAM price,
       * checks if there is enough available RAM, and allocates the purchased RAM to the app.
       * Finally, it updates the total RAM used and available in the system, and
       * transfers the tokens used for the purchase.
       *
       * @param dao_owner - the name of the DAO owner account
       * @param app - the name of the app account purchasing the RAM
       * @param quant - the amount and symbol of the tokens used for the purchase
       */
      [[eosio::action]] void buyram(const name &dao_owner, const name &app, const asset &quant);

      /**
       * Sell RAM action allows an app to sell RAM.
       * It checks the account type of the app, ensures the RAM is being sold for the correct token,
       * and that the amount of RAM being sold is positive.
       * It then calculates the amount of tokens to return based on the current RAM price,
       * checks if there is enough RAM being used by the app, and deallocates the sold RAM from the app.
       * Finally, it updates the total RAM used in the system, and
       * transfers the tokens from the sale.
       *
       * @param dao_owner - the name of the DAO owner account
       * @param app - the name of the app account selling the RAM
       * @param quant - the amount and symbol of the tokens used to sell
       */
      [[eosio::action]] void sellram(eosio::name dao_owner, eosio::name app, eosio::asset quant);

      struct [[eosio::table]] account_type_struct
      {
         name account_name;
         account_type acc_type;
         uint16_t version; // used for upgrading the account structure

         uint64_t primary_key() const { return account_name.value; }
         EOSLIB_SERIALIZE(struct account_type_struct, (account_name)(acc_type)(version))
      };

      typedef eosio::multi_index<"acctypes"_n, account_type_struct> account_type_table;

      struct [[eosio::table]] person
      {
         name account_name;
         account_status status;
         checksum256 username_hash;
         checksum256 password_salt;

         // primary key automatically added by EOSIO method
         uint64_t primary_key() const { return account_name.value; }
         // also index by username hash
         checksum256 index_by_username_hash() const { return username_hash; }
      };

      // Create a multi-index-table with two indexes
      typedef eosio::multi_index<"people"_n, person,
                                 eosio::indexed_by<"usernamehash"_n, eosio::const_mem_fun<person, checksum256, &person::index_by_username_hash>>>
          people_table;

      // Create an instance of the table that can is initalized in the constructor
      people_table _people;

      struct [[eosio::table]] app
      {
         name account_name;
         string app_name;
         checksum256 username_hash;
         string description;
         string logo_url;
         string origin;

         // primary key automatically added by EOSIO method
         uint64_t primary_key() const { return account_name.value; }
         // also index by username hash
         checksum256 index_by_username_hash() const { return username_hash; }
         checksum256 index_by_origin_hash() const { return eosio::sha256(origin.c_str(), std::strlen(origin.c_str())); }
      };

      // Create a multi-index-table with two indexes
      typedef eosio::multi_index<"apps"_n, app,
                                 eosio::indexed_by<"usernamehash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_username_hash>>,
                                 eosio::indexed_by<"originhash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_origin_hash>>>
          apps_table;

      // Create an instance of the table that can is initalized in the constructor
      apps_table _apps;

      struct [[eosio::table]] resource_config
      {
         double ram_fee;                      // RAM fee fraction (0.01 = 1% fee)
         double ram_price;                    // RAM price (bytes per token)
         uint64_t total_ram_available;        // Total available RAM (bytes)
         uint64_t total_ram_used;             // Total RAM used (bytes)
         uint64_t total_cpu_weight_allocated; // Total allocated (CPU weight)
         uint64_t total_net_weight_allocated; // Total allocated (NET weight)

         EOSLIB_SERIALIZE(resource_config, (ram_fee)(ram_price)(total_ram_available)(total_ram_used)(total_cpu_weight_allocated)(total_net_weight_allocated))
      };
      typedef eosio::singleton<"resconfig"_n, resource_config> resource_config_table;
      // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
      typedef eosio::multi_index<"resconfig"_n, resource_config> resource_config_table_dump;

      /**
       * Returns the account name of the app that corresponds to the origin
       *
       * @param {string} origin - the origin of the app
       * @example "https://www.tonomy.com"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_origin(string origin, name contract_name = "id.tmy"_n)
      {
         apps_table id_apps = apps_table(contract_name, contract_name.value);
         auto apps_by_origin_hash_itr = id_apps.get_index<"originhash"_n>();

         eosio::checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
         const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
         check(origin_itr == apps_by_origin_hash_itr.end(), "No app with this origin found");

         return origin_itr->account_name;
      }

      /**
       * Returns the account name of the app that corresponds to the origin
       *
       * @param {string} username - the username of the app
       * @example "demo.app.tonomy.id"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_username(string username, name contract_name = "tonomy"_n)
      {
         apps_table id_apps = apps_table(contract_name, contract_name.value);
         auto apps_by_username_hash_itr = id_apps.get_index<"usernamehash"_n>();

         eosio::checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
         const auto username_itr = apps_by_username_hash_itr.find(username_hash);
         check(username_itr == apps_by_username_hash_itr.end(), "No app with this username found");

         return username_itr->account_name;
      }

      using newperson_action = action_wrapper<"newperson"_n, &tonomy::newperson>;
      using updatekeyper_action = action_wrapper<"updatekeyper"_n, &tonomy::updatekeyper>;
      using newapp_action = action_wrapper<"newapp"_n, &tonomy::newapp>;
      using loginwithapp_action = action_wrapper<"loginwithapp"_n, &tonomy::loginwithapp>;
      using adminsetapp_action = action_wrapper<"adminsetapp"_n, &tonomy::adminsetapp>;
      using setresparams_action = action_wrapper<"setresparams"_n, &tonomy::setresparams>;
      using buyram_action = action_wrapper<"buyram"_n, &tonomy::buyram>;
      using sellram_action = action_wrapper<"sellram"_n, &tonomy::sellram>;
   };
}