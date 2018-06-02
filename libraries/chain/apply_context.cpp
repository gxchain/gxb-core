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
(import "env" "prints" (func $prints (param i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 4) "Hello World!\n")
 (export "memory" (memory $0))
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
    (call $prints
    (i32.const 4)
    )
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

void apply_context::reset_console()
{
    _pending_console_output = std::ostringstream();
    _pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
}

int apply_context::db_store_i64(uint64_t scope, uint64_t table, const account_name &payer, uint64_t id, const char *buffer, size_t buffer_size)
{
    return db_store_i64(receiver, scope, table, payer, id, buffer, buffer_size);
}

int apply_context::db_store_i64(uint64_t code, uint64_t scope, uint64_t table, const account_name &payer, uint64_t id, const char *buffer, size_t buffer_size)
{
    return 0;
}

void apply_context::db_update_i64(int iterator, account_name payer, const char *buffer, size_t buffer_size)
{

}

void apply_context::db_remove_i64(int iterator)
{

}

int apply_context::db_get_i64(int iterator, char *buffer, size_t buffer_size)
{
    const key_value_object &obj = keyval_cache.get(iterator);

    auto s = obj.value.size();
    if (buffer_size == 0) return s;

    auto copy_size = std::min(buffer_size, s);
    memcpy(buffer, obj.value.data(), copy_size);

    return copy_size;
}

int apply_context::db_next_i64(int iterator, uint64_t &primary)
{
    return 0;
}

int apply_context::db_previous_i64(int iterator, uint64_t &primary)
{
    return 0;
}

int apply_context::db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    return 0;
}

int apply_context::db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    return 0;
}

int apply_context::db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    return 0;
}

int apply_context::db_end_i64(uint64_t code, uint64_t scope, uint64_t table)
{
    return 0;
}

} } /// graphene::chain
