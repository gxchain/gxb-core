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

#include <graphene/app/database_api.hpp>
#include <graphene/app/database_api_impl.hpp>
#include <graphene/chain/get_config.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

namespace graphene { namespace app {
using namespace boost;

class database_api_impl;


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
{
   return my->get_objects( ids );
}

get_table_rows_result database_api::get_table_rows(string contract, string table, uint64_t start, uint64_t limit) const
{
    return my->get_table_rows(contract, table, start, limit);
}

fc::variants database_api::get_table_objects(uint64_t code, uint64_t scope, uint64_t table, uint64_t lower, uint64_t uppper, uint64_t limit) const
{
    return my->get_table_objects(code, scope, table, lower, uppper, limit);
}

bytes database_api::serialize_contract_call_args(string contract, string method, string json_args) const 
{
    return my->serialize_contract_call_args(contract, method, json_args);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback(std::function<void(const variant&)> cb, bool notify_remove_create)
{
   my->set_subscribe_callback(cb, notify_remove_create);
}

void database_api::set_data_transaction_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   my->set_data_transaction_subscribe_callback( cb, notify_remove_create );
}

void database_api::set_data_transaction_products_subscribe_callback(std::function<void(const variant&)> cb, vector<object_id_type> ids)
{
    my->set_data_transaction_products_subscribe_callback(cb, ids);
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api::unsubscribe_data_transaction_callback()
{
   my->unsubscribe_data_transaction_callback();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<signed_block_with_info> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block_with_info> database_api::get_block_by_id(block_id_type block_id)const
{
   return my->get_block_by_id( block_id );
}

processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
{
   return my->get_transaction( block_num, trx_in_block );
}

optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
{
   try {
      return my->_db.get_recent_transaction( id );
   } catch ( ... ) {
      return optional<signed_transaction>();
   }
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

chain_property_object database_api::get_chain_properties()const
{
   return my->get_chain_properties();
}

global_property_object database_api::get_global_properties()const
{
   return my->get_global_properties();
}

data_transaction_commission_percent_t database_api::get_commission_percent() const
{
  return my->get_commission_percent();
}

vm_cpu_limit_t database_api::get_cpu_limit() const
{
    return my->get_cpu_limit();
}

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

chain_id_type database_api::get_chain_id()const
{
   return my->get_chain_id();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}


bool database_api::is_public_key_registered(string public_key) const
{
    return my->is_public_key_registered(public_key);
}

bool database_api::is_account_registered(string name) const
{
    return my->is_account_registered(name);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
{
   return my->get_accounts( account_ids );
}

std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
{
   return my->get_full_accounts( names_or_ids, subscribe );
}

optional<account_object> database_api::get_account_by_name( string name )const
{
   return my->get_account_by_name( name );
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

uint64_t database_api::get_transaction_count() const
{
   return my->get_transaction_count();
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api::get_asset_count() const
{
   return my->get_asset_count();
}

map<account_id_type, uint64_t> database_api::list_data_transaction_complain_requesters(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const
{
    return my->list_data_transaction_complain_requesters(start_date_time, end_date_time, limit);
}


map<account_id_type, uint64_t> database_api::list_data_transaction_complain_datasources(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const
{
    return my->list_data_transaction_complain_datasources(start_date_time, end_date_time, limit);
}

uint64_t database_api::get_data_transaction_product_costs(fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_product_costs(start, end);
}

uint64_t database_api::get_data_transaction_total_count(fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_total_count(start, end);
}

uint64_t database_api::get_merchants_total_count() const
{
    return my->get_merchants_total_count();
}

uint64_t database_api::get_data_transaction_commission(fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_commission(start, end);
}

uint64_t database_api::get_data_transaction_pay_fee(fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_pay_fee(start, end);
}

uint64_t database_api::get_data_transaction_product_costs_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_product_costs_by_requester(requester, start, end);
}

uint64_t database_api::get_data_transaction_total_count_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_total_count_by_requester(requester, start, end);
}

uint64_t database_api::get_data_transaction_pay_fees_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_pay_fees_by_requester(requester, start, end);
}

uint64_t database_api::get_data_transaction_product_costs_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_product_costs_by_product_id(product_id, start, end);
}

uint64_t database_api::get_data_transaction_total_count_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const
{
   return my->get_data_transaction_total_count_by_product_id(product_id, start, end);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Balances                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
{
   return my->get_account_balances( id, assets );
}

vector<asset> database_api::get_account_lock_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
{
   return my->get_account_lock_balances( id, assets );
}

vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
{
   return my->get_named_account_balances( name, assets );
}

vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}


vector<asset> database_api::get_vested_balances( const vector<balance_id_type>& objs )const
{
   return my->get_vested_balances( objs );
}

vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
{
   return my->get_vesting_balances( account_id );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
{
   return my->get_assets( asset_ids );
}


vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets( lower_bound_symbol, limit );
}


vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols( symbols_or_ids );
}

optional<pocs_object> database_api::get_pocs_object(league_id_type league_id, account_id_type account_id, object_id_type product_id)const
{
    return my->get_pocs_object(league_id, account_id, product_id);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   return my->get_witnesses( witness_ids );
}

vector<worker_object> database_api::get_workers_by_account(account_id_type account)const
{
    const auto& idx = my->_db.get_index_type<worker_index>().indices().get<by_account>();
    auto itr = idx.find(account);
    vector<worker_object> result;

    if( itr != idx.end() && itr->worker_account == account )
    {
       result.emplace_back( *itr );
       ++itr;
    }

    return result;
}

fc::optional<witness_object> database_api::get_witness_by_account(account_id_type account)const
{
   return my->get_witness_by_account( account );
}

map<string, witness_id_type> database_api::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_witness_accounts( lower_bound_name, limit );
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api::get_committee_member_count() const
{
   return my->get_committee_member_count();
}

vector<account_id_type> database_api::get_trust_nodes() const
{
    return my->get_trust_nodes();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Committee members                                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<committee_member_object>> database_api::get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const
{
   return my->get_committee_members( committee_member_ids );
}


fc::optional<committee_member_object> database_api::get_committee_member_by_account(account_id_type account)const
{
   return my->get_committee_member_by_account( account );
}

map<string, committee_member_id_type> database_api::lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_committee_member_accounts( lower_bound_name, limit );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Votes                                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<variant> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   return my->lookup_vote_ids( votes );
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api::serialize_transaction(const signed_transaction& trx) const
{
    return my->serialize_transaction(trx);
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}
set<address> database_api::get_potential_address_signatures( const signed_transaction& trx )const
{
   return my->get_potential_address_signatures( trx );
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

processed_transaction database_api::validate_transaction( const signed_transaction& trx )const
{
   return my->validate_transaction( trx );
}

vector< fc::variant > database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   return my->get_required_fees( ops, id );
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
{
   return my->get_proposed_transactions( id );
}

/** TODO: add secondary index that will accelerate this process */

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blinded balances                                                 //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<blinded_balance_object> database_api::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   return my->get_blinded_balances( commitments );
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Private methods                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////


uint64_t database_api::get_test( ) const
{
    return my->get_test();
}

vector<free_data_product_object> database_api::list_home_free_data_products(uint8_t limit) const {
    return my->list_home_free_data_products(limit);
}


vector<league_object> database_api::list_home_leagues(uint8_t limit) const {
    return my->list_home_leagues(limit);
}

vector<data_market_category_object> database_api::list_data_market_categories(uint8_t data_market_type) const{
    return my->list_data_market_categories(data_market_type);
}


/**
 * @brief database_api::list_free_data_products
 * @param data_market_category_id
 * @param offset
 * @param limit
 * @param order_by
 * @param keyword
 * @return
 */
free_data_product_search_results_object   database_api::list_free_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const{
    return my->list_free_data_products(data_market_category_id, offset, limit,order_by,keyword,show_all);
}

/**
 * @brief database_api::list_league_data_products
 * @param data_market_category_id
 * @param offset
 * @param limit
 * @param order_by
 * @param keyword
 * @return
 */
league_data_product_search_results_object   database_api::list_league_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const{
    return my->list_league_data_products(data_market_category_id, offset, limit,order_by,keyword,show_all);
}

/**
 * @brief database_api::list_leagues
 * @param data_market_category_id
 * @param offset
 * @param limit
 * @param order_by
 * @param keyword
 * @return
 */
league_search_results_object database_api::list_leagues(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const {
    return my->list_leagues(data_market_category_id, offset, limit,order_by,keyword,show_all);
}

/**
 * @brief database_api::get_data_transaction_by_request_id
 * @param request_id
 * @return
 */
optional<data_transaction_object> database_api::get_data_transaction_by_request_id(string request_id) const {
    return my->get_data_transaction_by_request_id(request_id);
}

/**
* @brief get data_transaction_object by request_id
* @param request_id
* @return
*/
data_transaction_search_results_object database_api::list_data_transactions_by_requester(string requester, uint32_t limit) const {
    return my->list_data_transactions_by_requester(requester, limit);
}

map<account_id_type, uint64_t> database_api::list_second_hand_datasources(time_point_sec start_date_time, time_point_sec end_date_time, uint32_t limit) const {
    return my->list_second_hand_datasources(start_date_time, end_date_time, limit);
}

uint32_t database_api::list_total_second_hand_transaction_counts_by_datasource(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, account_id_type datasource_account) const {
    return my->list_total_second_hand_transaction_counts_by_datasource(start_date_time, end_date_time, datasource_account);
}

vector<optional<data_market_category_object>> database_api::get_data_market_categories(const vector<data_market_category_id_type>& data_market_category_ids)const{
   return my->get_data_market_categories(data_market_category_ids);

}
vector<optional<free_data_product_object>> database_api::get_free_data_products(const vector<free_data_product_id_type>& free_data_product_ids)const{
   return my->get_free_data_products(free_data_product_ids);
}

vector<optional<league_data_product_object>> database_api::get_league_data_products(const vector<league_data_product_id_type>& league_data_product_ids)const{
    return my->get_league_data_products(league_data_product_ids);
}

vector<optional<league_object>> database_api::get_leagues(const vector<league_id_type>& league_ids) const {
   return my->get_leagues(league_ids);
}

uint32_t database_api::get_witness_participation_rate() const
{
    return my->get_witness_participation_rate();
}

} } // graphene::app
