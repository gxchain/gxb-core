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
    const key_value_object &obj = keyval_cache.get(iterator);

    // validate
    const auto &table_obj = keyval_cache.get_table(obj.t_id);
    FC_ASSERT(table_obj.code == receiver, "db access violation");

    _db->modify(obj, [&](key_value_object &o) {
        o.value.resize(buffer_size);
        memcpy(o.value.data(), buffer, buffer_size);
        o.payer = payer;
    });
}

void apply_context::db_remove_i64(int iterator)
{
    const key_value_object &obj = keyval_cache.get(iterator);

    const auto &table_obj = keyval_cache.get_table(obj.t_id);
    FC_ASSERT(table_obj.code == receiver, "db access violation");

    // update_db_usage

    _db->remove(obj);
    keyval_cache.remove(iterator);
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
    if (iterator < -1) return -1; // cannot increment past end iterator of table

    const auto &obj = keyval_cache.get(iterator); // Check for iterator != -1 happens in this call
    const auto& kv_idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();

    auto itr = kv_idx.iterator_to(obj);
    ++itr;

    if (itr == kv_idx.end() || itr->t_id != obj.t_id) return keyval_cache.get_end_iterator_by_table_id(obj.t_id);

    primary = itr->primary_key;
    return keyval_cache.add(*itr);
}

int apply_context::db_previous_i64(int iterator, uint64_t &primary)
{
    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();

    // is end iterator
    if (iterator < -1) {
        auto tab = keyval_cache.find_table_by_end_iterator(iterator);
        FC_ASSERT(tab, "not a valid end iterator");

        auto itr = idx.upper_bound(tab->id);
        if (idx.begin() == idx.end() || itr == idx.begin()) return -1; // Empty table

        --itr;

        if (itr->t_id != tab->id) return -1; // Empty table

        primary = itr->primary_key;
        return keyval_cache.add(*itr);
    }

    const auto &obj = keyval_cache.get(iterator); // Check for iterator != -1 happens in this call

    auto itr = idx.iterator_to(obj);
    if (itr == idx.begin()) return -1; // cannot decrement past beginning iterator of table

    --itr;

    if (itr->t_id != obj.t_id) return -1; // cannot decrement past beginning iterator of table

    primary = itr->primary_key;
    return keyval_cache.add(*itr);
}

int apply_context::db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto& tab = find_table(code, scope, table);
    if (!tab.valid()) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& kv_idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto iter = kv_idx.find(boost::make_tuple(tab->id, id));
    if (iter == kv_idx.end()) return table_end_itr;

    return keyval_cache.add(*iter);
}

int apply_context::db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto &tab = find_table(code, scope, table);
    if (!tab.valid()) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto itr = idx.lower_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto &tab = find_table(code, scope, table);
    if (!tab.valid()) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto itr = idx.upper_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_end_i64(uint64_t code, uint64_t scope, uint64_t table)
{
    const auto &tab = find_table(code, scope, table);
    if (!tab.valid()) return -1;

    return keyval_cache.cache_table(*tab);
}

optional<table_id_object> apply_context::find_table(name code, name scope, name table)
{
    const auto& table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
    auto existing_tid = table_idx.find(boost::make_tuple(code, scope, table));
    if (existing_tid != table_idx.end()) {
        return *existing_tid;
    }

    return {};
}

const table_id_object &apply_context::find_or_create_table(name code, name scope, name table, const account_name &payer)
{
    const auto& table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
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

    _db->remove(tid);
}

} } /// graphene::chain
