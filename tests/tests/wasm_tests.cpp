#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/abi_def.hpp>

#include "../common/database_fixture.hpp"
#include "test_wasts.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(wast_tests, database_fixture)

BOOST_AUTO_TEST_CASE(nested_limit_test)
{ try {
   ACTOR(alice);
   transfer(account_id_type(), alice_id, asset(10000000));
   generate_blocks(HARDFORK_1006_TIME);
   while (db.head_block_time() <= HARDFORK_1006_TIME) {
       generate_block();
   }

    // nested loops
   contract_deploy_operation op;
   op.account = alice_id;
   op.vm_type = "0";
   op.vm_version = "0";
   op.abi = fc::json::from_string(contract_abi).as<abi_def>(GRAPHENE_MAX_NESTED_OBJECTS);
   // nested loops
   {
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << "(loop (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << ")";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "valid-nested-loops";
      REQUIRE_OP_EVALUATION_SUCCESS(op, code, bytes(wasm.begin(), wasm.end()));
       BOOST_TEST_MESSAGE("testing valid-nested-loops");
   }
   {
      BOOST_TEST_MESSAGE("testing invalid-nested-loops");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << "(loop (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << ")";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "invalid-nested-loops";
      REQUIRE_THROW_WITH_VALUE(op, code, bytes(wasm.begin(), wasm.end()));
   }

   // nested blocks
   {
      BOOST_TEST_MESSAGE("testing valid-nested-blocks");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << "(block (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << ")";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "valid-nested-blocks";
      REQUIRE_OP_EVALUATION_SUCCESS(op, code, bytes(wasm.begin(), wasm.end()));
   }
   {
      BOOST_TEST_MESSAGE("testing invalid-nested-blocks");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << "(block (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << ")";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "invalid-nested-blocks";
      REQUIRE_THROW_WITH_VALUE(op, code, bytes(wasm.begin(), wasm.end()));
   }
   // nested ifs
   {
      BOOST_TEST_MESSAGE("testing valid-nested-ifs");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << "(if (i32.wrap/i64 (get_local $0)) (then (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1023; ++i)
         ss << "))";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "valid-nested-ifs";
      REQUIRE_OP_EVALUATION_SUCCESS(op, code, bytes(wasm.begin(), wasm.end()));
   }
   {
      BOOST_TEST_MESSAGE("testing invalid-nested-ifs");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << "(if (i32.wrap/i64 (get_local $0)) (then (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 1024; ++i)
         ss << "))";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "invalid-nested-ifs";
      REQUIRE_THROW_WITH_VALUE(op, code, bytes(wasm.begin(), wasm.end()));
   }
   // mixed nested
   {
      BOOST_TEST_MESSAGE("testing valid-mixed-nested");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 223; ++i)
         ss << "(if (i32.wrap/i64 (get_local $0)) (then (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 400; ++i)
         ss << "(block (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 400; ++i)
         ss << "(loop (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 800; ++i)
         ss << ")";
      for(unsigned int i = 0; i < 223; ++i)
         ss << "))";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "valid-mixed";
      REQUIRE_OP_EVALUATION_SUCCESS(op, code, bytes(wasm.begin(), wasm.end()));
   }
   {
      BOOST_TEST_MESSAGE("testing invalid-mixed-nested");
      std::stringstream ss;
      ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      for(unsigned int i = 0; i < 224; ++i)
         ss << "(if (i32.wrap/i64 (get_local $0)) (then (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 400; ++i)
         ss << "(block (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 400; ++i)
         ss << "(loop (drop (i32.const " <<  i << "))";
      for(unsigned int i = 0; i < 800; ++i)
         ss << ")";
      for(unsigned int i = 0; i < 224; ++i)
         ss << "))";
      ss << "))";
      auto wasm = graphene::chain::wast_to_wasm(ss.str());
      op.name = "invalid-mixed";
      REQUIRE_THROW_WITH_VALUE(op, code, bytes(wasm.begin(), wasm.end()));
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
