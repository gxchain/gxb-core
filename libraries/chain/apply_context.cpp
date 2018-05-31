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
    //   require_write_lock( scope );
    const auto &tab = find_or_create_table(code, scope, table, payer);
    auto tableid = tab.id;

    FC_ASSERT(payer != account_name(), "must specify a valid account to pay for new record");

    const auto &obj = db.create<key_value_object>([&](auto &o) {
        o.t_id = tableid;
        o.primary_key = id;
        o.value.resize(buffer_size);
        o.payer = payer;
        memcpy(o.value.data(), buffer, buffer_size);
    });

    db.modify(tab, [&](auto &t) {
        ++t.count;
    });

    int64_t billable_size = (int64_t)(buffer_size + config::billable_size_v<key_value_object>);
    update_db_usage(payer, billable_size);

    keyval_cache.cache_table(tab);
    return keyval_cache.add(obj);
}

void apply_context::db_update_i64(int iterator, account_name payer, const char *buffer, size_t buffer_size)
{
    const key_value_object &obj = keyval_cache.get(iterator);

    const auto &table_obj = keyval_cache.get_table(obj.t_id);
    FC_ASSERT(table_obj.code == receiver, "db access violation");

    //   require_write_lock( table_obj.scope );

    const int64_t overhead = config::billable_size_v<key_value_object>;
    int64_t old_size = (int64_t)(obj.value.size() + overhead);
    int64_t new_size = (int64_t)(buffer_size + overhead);

    if (payer == account_name()) payer = obj.payer;

    if (account_name(obj.payer) != payer) {
        // refund the existing payer
        update_db_usage(obj.payer, -(old_size));
        // charge the new payer
        update_db_usage(payer, (new_size));
    } else if (old_size != new_size) {
        // charge/refund the existing payer the difference
        update_db_usage(obj.payer, new_size - old_size);
    }

    db.modify(obj, [&](auto &o) {
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

    //   require_write_lock( table_obj.scope );

    update_db_usage(obj.payer, -(obj.value.size() + config::billable_size_v<key_value_object>) );

    db.modify(table_obj, [&](auto &t) {
        --t.count;
    });
    db.remove(obj);

    if (table_obj.count == 0) {
        remove_table(table_obj);
    }

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
    const auto &idx = db.get_index<key_value_index, by_scope_primary>();

    auto itr = idx.iterator_to(obj);
    ++itr;

    if (itr == idx.end() || itr->t_id != obj.t_id) return keyval_cache.get_end_iterator_by_table_id(obj.t_id);

    primary = itr->primary_key;
    return keyval_cache.add(*itr);
}

int apply_context::db_previous_i64(int iterator, uint64_t &primary)
{
    const auto &idx = db.get_index<key_value_index, by_scope_primary>();
    if (iterator < -1) // is end iterator
    {
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
    //require_read_lock( code, scope ); // redundant?
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const key_value_object *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tab->id, id));
    if (!obj) return table_end_itr;

    return keyval_cache.add(*obj);
}

int apply_context::db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    //require_read_lock( code, scope ); // redundant?
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto &idx = db.get_index<key_value_index, by_scope_primary>();
    auto itr = idx.lower_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    //require_read_lock( code, scope ); // redundant?
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto &idx = db.get_index<key_value_index, by_scope_primary>();
    auto itr = idx.upper_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_end_i64(uint64_t code, uint64_t scope, uint64_t table)
{
    //require_read_lock( code, scope ); // redundant?
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    return keyval_cache.cache_table(*tab);
}

} } /// graphene::chain
