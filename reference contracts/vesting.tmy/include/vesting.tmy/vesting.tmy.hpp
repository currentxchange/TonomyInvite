// vesting.tmy.hpp

#pragma once

#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

namespace vestingtoken
{
    using eosio::action_wrapper;
    using eosio::asset;
    using eosio::check;
    using eosio::days;
    using eosio::microseconds;
    using eosio::name;
    using eosio::singleton;
    using eosio::time_point;
    using std::string;

    struct vesting_category
    {
        microseconds cliff_period;
        microseconds start_delay;
        microseconds vesting_period;
        double tge_unlock;
    };

    static const std::map<int, vesting_category> vesting_categories = {
        // DEPRECIATED:
        {1, {days(6 * 30), days(0 * 30), days(2 * 365), 0.0}}, // Seed Private Sale (DEPRECIATED),
        {2, {days(6 * 30), days(6 * 30), days(2 * 365), 0.0}}, // Strategic Partnerships Private Sale (DEPRECIATED),
        // Unchanged:
        {3, {days(0 * 30), days(0 * 30), days(0 * 30), 0.0}},   // Public Sale (DO NOT USED YET),
        {4, {days(0 * 30), days(1 * 365), days(5 * 365), 0.0}}, // Team and Advisors, Ecosystem
        {5, {days(0 * 30), days(0 * 30), days(1 * 365), 0.0}},  // Legal and Compliance
        {6, {days(0 * 30), days(0 * 30), days(2 * 365), 0.0}},  // Reserves, Partnerships, Liquidly Allocation
        {7, {days(0 * 30), days(0 * 30), days(5 * 365), 0.0}},  // Community and Marketing, Platform Dev, Infra Rewards
        // New (replacing depreciated):
        {8, {days(0 * 30), days(0 * 30), days(2 * 365), 0.05}},  // Seed (Early Bird)
        {9, {days(0 * 30), days(0 * 30), days(2 * 365), 0.025}}, // Seed (Last Chance)
        {10, {days(0 * 30), days(14), days(0 * 365), 1.0}},      // Public (TGE)
        // Public sale has a delay of 14 days to accommodate the "right of withdrawal" under EU's MICA regulations

        // TESTING ONLY:
        {997, {days(6 * 30), days(0 * 30), days(2 * 365), 0.0}},                  // TESTING ONLY
        {998, {eosio::seconds(0), eosio::seconds(10), eosio::seconds(20), 0.5}},  // TESTING ONLY
        {999, {eosio::seconds(10), eosio::seconds(10), eosio::seconds(20), 0.0}}, // TESTING ONLY
    };

    static const std::map<int, bool> depreciated_categories = {{1, true}, {2, true}};

    class [[eosio::contract("vesting.tmy")]] vestingToken : public eosio::contract
    {
    public:
        using contract::contract;
        static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
        static constexpr eosio::name token_contract_name = "eosio.token"_n;
        static const uint32_t MAX_ALLOCATIONS = 150;

        struct [[eosio::table]] vesting_settings
        {
            eosio::time_point sales_start_date;
            eosio::time_point launch_date;

            EOSLIB_SERIALIZE(vesting_settings, (sales_start_date)(launch_date))
        };

        typedef eosio::singleton<"settings"_n, vesting_settings> settings_table;
        // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
        typedef eosio::multi_index<"settings"_n, vesting_settings> settings_table_dump;

        // Define the structure of a vesting schedule
        struct [[eosio::table]] vested_allocation
        {
            uint64_t id;
            eosio::name holder;
            eosio::asset tokens_allocated;
            eosio::asset tokens_claimed;
            microseconds time_since_sale_start;
            int vesting_category_type;
            uint64_t primary_key() const { return id; }
            EOSLIB_SERIALIZE(struct vested_allocation, (id)(holder)(tokens_allocated)(tokens_claimed)(time_since_sale_start)(vesting_category_type))
        };

        // Define the mapping of vesting schedules
        typedef eosio::multi_index<"allocation"_n, vested_allocation> vesting_allocations;

        /**
         * @details Updates the start date for vesting schedules to a new specified date
         *
         * @param sales_start_date {string} - The new start date for vesting schedules.
         * @param launch_date {string} - The new start date for vesting schedules.
         * @details
         * Before any allocations can be executed, the start date should be set using this action.
         * If the launch date is not known when the sale starts, set it to a long time in the future.
         *
         * Example of the string format expected: "2024-04-01T24:00:00"
         */
        [[eosio::action]] void setsettings(string sales_start_date, string launch_date);

        /**
         * @details Assigns tokens to a holder with a specified vesting category.
         *
         * @param sender {name} - The account name of the sender who is assigning the tokens.
         * @param holder {name} - The account name of the token holder.
         * @param amount {asset} - The amount of tokens to be assigned.
         * @param category {integer} - The vesting category for the assigned tokens.
         */
        [[eosio::action]] void assigntokens(eosio::name sender, eosio::name holder, eosio::asset amount, int category);

        /**
         * @details Allows a holder to withdraw vested tokens if the vesting conditions are met.
         *
         * @param holder {name} - The account name of the token holder.
         */
        [[eosio::action]] void withdraw(eosio::name holder);

        /**
         * @details Migrates an allocation to a new amount and category
         *
         * @internal Auth required by the contract
         *
         * @param sender {name} - The account name of the sender who created the allocation.
         * @param holder {name} - The account name of the token holder.
         * @param allocation_id {uint64_t} - The ID of the allocation to be migrated.
         * @param amount {asset} - The new amount of tokens to be assigned.
         * @param category_id {int} - The new vesting category for the assigned tokens.
         */
        [[eosio::action]] void migratealloc(eosio::name sender, name holder, uint64_t allocation_id, eosio::asset old_amount, eosio::asset new_amount, int old_category_id, int new_category_id);

        using setsettings_action = action_wrapper<"setsettings"_n, &vestingToken::setsettings>;
        using assigntokens_action = action_wrapper<"assigntokens"_n, &vestingToken::assigntokens>;
        using withdraw_action = action_wrapper<"withdraw"_n, &vestingToken::withdraw>;
        using migratealloc_action = action_wrapper<"migratealloc"_n, &vestingToken::migratealloc>;
    };
}
