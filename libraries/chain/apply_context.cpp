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
       auto& acnt_indx = db.get_index_type<account_index>();
       auto account_itr = acnt_indx.indices().get<by_name>().find(receiver.to_string());
       dlog("receiver: ${r}", ("r", receiver.to_string()));
       // dlog("wast code: ${c}", ("c", account_itr->code));
       auto wasm_bytes = bytes(account_itr->code.begin(), account_itr->code.end());
       try {
           wasm_interface(graphene::chain::wasm_interface::vm_type::binaryen).apply(account_itr->code_version, wasm_bytes, *this);
       } catch (const wasm_exit&) {}
       dlog("wasm exec success");
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));


   auto console = _pending_console_output.str();
   dlog("CONSOLE OUTPUT BEGIN =====================\n"
           + console + "\n"
           + " CONSOLE OUTPUT END   =====================" );

   reset_console();
   auto end = fc::time_point::now();
   dlog("elapsed ${n}", ("n", end - start));
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
