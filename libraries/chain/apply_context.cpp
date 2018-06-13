#include <algorithm>
#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/contract_table_objects.hpp>

#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace graphene { namespace chain {

void apply_context::exec()
{
   auto start = fc::time_point::now();
   try {
       auto& acnt_indx = _db->get_index_type<account_index>();
       auto account_itr = acnt_indx.indices().get<by_name>().find(receiver.to_string());
       dlog("contract receiver: ${r}", ("r", receiver.to_string()));
       auto wasm_bytes = bytes(account_itr->code.begin(), account_itr->code.end());
       try {
           wasm_interface &wasm = const_cast<wasm_interface&>(_db->wasmif);
           wasm.apply(account_itr->code_version, wasm_bytes, *this);
       } catch (const wasm_exit&) {}
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));


   auto console = _pending_console_output.str();
   dlog("CONSOLE OUTPUT BEGIN =====================\n"
           + console + "\n"
           + " CONSOLE OUTPUT END =====================" );

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
    const auto &tab = find_or_create_table(code, scope, table, payer);
    auto tableid = tab.id;

    // assert payer
    const auto& new_obj = _db->create<key_value_object>([&](key_value_object& o) {
        o.t_id = tableid;
        o.primary_key = id;
        o.value.resize(buffer_size);
        o.payer = payer;
        memcpy(o.value.data(), buffer, buffer_size);
    });

    // update_db_usage

    keyval_cache.cache_table(tab);
    return keyval_cache.add(new_obj);
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

optional<table_id_object> apply_context::find_table(name code, name scope, name table)
{
    auto &t1 = _db->get_index_type<table_id_multi_index>();
    auto t2 = t1.indices();
    auto &table_idx = t2.get<by_code_scope_table>();
//    auto table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
    auto existing_tid = table_idx.find(boost::make_tuple(code, scope, table));
    if (existing_tid != table_idx.end()) {
        return *existing_tid;
   }
   return {};
}

const table_id_object &apply_context::find_or_create_table(name code, name scope, name table, const account_name &payer)
{
    auto &t1 = _db->get_index_type<table_id_multi_index>();
    auto t2 = t1.indices();
    auto &table_idx = t2.get<by_code_scope_table>();
//    auto table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
    auto existing_tid = table_idx.find(boost::make_tuple(code, scope, table));
    if (existing_tid != table_idx.end()) {
        return *existing_tid;
   }

   // update_db_usage

   return _db->create<table_id_object>([&](table_id_object &t_id){
      t_id.code = code;
      t_id.scope = scope;
      t_id.table = table;
      t_id.payer = payer;
   });
}

void apply_context::remove_table(const table_id_object &tid)
{
    // update_db_usage
    // db.remove(tid);
}

} } /// graphene::chain
