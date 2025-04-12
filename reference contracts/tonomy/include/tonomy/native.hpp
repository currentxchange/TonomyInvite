#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio.tonomy/eosio.tonomy.hpp>

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

   using eosiotonomy::authority;
   using eosiotonomy::block_header;
   using eosiotonomy::key_weight;
   using eosiotonomy::permission_level_weight;
   using eosiotonomy::wait_weight;

   /**
    * The `eosio.tonomy` is the first sample of system contract provided by `block.one` through the EOSIO platform. It is a minimalist system contract because it only supplies the actions that are absolutely critical to bootstrap a chain and nothing more. This allows for a chain agnostic approach to bootstrapping a chain.
    *
    * Just like in the `eosio.system` sample contract implementation, there are a few actions which are not implemented at the contract level (`newaccount`, `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, `canceldelay`, `onerror`, `setabi`, `setcode`), they are just declared in the contract so they will show in the contract's ABI and users will be able to push those actions to the chain via the account holding the `eosio.system` contract, but the implementation is at the EOSIO core level. They are referred to as EOSIO native actions.
    */
   class [[eosio::contract("tonomy")]] native : public eosio::contract
   {
   public:
      using contract::contract;
      static constexpr eosio::name governance_name = "tonomy"_n;

      native(name receiver, name code, eosio::datastream<const char *> ds) : contract(receiver, code, ds) {}

      /**
       * New account action, called after a new account is created. This code enforces resource-limits rules
       * for new accounts as well as new account naming conventions.
       *
       * 1. accounts cannot contain '.' symbols which forces all acccounts to be 12
       * characters long without '.' until a future account auction process is implemented
       * which prevents name squatting.
       *
       * 2. new accounts must stake a minimal number of tokens (as set in system parameters)
       * therefore, this method will execute an inline buyram from receiver for newacnt in
       * an amount equal to the current new account creation fee.
       */
      [[eosio::action]] void newaccount(name creator,
                                        name name,
                                        authority owner,
                                        authority active);
      /**
       * Update authorization action updates pemission for an account.
       *
       * @param account - the account for which the permission is updated,
       * @param pemission - the permission name which is updated,
       * @param parem - the parent of the permission which is updated,
       * @param auth - the json describing the permission authorization,
       * @param auth_parent - true if the parent permission should be checked, otherwise the "permission" will be used to authorize.
       *                      should be true when a new permission is being created, otherwise false
       */
      [[eosio::action]] void updateauth(name account,
                                        name permission,
                                        name parent,
                                        authority auth,
                                        bool auth_parent);

      /**
       * Delete authorization action deletes the authorization for an account's permission.
       *
       * @param account - the account for which the permission authorization is deleted,
       * @param permission - the permission name been deleted.
       */
      [[eosio::action]] void deleteauth(name account,
                                        name permission);

      /**
       * Link authorization action assigns a specific action from a contract to a permission you have created. Five system
       * actions can not be linked `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, and `canceldelay`.
       * This is useful because when doing authorization checks, the EOSIO based blockchain starts with the
       * action needed to be authorized (and the contract belonging to), and looks up which permission
       * is needed to pass authorization validation. If a link is set, that permission is used for authoraization
       * validation otherwise then active is the default, with the exception of `eosio.any`.
       * `eosio.any` is an implicit permission which exists on every account; you can link actions to `eosio.any`
       * and that will make it so linked actions are accessible to any permissions defined for the account.
       *
       * @param account - the permission's owner to be linked and the payer of the RAM needed to store this link,
       * @param code - the owner of the action to be linked,
       * @param type - the action to be linked,
       * @param requirement - the permission to be linked.
       */
      [[eosio::action]] void linkauth(name account,
                                      name code,
                                      name type,
                                      name requirement);

      /**
       * Unlink authorization action it's doing the reverse of linkauth action, by unlinking the given action.
       *
       * @param account - the owner of the permission to be unlinked and the receiver of the freed RAM,
       * @param code - the owner of the action to be unlinked,
       * @param type - the action to be unlinked.
       */
      [[eosio::action]] void unlinkauth(name account,
                                        name code,
                                        name type);

      /**
       * Cancel delay action cancels a deferred transaction.
       *
       * @param canceling_auth - the permission that authorizes this action,
       * @param trx_id - the deferred transaction id to be cancelled.
       */
      [[eosio::action]] void canceldelay(permission_level canceling_auth, checksum256 trx_id);

      /**
       * Set code action sets the contract code for an account.
       *
       * @param account - the account for which to set the contract code.
       * @param vmtype - reserved, set it to zero.
       * @param vmversion - reserved, set it to zero.
       * @param code - the code content to be set, in the form of a blob binary..
       */
      [[eosio::action]] void setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code);

      /**
       * Set abi action sets the abi for contract identified by `account` name. Creates an entry in the abi_hash_table
       * index, with `account` name as key, if it is not already present and sets its value with the abi hash.
       * Otherwise it is updating the current abi hash value for the existing `account` key.
       *
       * @param account - the name of the account to set the abi for
       * @param abi     - the abi hash represented as a vector of characters
       */
      [[eosio::action]] void setabi(name account, const std::vector<char> &abi);

      /**
       * On error action, notification of this action is delivered to the sender of a deferred transaction
       * when an objective error occurs while executing the deferred transaction.
       * This action is not meant to be called directly.
       *
       * @param sender_id - the id for the deferred transaction chosen by the sender,
       * @param sent_trx - the deferred transaction that failed.
       */
      [[eosio::action]] void onerror(uint128_t sender_id, std::vector<char> sent_trx);

      /**
       * Set privilege action allows to set privilege status for an account (turn it on/off).
       * @param account - the account to set the privileged status for.
       * @param is_priv - 0 for false, > 0 for true.
       */
      [[eosio::action]] void setpriv(name account, uint8_t is_priv);

      /**
       * Sets the resource limits of an account
       *
       * @param account - name of the account whose resource limit to be set
       * @param ram_bytes - ram limit in absolute bytes
       * @param net_weight - fractionally proportionate net limit of available resources based on (weight / total_weight_of_all_accounts)
       * @param cpu_weight - fractionally proportionate cpu limit of available resources based on (weight / total_weight_of_all_accounts)
       */
      [[eosio::action]] void setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);

      /**
       * Set producers action, sets a new list of active producers, by proposing a schedule change, once the block that
       * contains the proposal becomes irreversible, the schedule is promoted to "pending"
       * automatically. Once the block that promotes the schedule is irreversible, the schedule will
       * become "active".
       *
       * @param schedule - New list of active producers to set
       */
      [[eosio::action]] void setprods(const std::vector<eosio::producer_authority> &schedule);

      /**
       * Set params action, sets the blockchain parameters. By tuning these parameters, various degrees of customization can be achieved.
       *
       * @param params - New blockchain parameters to set
       */
      [[eosio::action]] void setparams(const eosio::blockchain_parameters &params);

      /**
       * Require authorization action, checks if the account name `from` passed in as param has authorization to access
       * current action, that is, if it is listed in the action’s allowed permissions vector.
       *
       * @param from - the account name to authorize
       */
      [[eosio::action]] void reqauth(name from);

      /**
       * Activate action, activates a protocol feature
       *
       * @param feature_digest - hash of the protocol feature to activate.
       */
      [[eosio::action]] void activate(const eosio::checksum256 &feature_digest);

      /**
       * Require activated action, asserts that a protocol feature has been activated
       *
       * @param feature_digest - hash of the protocol feature to check for activation.
       */
      [[eosio::action]] void reqactivated(const eosio::checksum256 &feature_digest);

      using newaccount_action = action_wrapper<"newaccount"_n, &native::newaccount>;
      using updateauth_action = action_wrapper<"updateauth"_n, &native::updateauth>;
      using deleteauth_action = action_wrapper<"deleteauth"_n, &native::deleteauth>;
      using linkauth_action = action_wrapper<"linkauth"_n, &native::linkauth>;
      using unlinkauth_action = action_wrapper<"unlinkauth"_n, &native::unlinkauth>;
      using canceldelay_action = action_wrapper<"canceldelay"_n, &native::canceldelay>;
      using setcode_action = action_wrapper<"setcode"_n, &native::setcode>;
      using setabi_action = action_wrapper<"setabi"_n, &native::setabi>;
      using onerror_action = action_wrapper<"onerror"_n, &native::onerror>;
      using setpriv_action = action_wrapper<"setpriv"_n, &native::setpriv>;
      using setalimits_action = action_wrapper<"setalimits"_n, &native::setalimits>;
      using setprods_action = action_wrapper<"setprods"_n, &native::setprods>;
      using setparams_action = action_wrapper<"setparams"_n, &native::setparams>;
      using reqauth_action = action_wrapper<"reqauth"_n, &native::reqauth>;
      using activate_action = action_wrapper<"activate"_n, &native::activate>;
      using reqactivated_action = action_wrapper<"reqactivated"_n, &native::reqactivated>;
   };
}
