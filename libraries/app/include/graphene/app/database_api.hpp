/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/app/full_account.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/data_market_object.hpp>
#include <graphene/chain/data_transaction_object.hpp>
#include <graphene/app/database_api_common.hpp>
#include <graphene/chain/pocs_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <vector>

namespace graphene { namespace app {

using namespace graphene::chain;
using namespace std;

class database_api_impl;


/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(graphene::chain::database& db);
      ~database_api();

      /////////////
      // Objects //
      /////////////

      /**
       * @brief Get the objects corresponding to the provided IDs
       * @param ids IDs of the objects to retrieve
       * @return The objects retrieved, in the order they are mentioned in ids
       *
       * If any of the provided IDs does not map to an object, a null variant is returned in its position.
       */
      fc::variants get_objects(const vector<object_id_type>& ids)const;
      get_table_rows_result get_table_rows(string contract, string table, uint64_t start=0, uint64_t limit=10) const;
      fc::variants get_table_objects(uint64_t code, uint64_t scope, uint64_t table, uint64_t lower, uint64_t uppper, uint64_t limit) const;
      bytes serialize_contract_call_args(string contract, string method, string json_args) const;

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_data_transaction_subscribe_callback(std::function<void(const variant&)> cb, bool clear_filter);
      void set_data_transaction_products_subscribe_callback(std::function<void(const variant&)> cb, vector<object_id_type> ids);
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       */
      void cancel_all_subscriptions();
      void unsubscribe_data_transaction_callback();

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block_with_info> get_block(uint32_t block_num)const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_id of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block_with_info> get_block_by_id(block_id_type block_id)const;

      /**
       * @brief used to fetch an individual transaction.
       */
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      /**
       * If the transaction has not expired, this method will return the transaction for the given ID or
       * it will return NULL if it is not known.  Just because it is not known does not mean it wasn't
       * included in the blockchain.
       */
      optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id )const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve the @ref chain_property_object associated with the chain
       */
      chain_property_object get_chain_properties()const;

      /**
       * @brief Retrieve the current @ref global_property_object
       */
      global_property_object get_global_properties()const;


      /**
       *  @brief Retrieve data_transaction commission_percent
       */
      data_transaction_commission_percent_t get_commission_percent() const;

      /**
       *  @brief Retrieve vm cpu_limit
       */
      vm_cpu_limit_t get_cpu_limit() const;

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Get the chain ID
       */
      chain_id_type get_chain_id()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_object get_dynamic_global_properties()const;

      //////////
      // Keys //
      //////////

      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;

     /**
      * Determine whether a textual representation of a public key
      * (in Base-58 format) is *currently* linked
      * to any *registered* (i.e. non-stealth) account on the blockchain
      * @param public_key Public key
      * @return Whether a public key is known
      */
     bool is_public_key_registered(string public_key) const;

     /**
      * Determine whether an account_name is registered on the blockchain
      * @param name account_name
      * @return true if account_name is registered
      */
     bool is_account_registered(string name) const;

      //////////////
      // Accounts //
      //////////////

      /**
       * @brief Get a list of accounts by ID
       * @param account_ids IDs of the accounts to retrieve
       * @return The accounts corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;

      /**
       * @brief Fetch all objects relevant to the specified accounts and subscribe to updates
       * @param callback Function to call with updates
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Map of string from @ref names_or_ids to the corresponding account
       *
       * This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
       * accounts. If any of the strings in @ref names_or_ids cannot be tied to an account, that input will be
       * ignored. All other accounts will be retrieved and subscribed.
       *
       */
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

      optional<account_object> get_account_by_name( string name )const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references( account_id_type account_id )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      //////////////
      // Balances //
      //////////////

      /**
       * @brief Get an account's balances in various assets
       * @param id ID of the account to get balances for
       * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
       * @return Balances of the account
       */
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

      /**
       * @brief Get an account's lock balances in various assets
       * @param id ID of the account to get lock balances for
       * @param assets IDs of the assets to get lock balances of; if empty, get all assets account has a lock balance in
       * @return lock balances of the account
       */
      vector<asset> get_account_lock_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

      /// Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;

      /** @return all unclaimed balance objects for a set of addresses */
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;

      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;

      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      /**
       * @brief Get the total number of transactions of the blockchain
       */
      uint64_t get_transaction_count() const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      /**
       * @brief Get the total number of assets registered with the blockchain
       */
      uint64_t get_asset_count() const;

      /**
      * @brief get_data_transaction_product_costs
      * @param start
      * @param end
      * @return the number of the data transaction product costs during this time
      */
      uint64_t get_data_transaction_product_costs(fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_total_count
      * @param start
      * @param end
      * @return the number of the data transaction count during this time
      */
      uint64_t get_data_transaction_total_count(fc::time_point_sec start, fc::time_point_sec end) const;


      /**
      * @brief get_merchants_total_count
      * @return the total count of merchants
      */
      uint64_t get_merchants_total_count() const;

      /**
      * @brief get_data_transaction_commission
      * @param start
      * @param end
      * @return the number of the data transaction commission during this time
      */
      uint64_t get_data_transaction_commission(fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_pay_fee
      * @param start
      * @param end
      * @return the number of the data transaction pay fee during this time
      */
      uint64_t get_data_transaction_pay_fee(fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_product_costs_by_requester
      * @param requester
      * @param start
      * @param end
      * @return the number of the data transaction product costs of the requester during this time
      */
      uint64_t get_data_transaction_product_costs_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_total_count_by_requester
      * @param requester
      * @param start
      * @param end
      * @return the number of the data transaction product count of the requester during this time
      */
      uint64_t get_data_transaction_total_count_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_pay_fees_by_requester
      * @param requester
      * @param start
      * @param end
      * @return the number of the data transaction pay fees of the requester during this time
      */
      uint64_t get_data_transaction_pay_fees_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_pay_fees_by_requester
      * @param product_id
      * @param start
      * @param end
      * @return the number of the data transaction product costs of the product during this time
      */
      uint64_t get_data_transaction_product_costs_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const;

      /**
      * @brief get_data_transaction_total_count_by_product_id
      * @param product_id
      * @param start
      * @param end
      * @return the number of the data transaction total count of the product during this time
      */
      uint64_t get_data_transaction_total_count_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const;

      /** list_data_transaction_complain_requesters.
      *
      * @param start_date_time
      * @param end_date_time
      * @param limit
      * @return map<account_id_type, uint64_t>
      */
      map<account_id_type, uint64_t> list_data_transaction_complain_requesters(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const;
      
      /** list_data_transaction_complain_datasources.
      *
      * @param start_date_time
      * @param end_date_time
      * @param limit
      * @return map<account_id_type, uint64_t>
      */
      map<account_id_type, uint64_t> list_data_transaction_complain_datasources(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const;

      ////////////
      // Assets //
      ////////////

      /**
       * @brief Get a list of assets by ID
       * @param asset_ids IDs of the assets to retrieve
       * @return The assets corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;

      /**
       * @brief Get assets alphabetically by symbol name
       * @param lower_bound_symbol Lower bound of symbol names to retrieve
       * @param limit Maximum number of assets to fetch (must not exceed 100)
       * @return The assets found
       */
      vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

      /**
       * @brief Get a list of assets by symbol
       * @param asset_symbols Symbols or stringified IDs of the assets to retrieve
       * @return The assets corresponding to the provided symbols or IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      /** get pocs_object.
       *
       * @param league_id
       * @param account_id
       * @param product_id
       * @return pocs_object
       */
      optional<pocs_object> get_pocs_object(league_id_type league_id, account_id_type account_id, object_id_type product_id) const;


      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by ID
       * @param witness_ids IDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The ID of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional<witness_object> get_witness_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered witnesses
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of witness names to corresponding IDs
       */
      map<string, witness_id_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_witness_count()const;

      /**
       * @brief Get the total number of committee_members registered with the blockchain
       */
      uint64_t get_committee_member_count()const;

      /**
       * @brief Get a list of trust_nodes
       */
      vector<account_id_type> get_trust_nodes() const;

      ///////////////////////
      // Committee members //
      ///////////////////////

      /**
       * @brief Get a list of committee_members by ID
       * @param committee_member_ids IDs of the committee_members to retrieve
       * @return The committee_members corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<committee_member_object>> get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const;

      /**
       * @brief Get the committee_member owned by a given account
       * @param account The ID of the account whose committee_member should be retrieved
       * @return The committee_member object, or null if the account does not have a committee_member
       */
      fc::optional<committee_member_object> get_committee_member_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered committee_members
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of committee_member names to corresponding IDs
       */
      map<string, committee_member_id_type> lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const;


      /// WORKERS

      /**
       * Return the worker objects associated with this account.
       */
      vector<worker_object> get_workers_by_account(account_id_type account)const;


      ///////////
      // Votes //
      ///////////

      /**
       *  @brief Given a set of votes, return the objects they are voting for.
       *
       *  This will be a mixture of committee_member_object, witness_objects, and worker_objects
       *
       *  The results will be in the same order as the votes.  Null will be returned for
       *  any vote ids that are not found.
       */
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string get_transaction_hex(const signed_transaction& trx)const;

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string serialize_transaction(const signed_transaction& tx) const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       *  Validates a transaction against the current state without broadcasting it on the network.
       */
      processed_transaction validate_transaction( const signed_transaction& trx )const;

      /**
       *  For each operation calculate the required fee in the specified asset type.  If the asset type does
       *  not have a valid core_exchange_rate
       */
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      ///////////////////////////
      // Proposed transactions //
      ///////////////////////////

      /**
       *  @return the set of proposed transactions relevant to the specified account id.
       */
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      //////////////////////
      // Blinded balances //
      //////////////////////

      /**
       *  @return the set of blinded balance objects by commitment ID
       */
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;


      ////////////////////////////
      // Data Market Interface  //
      ////////////////////////////
      /**
       *
       * 这是一个测试用的函数
       * @see get_test()
       * @brief get_test
       * @return
       */
      uint64_t                          get_test() const;
    /**
     *
     * 获取首页的热门数据市场
     * @brief list_home_free_data_products
     * @param limit
     * @return
     */
    vector<free_data_product_object>  list_home_free_data_products(uint8_t limit) const;
    /**
     *
     * 获得首页的活跃联盟
     * @brief list_home_leagues
     * @param limit
     * @return
     */
    vector<league_object>  list_home_leagues(uint8_t limit) const;
    /**
     *
     * 获得某个数据市场的行业分类列表
     * @brief list_data_market_categories
     * @return
     */
    vector<data_market_category_object> list_data_market_categories(uint8_t data_market_type) const;

//    /**
//     *
//     * 获取推荐的自由市场数据产品
//     * @brief list_recommend_free_data_products
//     * @return
//     */
//    vector<free_data_product_object>  list_recommend_free_data_products(string product_id_or_league_id) const;
//    /**
//     *
//     * 获得推荐的联盟
//     * @brief list_recommend_leagues
//     * @return
//     */
//    vector<league_object>        list_recommend_leagues(string product_id_or_league_id) const;


    /**
     * @brief list_free_data_products
     * @param data_market_category_id
     * @param offset
     * @param limit
     * @param order_by
     * @param keyword
     * @param show_all
     * @return
     */
    free_data_product_search_results_object   list_free_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all = false) const;
    /**
     *
     *
     * @brief list_league_data_products
     * @param data_market_category_id
     * @param offset
     * @param limit
     * @param order_by
     * @param keyword
     * @param show_all
     * @return
     */
    league_data_product_search_results_object   list_league_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all = false) const;
    /**
     * @brief list_leagues
     * @param data_market_category_id
     * @param offset
     * @param limit
     * @param order_by
     * @param keyword
     * @param show_all
     * @return
     */
    league_search_results_object  list_leagues(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all = false) const;

    /**
     * @brief get_data_market_categories
     * @param data_market_category_ids
     * @return
     */
    vector<optional<data_market_category_object>> get_data_market_categories(const vector<data_market_category_id_type>& data_market_category_ids)const;
    /**
     * @brief get_free_data_products
     * @param product_ids
     * @return
     */
    vector<optional<free_data_product_object>> get_free_data_products(const vector<free_data_product_id_type>& product_ids)const;

    /**
     * @brief get_league_data_products
     * @param product_ids
     * @return
     */
    vector<optional<league_data_product_object>> get_league_data_products(const vector<league_data_product_id_type>& product_ids)const;


    /**
     * @brief get league by league_ids
     * @param league_ids
     * @return
     */
    vector<optional<league_object>> get_leagues(const vector<league_id_type>& league_ids) const;


    optional<data_transaction_object> get_data_transaction_by_request_id(string request_id) const;
    data_transaction_search_results_object list_data_transactions_by_requester(string requester, uint32_t limit) const;


    map<account_id_type, uint64_t> list_second_hand_datasources(time_point_sec start_date_time, time_point_sec end_date_time, uint32_t limit) const;
    uint32_t list_total_second_hand_transaction_counts_by_datasource(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, account_id_type datasource_account) const;

    /**
     * @brief get witness participation rate
     * @return uint32_t
     */   
    uint32_t get_witness_participation_rate() const;

   private:
      std::shared_ptr< database_api_impl > my;

};

} }

FC_API(graphene::app::database_api,
   // Objects
   (get_objects)
   (get_table_objects)
   (get_table_rows)
   (serialize_contract_call_args)
   (serialize_transaction)
   // Subscriptions
   (set_subscribe_callback)
   (set_data_transaction_subscribe_callback)
   (set_data_transaction_products_subscribe_callback)
   (set_pending_transaction_callback)
   (set_block_applied_callback)
   (cancel_all_subscriptions)
   (unsubscribe_data_transaction_callback)

   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_block_by_id)
   (get_transaction)
   (get_recent_transaction_by_id)

   // Globals
   (get_chain_properties)
   (get_global_properties)
   (get_commission_percent)
   (get_config)
   (get_chain_id)
   (get_dynamic_global_properties)

   // Keys
   (get_key_references)
   (is_public_key_registered)

   // Accounts
   (get_accounts)
   (get_full_accounts)
   (get_account_by_name)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_transaction_count)
   (get_account_count)
   (get_asset_count)
   (is_account_registered)

   // statistic
   (get_data_transaction_product_costs)
   (get_data_transaction_total_count)
   (get_merchants_total_count)
   (get_data_transaction_commission)
   (get_data_transaction_pay_fee)
   (get_data_transaction_product_costs_by_requester)
   (get_data_transaction_total_count_by_requester)
   (get_data_transaction_pay_fees_by_requester)
   (get_data_transaction_product_costs_by_product_id)
   (get_data_transaction_total_count_by_product_id)
   (list_data_transaction_complain_requesters)
   (list_data_transaction_complain_datasources)

   // Balances
   (get_account_balances)
   (get_account_lock_balances)
   (get_named_account_balances)
   (get_balance_objects)
   (get_vested_balances)
   (get_vesting_balances)

   // Assets
   (get_assets)
   (list_assets)
   (lookup_asset_symbols)

   // Markets / feeds
   (get_pocs_object)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_trust_nodes)

   // Committee members
   (get_committee_members)
   (get_committee_member_by_account)
   (lookup_committee_member_accounts)
   (get_committee_member_count)

   // workers
   (get_workers_by_account)
   // Votes
   (lookup_vote_ids)

   // Authority / validation
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (get_potential_address_signatures)
   (verify_authority)
   (verify_account_authority)
   (validate_transaction)
   (get_required_fees)
   // Proposed transactions
   (get_proposed_transactions)
   // Blinded balances
   (get_blinded_balances)
   // Data Market Interface
   (get_test)
   (list_home_free_data_products)
   (list_home_leagues)
   (list_data_market_categories)
   (list_free_data_products)
   (list_league_data_products)
   (list_leagues)
   (get_data_market_categories)
   (get_free_data_products)
   (get_league_data_products)
   (get_leagues)
   (get_data_transaction_by_request_id)
   (list_data_transactions_by_requester)
   (list_second_hand_datasources)
   (list_total_second_hand_transaction_counts_by_datasource)
   (get_witness_participation_rate)
   
)
