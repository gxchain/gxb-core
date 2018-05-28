#include <algorithm>
#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>

#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace graphene { namespace chain {

void apply_context::exec()
{
   auto start = fc::time_point::now();
   try {
       static const char wast_code[] = R"=====(
(module
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 8) "Hello World!\n")
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $add (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.add
   (get_local $1)
   (get_local $0)
  )
 )
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
 )
)
)=====";

       std::vector<uint8_t> wasm = graphene::chain::wast_to_wasm(wast_code);
       auto code_id = fc::sha256::hash(wast_code, (uint32_t) strlen(wast_code));
       auto wasm_bytes = bytes(wasm.begin(), wasm.end());
       dlog("wast code ${c}, code_id ${i}", ("c", wast_code)("i", code_id));

       wasm_interface(graphene::chain::wasm_interface::vm_type::binaryen).apply(code_id, wasm_bytes, *this);
       dlog("wasm exec success");
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));


   auto console = _pending_console_output.str();
   dlog("CONSOLE OUTPUT BEGIN =====================\n"
           + console
           + " CONSOLE OUTPUT END   =====================" );

   reset_console();
}

void apply_context::reset_console() {
   _pending_console_output = std::ostringstream();
   _pending_console_output.setf( std::ios::scientific, std::ios::floatfield );
}


} } /// graphene::chain
