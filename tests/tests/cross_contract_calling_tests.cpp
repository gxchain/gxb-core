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
#include <graphene/chain/abi_serializer.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <fc/io/json.hpp>

#include "../common/database_fixture.hpp"
#include "test_wasts.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( inter_contract_calling_tests, database_fixture )

BOOST_AUTO_TEST_CASE( inter_contract_call )
{ try {
    ACTOR(alice);
    create_account("ram-account", alice_public_key);

    create_user_issued_asset("GXS", alice_id(db), 0, 5);
    issue_uia(alice_id, asset{1000000, asset_id_type(1)});
    generate_blocks(HARDFORK_1008_TIME, true, ~0);
    generate_block();

    contract_deploy_operation deploy_op;
    deploy_op.account = alice_id;
    deploy_op.vm_type = "0";
    deploy_op.vm_version = "0";
    deploy_op.fee = asset(2000, asset_id_type(1));

    deploy_op.name = "a";
    auto wasm = graphene::chain::wast_to_wasm(contracta_wast_code);
    deploy_op.code = bytes(wasm.begin(), wasm.end());
    deploy_op.abi = fc::json::from_string(contracta_abi).as<abi_def>(GRAPHENE_MAX_NESTED_OBJECTS);

    trx.operations.push_back(deploy_op);

    deploy_op.name = "b";
    wasm = graphene::chain::wast_to_wasm(contractb_wast_code);
    deploy_op.code = bytes(wasm.begin(), wasm.end());
    deploy_op.abi = fc::json::from_string(contractb_abi).as<abi_def>(GRAPHENE_MAX_NESTED_OBJECTS);

    trx.operations.push_back(deploy_op);

    deploy_op.name = "c";
    wasm = graphene::chain::wast_to_wasm(contractc_wast_code);
    deploy_op.code = bytes(wasm.begin(), wasm.end());
    deploy_op.abi = fc::json::from_string(contractc_abi).as<abi_def>(GRAPHENE_MAX_NESTED_OBJECTS);

    trx.operations.push_back(deploy_op);

    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();

    generate_block();
    const auto &contracta = get_account("a");
    const auto &contractb = get_account("b");
    const auto &contractc = get_account("c");
    issue_uia(contracta, asset{100000, asset_id_type(1)});
    issue_uia(contractb, asset{100000, asset_id_type(1)});
    issue_uia(contractc, asset{100000, asset_id_type(1)});
    generate_block();

    BOOST_REQUIRE_EQUAL(get_balance(contracta, asset_id_type(1)(db)), 100000);
    BOOST_REQUIRE_EQUAL(get_balance(contractb, asset_id_type(1)(db)), 100000);
    BOOST_REQUIRE_EQUAL(get_balance(contractc, asset_id_type(1)(db)), 100000);

    generate_blocks(HARDFORK_1016_TIME, true, ~0);
    generate_block();

    db.set_contract_log_to_console(true);
    contract_call_operation op;
    op.account = alice_id;
    op.contract_id = contracta.id;
    op.method_name = N(common);
    op.data = bytes();
    op.fee = asset{5000, asset_id_type(1)};
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();
    generate_block();

    //callself testcase
    BOOST_TEST_MESSAGE("++++++++ callself testcase ++++++++");
    op.method_name = N(callself);
    fc::variant action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cnt\":0}");
    graphene::chain::abi_serializer abis(contracta.abi, fc::milliseconds(1000000));
    auto action_type = abis.get_action_type("callself");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    BOOST_REQUIRE_THROW(db.push_transaction(trx, ~0), assert_exception);
    trx.clear();
    BOOST_TEST_MESSAGE("-------- callself testcase --------");

    //circle testcase
    BOOST_TEST_MESSAGE("++++++++ circle testcase ++++++++");
    op.method_name = N(circle);
    action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\"}");
    action_type = abis.get_action_type("circle");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    BOOST_REQUIRE_THROW(db.push_transaction(trx, ~0), assert_exception);
    trx.clear();
    BOOST_TEST_MESSAGE("-------- circle testcase --------");

    //senderpass testcase
    BOOST_TEST_MESSAGE("++++++++ senderpass testcase ++++++++");
    op.method_name = N(senderpass);
    action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\", \"cnt\":0}");
    action_type = abis.get_action_type("senderpass");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();
    generate_block();
    BOOST_TEST_MESSAGE("-------- senderpass testcase --------");

    //senderfail testcase
    BOOST_TEST_MESSAGE("++++++++ senderfail testcase ++++++++");
    op.method_name = N(senderfail);
    action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\", \"cnt\":0}");
    action_type = abis.get_action_type("senderfail");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    BOOST_REQUIRE_THROW(db.push_transaction(trx, ~0), assert_exception);
    trx.clear();
    BOOST_TEST_MESSAGE("-------- senderfail testcase --------");

    //receiverpass testcase
    BOOST_TEST_MESSAGE("++++++++ receiverpass testcase ++++++++");
    op.method_name = N(receiverpass);
    action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\", \"cnt\":0}");
    action_type = abis.get_action_type("receiverpass");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();
    generate_block();
    BOOST_TEST_MESSAGE("-------- receiverpass testcase --------");

    //originpass testcase
    BOOST_TEST_MESSAGE("++++++++ originpass testcase ++++++++");
    op.method_name = N(originpass);
    action_args_var = fc::json::from_string("{\"originacc\":\"alice\",\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\", \"cnt\":0}");
    action_type = abis.get_action_type("originpass");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();
    generate_block();
    BOOST_TEST_MESSAGE("-------- originpass testcase --------");

    //minustrans testcase
    BOOST_TEST_MESSAGE("++++++++ minustrans testcase ++++++++");
    op.method_name = N(minustrans);
    action_args_var = fc::json::from_string("{\"cccb\":\"b\"}");
    action_type = abis.get_action_type("minustrans");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);
    set_expiration(db, trx);
    sign(trx, alice_private_key);
    BOOST_REQUIRE_THROW(db.push_transaction(trx, ~0), assert_exception);
    trx.clear();
    BOOST_TEST_MESSAGE("-------- minustrans testcase --------");

    //fee testcase1
    //ramadd(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool aadd, bool badd, bool cadd)
    //ramdel(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool adel, bool bdel, bool cdel, uint64_t pk)
    //ramdelall(const std::string &ccca, const std::string &cccb, const std::string &cccc)
    const auto &balancea = get_balance(contracta, asset_id_type(1)(db));
    const auto &balanceb = get_balance(contractb, asset_id_type(1)(db));
    const auto &balancec = get_balance(contractc, asset_id_type(1)(db));

    op.method_name = N(ramadd);
    action_args_var = fc::json::from_string("{\"ccca\":\"a\", \"cccb\":\"b\", \"cccc\":\"c\", \"aadd\":true, \"badd\":true, \"cadd\":true}");
    action_type = abis.get_action_type("ramadd");
    op.data = abis.variant_to_binary(action_type, action_args_var, fc::milliseconds(1000000));
    trx.operations.push_back(op);

    set_expiration(db, trx);
    sign(trx, alice_private_key);
    PUSH_TX(db, trx);
    trx.clear();
    generate_block();

    const auto &balancea1 = get_balance(contracta, asset_id_type(1)(db));
    const auto &balanceb1 = get_balance(contractb, asset_id_type(1)(db));
    const auto &balancec1 = get_balance(contractc, asset_id_type(1)(db));
    idump((db.fetch_block_by_number(14)));

    BOOST_REQUIRE_EQUAL(balancea - 11329, balancea1);
    BOOST_REQUIRE_EQUAL(balanceb - 11329, balanceb1);
    BOOST_REQUIRE_EQUAL(balancec - 11329, balancec1);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
