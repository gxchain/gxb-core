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
#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>


#include <memory>
#include <vector>
#include <deque>
#include <cstdint>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/protocol/name.hpp>
#include <graphene/db/object_id.hpp>
#include <graphene/chain/protocol/config.hpp>

#define SECONDS_PER_DAY 86400 //seconds of a day

namespace graphene { namespace chain {
   using namespace graphene::db;

   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;

   using bytes              = std::vector<char>;

   struct void_t{};

   using action_name      = name;
   using scope_name       = name;
   using account_name     = uint64_t;
   using permission_name  = name;
   using table_name       = name;

//   using int128_t            = boost::multiprecision::int128_t;
   using int128_t            = __int128;

   using checksum256_type    = fc::sha256;
   using checksum512_type    = fc::sha512;
   using checksum160_type    = fc::ripemd160;

   typedef vector<std::pair<uint16_t,vector<char>>> abi_extensions_type;

   struct operation_ext_version_t {
       uint8_t version = 0;
   };

   struct data_transaction_commission_percent_t {
       // 10 percent is 1000
       uint16_t league_data_market_commission_percent = (10 * GRAPHENE_1_PERCENT);
       uint16_t free_data_market_commission_percent = (10 * GRAPHENE_1_PERCENT);

       string commission_account = GRAPHENE_DATA_TRANSACTION_COMMISSION_ACCOUNT;
       uint16_t reserve_percent = (1 * GRAPHENE_1_PERCENT); // reserve percent of commission, go to reserve pool
   };

   // vm execution cpu limit
   struct vm_cpu_limit_t {
       uint64_t trx_cpu_limit = 10000; // 10 ms
       uint64_t block_cpu_limit = 500000; // 500 ms
   };

   struct operation_ext_copyright_hash_t {
       fc::optional<string> copyright_hash;
   };

   struct pocs_threshold_league_t {
       // Trigger pocs thresholds
       vector<uint64_t> pocs_thresholds;
       // the base fee rate
       vector<uint64_t> fee_bases;
       // the product weight of the pocs
       vector<uint64_t> product_pocs_weights;
   };

   struct pocs_threshold_league_data_product_t{
       uint64_t pocs_threshold = 0;
   };

   struct interest_rate_t {
       uint32_t lock_days = 0;
       uint32_t interest_rate = 0;
       bool is_valid = false;//
   };

   struct lock_balance_params_t {
       vector< pair<fc::string, interest_rate_t> > params;
   };

   typedef fc::ecc::private_key        private_key_type;
   typedef fc::sha256 chain_id_type;

   enum asset_issuer_permission_flags
   {
      charge_market_fee    = 0x01, /**< an issuer-specified percentage of all market trades in this asset is paid to the issuer */
      white_list           = 0x02, /**< accounts must be whitelisted in order to hold this asset */
      override_authority   = 0x04, /**< issuer may transfer asset back to himself */
      transfer_restricted  = 0x08, /**< require the issuer to be one party to every transfer */
      disable_force_settle = 0x10, /**< disable force settling */
      global_settle        = 0x20, /**< allow the bitasset issuer to force a global settling -- this may be set in permissions, but not flags */
      disable_confidential = 0x40, /**< allow the asset to be used with confidential transactions */
      witness_fed_asset    = 0x80, /**< allow the asset to be fed by witnesses */
      committee_fed_asset  = 0x100 /**< allow the asset to be fed by the committee */
   };
   const static uint32_t ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_force_settle|global_settle|disable_confidential
      |witness_fed_asset|committee_fed_asset;
   const static uint32_t UIA_ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;

   enum reserved_spaces
   {
      relative_protocol_ids = 0,
      protocol_ids          = 1,
      implementation_ids    = 2
   };

   enum data_market_type_enum
   {
      free_data_market   = 1,//free_data_market
      league_data_market = 2//league_data_market
   };

   /**
    * @brief The data_market_category_status enum
    */
   enum data_market_category_status{
        data_market_category_undo_status = 0,// init
        data_market_category_enable_status  = 1,//enable, online
   };
   /**
    * @brief The free_data_product_status enum
    */
   enum data_product_status{
        data_product_undo_status = 0,//init
        data_product_enable_status  = 1,//enable, online
        data_product_pause_status = 2// disable
   };
   /**
    * @brief The league_status enum
    */
   enum league_status{
        league_undo_status = 0,//init
        league_enable_status  = 1,//enable, online
        league_pause_status = 2//disable
   };

   /**
    * @brief The league_data_product_status enum
    */
   enum league_datasource_status{
        league_datasource_undo_status = 0,//init
        league_datasource_enable_status  = 1,//enable, online
        league_datasource_pause_status = 2//disable
   };

   /**
    * @brief The data_transaction_status
    */
   enum data_transaction_status {
        data_transaction_status_init = 0,
        data_transaction_status_confirmed = 1,
        data_transaction_status_user_rejected = 99
   };

   enum data_transaction_datasource_status {
       data_transaction_datasource_status_init = 0,
       data_transaction_datasource_status_uploaded = 1,
       data_transaction_datasource_status_payed = 2,
       data_transaction_datasource_status_no_data = 3,
       data_transaction_datasource_status_error = 99
   };

   /**
    *  operation version
    */
   enum operation_version {
       operation_version_one = 1,
       operation_version_two = 2,
       operation_version_three = 3,
       operation_version_four = 4,
       operation_version_five = 5
   };

   inline bool is_relative( object_id_type o ){ return o.space() == 0; }

   /**
    *  List all object types from all namespaces here so they can
    *  be easily reflected and displayed in debug output.  If a 3rd party
    *  wants to extend the core code then they will have to change the
    *  packed_object::type field from enum_type to uint16 to avoid
    *  warnings when converting packed_objects to/from json.
    */
   enum object_type
   {
      null_object_type,//1.0.x
      base_object_type,//1
      account_object_type,//2
      asset_object_type,//3
      force_settlement_object_type,//4
      committee_member_object_type,//5
      witness_object_type,//6
      limit_order_object_type,//7
      call_order_object_type,//8
      custom_object_type,//9
      proposal_object_type,//10
      operation_history_object_type,//11
      withdraw_permission_object_type,//12
      vesting_balance_object_type,//13
      worker_object_type,//14
      balance_object_type,//15
      data_market_category_object_type,//16
      free_data_product_object_type,//17
      league_data_product_object_type,//18
      league_object_type,//19
      data_transaction_object_type, //20
      pocs_object_type,//21
      datasource_copyright_object_type,//22
      second_hand_data_object_type,//23
      data_transaction_complain_object_type,//24
      lock_balance_object_type,//25
      index64_object_type,
      index128_object_type,
      index256_object_type,
      index_double_object_type,
      index_long_double_object_type,

      OBJECT_TYPE_COUNT /////< Sentry value which contains the number of different object types

   };

   enum impl_object_type
   {
      impl_global_property_object_type,//2.0.x
      impl_dynamic_global_property_object_type,//1
      impl_reserved0_object_type,      // formerly index_meta_object_type, TODO: delete me
      impl_asset_dynamic_data_type,//3
      impl_asset_bitasset_data_type,//4
      impl_account_balance_object_type,//5
      impl_account_statistics_object_type,//6
      impl_transaction_object_type,//7
      impl_block_summary_object_type,//8
      impl_account_transaction_history_object_type,//9
      impl_blinded_balance_object_type,//10
      impl_chain_property_object_type,//11
      impl_witness_schedule_object_type,//12
      impl_budget_record_object_type,//13
      impl_special_authority_object_type,//14
      impl_buyback_object_type,//15
      impl_fba_accumulator_object_type,//16
      impl_account_merchant_object_type,//17
      impl_free_data_product_search_results_object_type,//18
      impl_league_data_product_search_results_object_type,//19
      impl_league_search_results_object_type,//20
      impl_data_transaction_search_results_object_type,//21
      //impl_search_results_object_type
      impl_signature_object_type, //22
      impl_table_id_object_type, //23
      impl_key_value_object_type //24
   };

   //typedef fc::unsigned_int            object_id_type;
   //typedef uint64_t                    object_id_type;
   class account_object;
   class committee_member_object;
   class witness_object;
   class asset_object;
   class force_settlement_object;
   class limit_order_object;
   class call_order_object;
   class custom_object;
   class proposal_object;
   class operation_history_object;
   class withdraw_permission_object;
   class vesting_balance_object;
   class worker_object;
   class balance_object;
   class blinded_balance_object;
   class data_market_category_object;
   class free_data_product_object;
   class league_data_product_object;
   class league_object;
   class data_transaction_object;
   class data_transaction_search_results_object;
   class personal_auth_object;
   class pocs_object;
   class datasource_copyright_object;
   class second_hand_data_object;
   class data_transaction_complain_object;
   class lock_balance_object;

   typedef object_id< protocol_ids, account_object_type,            account_object>               account_id_type;
   typedef object_id< protocol_ids, asset_object_type,              asset_object>                 asset_id_type;
   typedef object_id< protocol_ids, force_settlement_object_type,   force_settlement_object>      force_settlement_id_type;
   typedef object_id< protocol_ids, committee_member_object_type,   committee_member_object>      committee_member_id_type;
   typedef object_id< protocol_ids, witness_object_type,            witness_object>               witness_id_type;
   typedef object_id< protocol_ids, limit_order_object_type,        limit_order_object>           limit_order_id_type;
   typedef object_id< protocol_ids, call_order_object_type,         call_order_object>            call_order_id_type;
   typedef object_id< protocol_ids, custom_object_type,             custom_object>                custom_id_type;
   typedef object_id< protocol_ids, proposal_object_type,           proposal_object>              proposal_id_type;
   typedef object_id< protocol_ids, operation_history_object_type,  operation_history_object>     operation_history_id_type;
   typedef object_id< protocol_ids, withdraw_permission_object_type,withdraw_permission_object>   withdraw_permission_id_type;
   typedef object_id< protocol_ids, vesting_balance_object_type,    vesting_balance_object>       vesting_balance_id_type;
   typedef object_id< protocol_ids, worker_object_type,             worker_object>                worker_id_type;
   typedef object_id< protocol_ids, balance_object_type,            balance_object>               balance_id_type;
   typedef object_id< protocol_ids, data_market_category_object_type,       data_market_category_object> data_market_category_id_type;
   typedef object_id< protocol_ids, free_data_product_object_type,       free_data_product_object>  free_data_product_id_type;
   typedef object_id< protocol_ids, league_data_product_object_type,       league_data_product_object>  league_data_product_id_type;
   typedef object_id< protocol_ids, league_object_type,       league_object>                        league_id_type;
   typedef object_id< protocol_ids, data_transaction_object_type,   data_transaction_object>         data_transaction_id_type;
   typedef object_id< protocol_ids, pocs_object_type, pocs_object>                                  pocs_id_type;
   typedef object_id< protocol_ids, datasource_copyright_object_type, datasource_copyright_object>      datasource_copyright_id_type;
   typedef object_id< protocol_ids, second_hand_data_object_type, second_hand_data_object>         second_hand_data_id_type;
   typedef object_id< protocol_ids, data_transaction_complain_object_type, data_transaction_complain_object> data_transaction_complain_id_type;
   typedef object_id< protocol_ids, lock_balance_object_type, lock_balance_object>       lock_balance_id_type;

   // implementation types
   class global_property_object;
   class dynamic_global_property_object;
   class asset_dynamic_data_object;
   class asset_bitasset_data_object;
   class account_balance_object;
   class account_statistics_object;
   class transaction_object;
   class block_summary_object;
   class account_transaction_history_object;
   class chain_property_object;
   class witness_schedule_object;
   class budget_record_object;
   class special_authority_object;
   class buyback_object;
   class fba_accumulator_object;
   class account_merchant_object;//账户下的商户对象
   class free_data_product_search_results_object;
   class league_data_product_search_results_object;
   class league_search_results_object;
   class signature_object;
   class table_id_object;
   class key_value_object;

   typedef object_id< implementation_ids, impl_global_property_object_type,  global_property_object>                    global_property_id_type;
   typedef object_id< implementation_ids, impl_dynamic_global_property_object_type,  dynamic_global_property_object>    dynamic_global_property_id_type;
   typedef object_id< implementation_ids, impl_asset_dynamic_data_type,      asset_dynamic_data_object>                 asset_dynamic_data_id_type;
   typedef object_id< implementation_ids, impl_asset_bitasset_data_type,     asset_bitasset_data_object>                asset_bitasset_data_id_type;
   typedef object_id< implementation_ids, impl_account_balance_object_type,  account_balance_object>                    account_balance_id_type;
   typedef object_id< implementation_ids, impl_account_statistics_object_type,account_statistics_object>                account_statistics_id_type;
   typedef object_id< implementation_ids, impl_transaction_object_type,      transaction_object>                        transaction_obj_id_type;
   typedef object_id< implementation_ids, impl_block_summary_object_type,    block_summary_object>                      block_summary_id_type;

   typedef object_id< implementation_ids, impl_account_transaction_history_object_type,account_transaction_history_object>       account_transaction_history_id_type;
   typedef object_id< implementation_ids, impl_chain_property_object_type,   chain_property_object>                     chain_property_id_type;
   typedef object_id< implementation_ids, impl_witness_schedule_object_type, witness_schedule_object>                   witness_schedule_id_type;
   typedef object_id< implementation_ids, impl_budget_record_object_type, budget_record_object >                        budget_record_id_type;
   typedef object_id< implementation_ids, impl_blinded_balance_object_type, blinded_balance_object >                    blinded_balance_id_type;
   typedef object_id< implementation_ids, impl_special_authority_object_type, special_authority_object >                special_authority_id_type;
   typedef object_id< implementation_ids, impl_buyback_object_type, buyback_object >                                    buyback_id_type;
   typedef object_id< implementation_ids, impl_fba_accumulator_object_type, fba_accumulator_object >                    fba_accumulator_id_type;
   typedef object_id< implementation_ids, impl_account_merchant_object_type, account_merchant_object >                  account_merchant_id_type;
   typedef object_id< implementation_ids, impl_free_data_product_search_results_object_type, free_data_product_search_results_object >   free_data_product_search_results_id_type;
   typedef object_id< implementation_ids, impl_league_data_product_search_results_object_type, league_data_product_search_results_object >   league_data_product_search_results_id_type;
   typedef object_id< implementation_ids, impl_league_search_results_object_type, league_search_results_object >        league_search_results_id_type;
   typedef object_id< implementation_ids, impl_data_transaction_search_results_object_type, data_transaction_search_results_object >        data_transaction_search_results_id_type;
   typedef object_id< implementation_ids, impl_signature_object_type, signature_object>      signature_id_type;
   typedef object_id< implementation_ids, impl_table_id_object_type, table_id_object>        table_id_object_id_type;
   typedef object_id< implementation_ids, impl_key_value_object_type, key_value_object>      key_value_object_id_type;


   //typedef object_id< implementation_ids, impl_search_results_object_type,search_results_object<DerivedClass>>          search_results_id_type;

   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef safe<int64_t>                                        share_type;
   typedef uint16_t                                             weight_type;

   struct public_key_type
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key_type();
       public_key_type( const fc::ecc::public_key_data& data );
       public_key_type( const fc::ecc::public_key& pubkey );
       explicit public_key_type( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
       // TODO: This is temporary for testing
       bool is_valid_v1( const std::string& base58str );
   };

   struct extended_public_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_public_key_type();
      extended_public_key_type( const fc::ecc::extended_key_data& data );
      extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
      explicit extended_public_key_type( const std::string& base58str );
      operator fc::ecc::extended_public_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
      friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
      friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
   };

   struct extended_private_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_private_key_type();
      extended_private_key_type( const fc::ecc::extended_key_data& data );
      extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
      explicit extended_private_key_type( const std::string& base58str );
      operator fc::ecc::extended_private_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
      friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
      friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
   };

} }  // graphene::chain

namespace fc
{
    void to_variant( const graphene::chain::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  graphene::chain::public_key_type& vo );
    void to_variant( const graphene::chain::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_public_key_type& vo );
    void to_variant( const graphene::chain::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_private_key_type& vo );
}

FC_REFLECT( graphene::chain::public_key_type, (key_data) )
FC_REFLECT( graphene::chain::public_key_type::binary_key, (data)(check) )
FC_REFLECT( graphene::chain::extended_public_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( graphene::chain::extended_private_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_ENUM( graphene::chain::data_market_type_enum,
                 (free_data_market)
                 (league_data_market)
               )

FC_REFLECT_ENUM( graphene::chain::object_type,
                 (null_object_type)
                 (base_object_type)
                 (account_object_type)
                 (force_settlement_object_type)
                 (asset_object_type)
                 (committee_member_object_type)
                 (witness_object_type)
                 (limit_order_object_type)
                 (call_order_object_type)
                 (custom_object_type)
                 (proposal_object_type)
                 (operation_history_object_type)
                 (withdraw_permission_object_type)
                 (vesting_balance_object_type)
                 (worker_object_type)
                 (balance_object_type)
                 (data_market_category_object_type)
                 (free_data_product_object_type)
                 (league_data_product_object_type)
                 (league_object_type)
                 (data_transaction_object_type)
                 (pocs_object_type)
                 (datasource_copyright_object_type)
                 (second_hand_data_object_type)
                 (data_transaction_complain_object_type)
                 (lock_balance_object_type)
                 (index64_object_type)
                 (index128_object_type)
                 (index256_object_type)
                 (index_double_object_type)
                 (index_long_double_object_type)
                 (OBJECT_TYPE_COUNT)
               )
FC_REFLECT_ENUM( graphene::chain::impl_object_type,
                 (impl_global_property_object_type)
                 (impl_dynamic_global_property_object_type)
                 (impl_reserved0_object_type)
                 (impl_asset_dynamic_data_type)
                 (impl_asset_bitasset_data_type)
                 (impl_account_balance_object_type)
                 (impl_account_statistics_object_type)
                 (impl_transaction_object_type)
                 (impl_block_summary_object_type)
                 (impl_account_transaction_history_object_type)
                 (impl_blinded_balance_object_type)
                 (impl_chain_property_object_type)
                 (impl_witness_schedule_object_type)
                 (impl_budget_record_object_type)
                 (impl_special_authority_object_type)
                 (impl_buyback_object_type)
                 (impl_fba_accumulator_object_type)
                 (impl_account_merchant_object_type)
                 (impl_free_data_product_search_results_object_type)
                 (impl_league_data_product_search_results_object_type)
                 (impl_league_search_results_object_type)
                 (impl_data_transaction_search_results_object_type)
                 (impl_signature_object_type)
                 (impl_table_id_object_type)
                 (impl_key_value_object_type)
               )

FC_REFLECT_TYPENAME( graphene::chain::share_type )

FC_REFLECT_TYPENAME( graphene::chain::account_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_id_type )
FC_REFLECT_TYPENAME( graphene::chain::force_settlement_id_type )
FC_REFLECT_TYPENAME( graphene::chain::committee_member_id_type )
FC_REFLECT_TYPENAME( graphene::chain::witness_id_type )
FC_REFLECT_TYPENAME( graphene::chain::limit_order_id_type )
FC_REFLECT_TYPENAME( graphene::chain::call_order_id_type )
FC_REFLECT_TYPENAME( graphene::chain::custom_id_type )
FC_REFLECT_TYPENAME( graphene::chain::proposal_id_type )
FC_REFLECT_TYPENAME( graphene::chain::operation_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::withdraw_permission_id_type )
FC_REFLECT_TYPENAME( graphene::chain::vesting_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::worker_id_type )
FC_REFLECT_TYPENAME( graphene::chain::balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_dynamic_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_bitasset_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_statistics_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME( graphene::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_transaction_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::budget_record_id_type )
FC_REFLECT_TYPENAME( graphene::chain::special_authority_id_type )
FC_REFLECT_TYPENAME( graphene::chain::buyback_id_type )
FC_REFLECT_TYPENAME( graphene::chain::fba_accumulator_id_type )
FC_REFLECT_TYPENAME( graphene::chain::free_data_product_id_type )
FC_REFLECT_TYPENAME( graphene::chain::league_data_product_id_type)
FC_REFLECT_TYPENAME( graphene::chain::league_id_type )
FC_REFLECT_TYPENAME( graphene::chain::data_transaction_id_type )
FC_REFLECT_TYPENAME( graphene::chain::data_transaction_search_results_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_merchant_id_type )
FC_REFLECT_TYPENAME( graphene::chain::data_market_category_id_type )
FC_REFLECT_TYPENAME( graphene::chain::free_data_product_search_results_id_type )
FC_REFLECT_TYPENAME( graphene::chain::league_data_product_search_results_id_type )
FC_REFLECT_TYPENAME( graphene::chain::league_search_results_id_type )
FC_REFLECT_TYPENAME( graphene::chain::pocs_id_type)
FC_REFLECT_TYPENAME( graphene::chain::datasource_copyright_id_type)
FC_REFLECT_TYPENAME( graphene::chain::second_hand_data_id_type)
FC_REFLECT_TYPENAME( graphene::chain::data_transaction_complain_id_type)
FC_REFLECT_TYPENAME( graphene::chain::lock_balance_id_type)
FC_REFLECT_TYPENAME( graphene::chain::signature_id_type)
FC_REFLECT_TYPENAME( graphene::chain::table_id_object_id_type)
FC_REFLECT_TYPENAME( graphene::chain::key_value_object_id_type)

FC_REFLECT(graphene::chain::void_t, )
FC_REFLECT(graphene::chain::operation_ext_version_t, (version))
FC_REFLECT(graphene::chain::data_transaction_commission_percent_t, (league_data_market_commission_percent)(free_data_market_commission_percent)(commission_account)(reserve_percent))
FC_REFLECT(graphene::chain::operation_ext_copyright_hash_t, (copyright_hash))
FC_REFLECT(graphene::chain::pocs_threshold_league_t, (pocs_thresholds)(fee_bases)(product_pocs_weights))
FC_REFLECT(graphene::chain::pocs_threshold_league_data_product_t, (pocs_threshold))
FC_REFLECT(graphene::chain::interest_rate_t, (lock_days)(interest_rate)(is_valid))
FC_REFLECT(graphene::chain::lock_balance_params_t, (params))
FC_REFLECT(graphene::chain::vm_cpu_limit_t, (trx_cpu_limit)(block_cpu_limit))

FC_REFLECT_ENUM(graphene::chain::asset_issuer_permission_flags,
   (charge_market_fee)
   (white_list)
   (transfer_restricted)
   (override_authority)
   (disable_force_settle)
   (global_settle)
   (disable_confidential)
   (witness_fed_asset)
   (committee_fed_asset)
   )
