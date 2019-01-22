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

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <fc/io/json.hpp>

#include "../common/database_fixture.hpp"
#include "test_wasts.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( cross_contract_calling_tests, database_fixture )

BOOST_AUTO_TEST_CASE( cross_contract_call )
{ try {
    ACTOR(alice);

    transfer(account_id_type(), alice_id, asset(1000000));
    generate_block();

    contract_deploy_operation deploy_op;
    deploy_op.account = alice_id;
    deploy_op.name = "contracta";
    deploy_op.vm_type = "0";
    deploy_op.vm_version = "0";
    auto wasm = graphene::chain::wast_to_wasm(contracta_wast_code);
    deploy_op.code = bytes(wasm.begin(), wasm.end());
    deploy_op.abi = fc::json::from_string(contracta_abi).as<abi_def>(GRAPHENE_MAX_NESTED_OBJECTS);
    deploy_op.fee = asset(2000);
    trx.operations.push_back(deploy_op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();

    // call contract, action hi
    auto& contract_obj = get_account("bob");
    string s = "123";

    contract_call_operation op;
    op.account = alice_id;
    op.contract_id = contract_obj.id;
    op.method_name = N(hi);
    op.data = bytes(s.begin(), s.end());
    op.fee = db.get_global_properties().parameters.current_fees->calculate_fee(op);
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    idump((trx));
    PUSH_TX(db, trx);
    trx.clear();

    // call contract, action hi, deposit asset
    contract_call_operation call_op;
    call_op.account = alice_id;
    call_op.account = alice_id;
    call_op.contract_id = contract_obj.id;
    call_op.amount = share_type(100);
    call_op.method_name = N(hi);
    call_op.data = bytes(s.begin(), s.end());
    call_op.fee = db.get_global_properties().parameters.current_fees->calculate_fee(call_op);
    trx.operations.push_back(call_op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    idump((trx));
    PUSH_TX(db, trx);
    BOOST_REQUIRE_EQUAL(get_balance(account_id_type(contract_obj.id)(db), asset_id_type()(db)), 100);
    trx.clear();
} FC_LOG_AND_RETHROW() }

//test create witness after hardfork_1129_time failing for not enough GXS
BOOST_AUTO_TEST_CASE( witness_create1 )
{ try {
   //prepare
   ACTOR(wit1);
   ACTOR(wit2);
   create_user_issued_asset("GXS", wit2_id(db), 0, 5);
   upgrade_to_lifetime_member(wit1_id);
   upgrade_to_lifetime_member(wit2_id);

   //test create witness before hardfork_1129_time
   create_witness(wit1_id, wit1_private_key);

   //test create witness after hardfork_1129_time failing for not enough GXS
   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   witness_create_operation op;
   op.witness_account = wit2_id;
   op.block_signing_key = wit2_private_key.get_public_key();
   trx.operations.push_back( op );
   update_operation_fee(trx);
   sign(trx, wit2_private_key);
   set_expiration( db, trx );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, ~0 ), fc::exception );
} FC_LOG_AND_RETHROW() }

//test create witness after hardfork_1129_time success
BOOST_AUTO_TEST_CASE( witness_create2 )
{ try {
   //prepare
   ACTOR(wit2);
   asset_id_type gxs_id = create_user_issued_asset("GXS", wit2_id(db), 0, 5).id;
   upgrade_to_lifetime_member(wit2_id);

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration( db, trx );
   issue_uia( wit2_id(db), asset( (uint64_t)100000*1000000, gxs_id ) );
   witness_id_type wit2_witness_id = create_witness(wit2_id(db), wit2_private_key).id;

   {
      account_update_operation op;
      op.account = wit2_id;
      op.new_options = wit2_id(db).options;
      op.new_options->votes.insert(wit2_witness_id(db).vote_id);
      op.new_options->num_witness = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                  [](vote_id_type id) { return id.type() == vote_id_type::witness; });
      op.new_options->num_committee = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                    [](vote_id_type id) { return id.type() == vote_id_type::committee; });
      trx.operations.push_back(op);
      update_operation_fee(trx);
      sign( trx, wit2_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   const auto& witnesses = db.get_global_properties().active_witnesses;

   // make sure we're in active_witnesses
   auto itr = std::find(witnesses.begin(), witnesses.end(), wit2_witness_id);
   BOOST_CHECK(itr != witnesses.end());

   // generate blocks until we are at the beginning of a round
   while( ((db.get_dynamic_global_properties().current_aslot + 1) % witnesses.size()) != 0 )
      generate_block();

   int produced = 0;
   // Make sure we get scheduled at least once in witnesses.size()*2 blocks
   // may take this many unless we measure where in the scheduling round we are
   // TODO:  intense_test that repeats this loop many times
   for( size_t i=0, n=witnesses.size()*2; i<n; i++ )
   {
      signed_block block = generate_block();
      if( block.witness == wit2_witness_id )
         produced++;
   }
   BOOST_CHECK_GE( produced, 1 );

   BOOST_CHECK(wit2_witness_id(db).is_valid == true);
   BOOST_CHECK(wit2_witness_id(db).total_votes > 0);

   {
       trust_node_pledge_withdraw_operation op;
       op.witness_account = wit2_id;
       trx.operations.push_back( op );
       update_operation_fee(trx);
       set_expiration( db, trx );
       sign( trx, wit2_private_key );
       PUSH_TX( db, trx );
       trx.clear();
   }

   generate_block();
   BOOST_CHECK(wit2_witness_id(db).is_valid == false);
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   generate_block();
   BOOST_CHECK(wit2_witness_id(db).total_votes == 0);

   {
       witness_update_operation witness_update_op;
       witness_update_op.witness = wit2_witness_id;
       witness_update_op.witness_account = wit2_id;
       witness_update_op.new_url = "https://gxb.witness.org";
       trx.operations.push_back( witness_update_op );
       update_operation_fee(trx);
       set_expiration( db, trx );
       sign( trx, wit2_private_key );
       PUSH_TX( db, trx );
       trx.clear();
   }

   generate_block();
   BOOST_CHECK(wit2_witness_id(db).is_valid == true);


} FC_LOG_AND_RETHROW() }

// HARDFORK_1129_TIME前create_witness(账号余额小于10000GXC，创建成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(create_witness_before_hardfork1129_success)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   auto &witness_obj = create_witness(nathan_id, nathan_private_key);
   BOOST_CHECK(witness_obj.is_valid == true);
} FC_LOG_AND_RETHROW() }

// HARDFORK_1129_TIME前create_committee(账号余额小于10000GXC，创建成功，committee.is_valid=true)
BOOST_AUTO_TEST_CASE(create_committee_before_hardfork1129_success)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   auto &committee_obj = create_committee_member(nathan);
   BOOST_CHECK(committee_obj.is_valid == true);
} FC_LOG_AND_RETHROW() }

//HARDFORK_1129_TIME后create_witness需要扣除押金(账号余额30000GXC，创建成功，账号余额20000GXC，trust_node_pledge_object存在并且额度是10000GXC)
BOOST_AUTO_TEST_CASE(create_witness_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const auto &witness_obj = create_witness(nathan_id(db));
   BOOST_CHECK(witness_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

//HARDFORK_1129_TIME后create_committee需要扣除押金(账号余额30000GXC，创建成功，账号余额20000GXC，trust_node_pledge_object存在并且额度是10000GXC)
BOOST_AUTO_TEST_CASE(create_committee_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const committee_member_object& committee_obj = create_committee_member(nathan_id(db));
   BOOST_CHECK(committee_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(create_committee_and_witness_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const committee_member_object& committee_obj = create_committee_member(nathan_id(db));
   BOOST_CHECK(committee_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto &witness_obj = create_witness(nathan_id(db));
   BOOST_CHECK(witness_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(create_witness_and_committee_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const auto &witness_obj = create_witness(nathan_id(db));
   BOOST_CHECK(witness_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const committee_member_object& committee_obj = create_committee_member(nathan_id(db));
   BOOST_CHECK(committee_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

///////////////////////////create test finish//////////////////////////

///////////////////////////update test begin///////////////////////////
// case1: HARDFORK_1129_TIME前update_witness不需要补缴押金(账号余额0，create_witness成功，update_witness成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(update_witness_before_hardfork1129_success)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   witness_id_type nathan_witness_id = create_witness(nathan_id(db), nathan_private_key).id;
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);

   string new_url = "https://gxb.witness.org";
   update_witness(nathan_witness_id, nathan_id, nathan_private_key, new_url);
   BOOST_CHECK(nathan_witness_id(db).url == new_url);
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);
} FC_LOG_AND_RETHROW() }

// case2: HARDFORK_1129_TIME前update_committee不需要补缴押金(账号余额0，create_committee成功，update_committee成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(update_committee_before_hardfork1129_success)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   const committee_member_id_type &committee_id = create_committee_member(nathan_id(db)).id;
   BOOST_CHECK(committee_id(db).is_valid == true);

   string new_url = "https://gxb.witness.org";
   update_committee(committee_id, nathan_id, nathan_private_key, new_url);
   BOOST_CHECK(committee_id(db).url == new_url);
   BOOST_CHECK(committee_id(db).is_valid == true);
} FC_LOG_AND_RETHROW() }

// case3: HARDFORK_1129_TIME前create_witness，HARDFORK_1129_TIME后update_witness需要补缴押金(账号余额30000GXC，update成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(update_witness_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);

   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   witness_id_type nathan_witness_id = create_witness(nathan_id(db)).id;
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*30000);

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);

   string new_url = "https://gxb.witness.org";
   update_witness(nathan_witness_id, nathan_id, nathan_private_key, new_url);

   BOOST_CHECK(nathan_witness_id(db).url == new_url);
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

// case4: HARDFORK_1129_TIME前create_committee，HARDFORK_1129_TIME后update_committee需要补缴押金(账号余额30000，update成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(update_committee_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);

   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const committee_member_id_type &committee_id = create_committee_member(nathan_id(db)).id;
   BOOST_CHECK(committee_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*30000);

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);

   string new_url = "https://gxb.witness.org";
   update_committee(committee_id, nathan_id, nathan_private_key, new_url);

   BOOST_CHECK(committee_id(db).url == new_url);
   BOOST_CHECK(committee_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

// case5: HARDFORK_1129_TIME后create_witness，update_witness除手续费以外不会减少账号余额(账号余额为10000，create_witness成功，update_witness成功，witness.is_valid=true)
BOOST_AUTO_TEST_CASE(create_witness_update_witness_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);

   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);

   witness_id_type nathan_witness_id = create_witness(nathan_id(db)).id;
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   string new_url = "https://gxb.witness.org";
   update_witness(nathan_witness_id, nathan_id, nathan_private_key, new_url);
   BOOST_CHECK(nathan_witness_id(db).url == new_url);
   BOOST_CHECK(nathan_witness_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

// case6: HARDFORK_1129_TIME后create_committee，update_committee除手续费以外不会减少账号余额(账号余额为30000，create_committee成功，update_committee成功，committee.is_valid=true)
BOOST_AUTO_TEST_CASE(create_committee_update_committee_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);

   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);

   const committee_member_id_type &committee_id = create_committee_member(nathan_id(db)).id;
   BOOST_CHECK(committee_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   string new_url = "https://gxb.witness.org";
   update_committee(committee_id, nathan_id, nathan_private_key, new_url);
   BOOST_CHECK(committee_id(db).url == new_url);
   BOOST_CHECK(committee_id(db).is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

// case7: HARDFORK_1129_TIME后create_witness，全局参数调整成20000GXC，update_witness需要补缴(账号余额30000，create_witness成功，全局参数调整成20000GXC，update_witness成功)
//BOOST_AUTO_TEST_CASE(create_witness_change_parameter_update_witness_after_hardfork1129_test)
//{ try {
//   ACTOR(nathan);
//   upgrade_to_lifetime_member(nathan_id);
//   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
//
//   generate_blocks(HARDFORK_1129_TIME, true, ~0);
//   generate_block();
//
//   set_expiration(db, trx);
//   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));
//
//   const auto &witness_obj = create_witness(nathan_id(db));
//   BOOST_CHECK(witness_obj.is_valid == true);
//   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
//   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
//   auto pledge_itr = pledge_idx.find(nathan_id);
//   BOOST_ASSERT(pledge_itr != pledge_idx.end());
//   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
//
//   const char *parameter = "{\"extensions\":[[9,{\"amount\":2000000000}]]}";
//   fc::variant var = fc::json::from_string(parameter);
//   variant_object var_obj;
//   from_variant(var, var_obj, 10);
//
//   const chain_parameters& current_params = db.get_global_properties().parameters;
//   chain_parameters new_params = current_params;
//   fc::reflector<chain_parameters>::visit(fc::from_variant_visitor<chain_parameters>(var_obj, new_params, GRAPHENE_MAX_NESTED_OBJECTS));
//
//   committee_member_update_global_parameters_operation update_op;
//   update_op.fee.asset_id = asset_id_type(1);
//   update_op.new_parameters = new_params;
//
//   proposal_create_operation prop_op;
//   prop_op.expiration_time = time_point_sec(db.get_dynamic_global_properties().time.sec_since_epoch() + 60*200);
//   prop_op.review_period_seconds = 100;
//   prop_op.fee_paying_account = nathan_id;
//   prop_op.proposed_ops.emplace_back(update_op);
//   trx.operations.push_back(prop_op);
//   update_operation_fee(trx);
//   trx.validate();
//   PUSH_TX(db, trx, ~0);
//   trx.clear();
//   generate_block();
//
//   edump((db.get_global_properties()));
//   proposal_update_operation update_op1;
//   update_op1.fee_paying_account = nathan_id;
//   update_op1.proposal = proposal_id_type(0);
//   update_op1.active_approvals_to_add.insert(account_id_type(6));
//   update_op1.active_approvals_to_add.insert(account_id_type(7));
//   update_op1.active_approvals_to_add.insert(account_id_type(8));
//   update_op1.active_approvals_to_add.insert(account_id_type(9));
//   update_op1.active_approvals_to_add.insert(account_id_type(10));
//   update_op1.active_approvals_to_add.insert(account_id_type(11));
//   update_op1.active_approvals_to_add.insert(account_id_type(12));
//   update_op1.active_approvals_to_add.insert(account_id_type(13));
//   update_op1.active_approvals_to_add.insert(account_id_type(14));
//   update_op1.active_approvals_to_add.insert(account_id_type(15));
//
////   init_account_priv_key
//
//   trx.operations.push_back(update_op1);
//  edump((trx));
//   update_operation_fee(trx);
//   edump((trx));
//   trx.validate();
//   PUSH_TX(db, trx, ~0);
//   trx.clear();
//
//   generate_block();
//   edump((proposal_id_type(0)(db)));
//   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time +3600);
//   generate_block();
//   edump((db.get_global_properties().parameters));
//
//} FC_LOG_AND_RETHROW() }

// case7: HARDFORK_1129_TIME后create_witness，全局参数调整成20000GXC，update_witness需要补缴(账号余额30000，create_witness成功，全局参数调整成20000GXC，update_witness成功)
//BOOST_AUTO_TEST_CASE(create_witness_change_parameter_update_witness_after_hardfork1129_test)
//{ try {
//   ACTOR(nathan);
//   upgrade_to_lifetime_member(nathan_id);
//   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;
//
//   generate_blocks(HARDFORK_1129_TIME, true, ~0);
//   generate_block();
//
//   set_expiration(db, trx);
//   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));
//   edump((db.get_global_properties()));
//   const auto &witness_obj = create_witness(nathan_id(db));
//   BOOST_CHECK(witness_obj.is_valid == true);
//   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
//   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
//   auto pledge_itr = pledge_idx.find(nathan_id);
//   BOOST_ASSERT(pledge_itr != pledge_idx.end());
//   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
//
//   const char *parameter = "{\"extensions\":[[9,{\"amount\":2000000000}]]}";
//   fc::variant var = fc::json::from_string(parameter);
//   variant_object var_obj;
//   from_variant(var, var_obj, 10);
//
//   const chain_parameters& current_params = db.get_global_properties().parameters;
//   chain_parameters new_params = current_params;
//   fc::reflector<chain_parameters>::visit(fc::from_variant_visitor<chain_parameters>(var_obj, new_params, GRAPHENE_MAX_NESTED_OBJECTS));
//
//edump((new_params));
//db.modify(db.get_global_properties(), [&new_params,this](global_property_object &obj){
//obj.parameters =new_params;
//obj.parameters.extensions.emplace(trust_node_pledge_t());
//edump((db.get_global_properties()));
//});
//edump((db.get_global_properties()));
//generate_block();
//edump((db.get_global_properties()));
//
//} FC_LOG_AND_RETHROW() }

/////////////////////////////withdraw testcase begin///////////////
//case3: HARDFORK_1129_TIME后create_witness，调用提取能通过，并且返还10000GXC到cdd，witness and committee变成false(账号余额30000GXC，create_witness成功，提取，witness.is_valid=false，vb_object存在并且额度是10000GXC)
BOOST_AUTO_TEST_CASE(create_witness_withdraw_pledge_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const auto &witness_obj = create_witness(nathan_id(db));
   BOOST_CHECK(witness_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));

   trust_node_pledge_withdraw_operation op;
   op.witness_account = nathan_id;
   trx.operations.push_back(op);
   update_operation_fee(trx);
   trx.validate();
   PUSH_TX(db, trx, ~0);
   trx.clear();
   generate_block();

   BOOST_CHECK(witness_obj.is_valid == false);
   auto &vb_index = db.get_index_type< vesting_balance_index >().indices().get<by_account>();
   const auto &vb_itr = vb_index.find(nathan_id);
   BOOST_ASSERT(vb_itr != vb_index.end());
   BOOST_CHECK(vb_itr->balance == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));

} FC_LOG_AND_RETHROW() }
// case4: HARDFORK_1129_TIME后create_committee，调用提取能通过，并且返还10000GXC到cdd，witness and committee变成false(账号余额30000GXC，create_committee成功，提取，committee.is_valid=false，vb_object存在并且额度是10000GXC)
BOOST_AUTO_TEST_CASE(create_committee_withdraw_pledge_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const committee_member_object& committee_obj = create_committee_member(nathan_id(db));
   BOOST_CHECK(committee_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);
   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));

   trust_node_pledge_withdraw_operation op;
   op.witness_account = nathan_id;
   trx.operations.push_back(op);
   update_operation_fee(trx);
   trx.validate();
   PUSH_TX(db, trx, ~0);
   trx.clear();
   generate_block();

   BOOST_CHECK(committee_obj.is_valid == false);
   auto &vb_index = db.get_index_type< vesting_balance_index >().indices().get<by_account>();
   const auto &vb_itr = vb_index.find(nathan_id);
   BOOST_ASSERT(vb_itr != vb_index.end());
   BOOST_CHECK(vb_itr->balance == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));
} FC_LOG_AND_RETHROW() }

// case5: HARDFORK_1129_TIME后create_committee and create_committee，调用提取能通过，并且返还10000GXC到cdd，witness and committee变成false(账号余额30000GXC，create_committee成功，create_committee成功，提取，witness.is_valid=false，committee.is_valid=false，vb_object存在并且额度是10000GXC)
BOOST_AUTO_TEST_CASE(create_witness_create_committee_withdraw_pledge_after_hardfork1129_test)
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   asset_id_type gxs_id = create_user_issued_asset("GXS", nathan_id(db), 0, GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS).id;

   generate_blocks(HARDFORK_1129_TIME, true, ~0);
   generate_block();

   set_expiration(db, trx);
   issue_uia(nathan_id(db), asset((uint64_t)GRAPHENE_BLOCKCHAIN_PRECISION*30000, gxs_id));

   const auto &witness_obj = create_witness(nathan_id(db));
   BOOST_CHECK(witness_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const committee_member_object& committee_obj = create_committee_member(nathan_id(db));
   BOOST_CHECK(committee_obj.is_valid == true);
   BOOST_CHECK(get_balance(nathan_id, asset_id_type(1)) == GRAPHENE_BLOCKCHAIN_PRECISION*20000);

   const auto& pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
   auto pledge_itr = pledge_idx.find(nathan_id);
   BOOST_ASSERT(pledge_itr != pledge_idx.end());
   BOOST_CHECK(pledge_itr->amount == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));

   trust_node_pledge_withdraw_operation op;
   op.witness_account = nathan_id;
   trx.operations.push_back(op);
   update_operation_fee(trx);
   trx.validate();
   PUSH_TX(db, trx, ~0);
   trx.clear();
   generate_block();

   BOOST_CHECK(witness_obj.is_valid == false);
   BOOST_CHECK(committee_obj.is_valid == false);
   auto &vb_index = db.get_index_type< vesting_balance_index >().indices().get<by_account>();
   const auto &vb_itr = vb_index.find(nathan_id);
   BOOST_ASSERT(vb_itr != vb_index.end());
   BOOST_CHECK(vb_itr->balance == asset(GRAPHENE_BLOCKCHAIN_PRECISION*10000, asset_id_type(1)));

} FC_LOG_AND_RETHROW() }
BOOST_AUTO_TEST_SUITE_END()
