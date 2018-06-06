#pragma once
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

#include <graphene/chain/action.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/contract_table_objects.hpp>
#include <graphene/chain/protocol/name.hpp>

namespace graphene { namespace chain {

class database;
class apply_context {
   private:
     template <typename T>
     class iterator_cache {
       public:
         iterator_cache() {
             _end_iterator_to_table.reserve(8);
             _iterator_to_object.reserve(32);
         }

         /// Returns end iterator of the table.
         int cache_table(const table_id_object &tobj)
         {
             auto itr = _table_cache.find(tobj.id);
             if (itr != _table_cache.end())
                 return itr->second.second;

             auto ei = index_to_end_iterator(_end_iterator_to_table.size());
             _end_iterator_to_table.push_back(&tobj);
             _table_cache.emplace(tobj.id, make_pair(&tobj, ei));
             return ei;
         }

         const table_id_object &get_table(table_id i) const
         {
             auto itr = _table_cache.find(i);
             FC_ASSERT(itr != _table_cache.end(), "an invariant was broken, table should be in cache");
             return *itr->second.first;
         }

         int get_end_iterator_by_table_id(table_id i) const
         {
             auto itr = _table_cache.find(i);
             FC_ASSERT(itr != _table_cache.end(), "an invariant was broken, table should be in cache");
             return itr->second.second;
         }

         const table_id_object *find_table_by_end_iterator(int ei) const
         {
             FC_ASSERT(ei < -1, "not an end iterator");
             auto indx = end_iterator_to_index(ei);
             if (indx >= _end_iterator_to_table.size()) return nullptr;
             return _end_iterator_to_table[indx];
         }

         const T &get(int iterator)
         {
             FC_ASSERT(iterator != -1, "invalid iterator");
             FC_ASSERT(iterator >= 0, "dereference of end iterator");
             FC_ASSERT(iterator < _iterator_to_object.size(), "iterator out of range");
             auto result = _iterator_to_object[iterator];
             FC_ASSERT(result, "dereference of deleted object");
             return *result;
         }

         void remove(int iterator)
         {
             FC_ASSERT(iterator != -1, "invalid iterator");
             FC_ASSERT(iterator >= 0, "cannot call remove on end iterators");
             FC_ASSERT(iterator < _iterator_to_object.size(), "iterator out of range");
             auto obj_ptr = _iterator_to_object[iterator];
             if (!obj_ptr) return;
             _iterator_to_object[iterator] = nullptr;
             _object_to_iterator.erase(obj_ptr);
         }

         int add(const T &obj)
         {
             auto itr = _object_to_iterator.find(&obj);
             if (itr != _object_to_iterator.end())
                 return itr->second;

             _iterator_to_object.push_back(&obj);
             _object_to_iterator[&obj] = _iterator_to_object.size() - 1;

             return _iterator_to_object.size() - 1;
         }

       private:
         map<table_id, pair<const table_id_object *, int>> _table_cache;
         vector<const table_id_object *> _end_iterator_to_table;
         vector<const T *> _iterator_to_object;
         map<const T *, int> _object_to_iterator;

         /// Precondition: std::numeric_limits<int>::min() < ei < -1
         /// Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
         inline size_t end_iterator_to_index(int ei) const { return (-ei - 2); }
         /// Precondition: indx < _end_iterator_to_table.size() <= std::numeric_limits<int>::max()
         inline int index_to_end_iterator(size_t indx) const { return -(indx + 2); }
      }; /// class iterator_cache

      template<typename>
      struct array_size;

      template<typename T, size_t N>
      struct array_size< std::array<T,N> > {
          static constexpr size_t size = N;
      };

      template <typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst, typename Enable = void>
      class secondary_key_helper;

      template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
      class secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
         typename std::enable_if<std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value>::type >
      {
         public:
            typedef SecondaryKey secondary_key_type;

            static void set(secondary_key_type& sk_in_table, const secondary_key_type& sk_from_wasm) {
               sk_in_table = sk_from_wasm;
            }

            static void get(secondary_key_type& sk_from_wasm, const secondary_key_type& sk_in_table ) {
               sk_from_wasm = sk_in_table;
            }

            static auto create_tuple(const table_id_object& tab, const secondary_key_type& secondary) {
               return boost::make_tuple( tab.id, secondary );
            }
      };

      template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
      class secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
         typename std::enable_if<!std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value &&
                                 std::is_pointer<typename std::decay<SecondaryKeyProxy>::type>::value>::type >
      {
         public:
            typedef SecondaryKey      secondary_key_type;
            typedef SecondaryKeyProxy secondary_key_proxy_type;
            typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

            static constexpr size_t N = array_size<SecondaryKey>::size;

            static void set(secondary_key_type& sk_in_table, secondary_key_proxy_const_type sk_from_wasm) {
               std::copy(sk_from_wasm, sk_from_wasm + N, sk_in_table.begin());
            }

            static void get(secondary_key_proxy_type sk_from_wasm, const secondary_key_type& sk_in_table) {
               std::copy(sk_in_table.begin(), sk_in_table.end(), sk_from_wasm);
            }

            static auto create_tuple(const table_id_object& tab, secondary_key_proxy_const_type sk_from_wasm) {
               secondary_key_type secondary;
               std::copy(sk_from_wasm, sk_from_wasm + N, secondary.begin());
               return boost::make_tuple( tab.id, secondary );
            }
      };

   /// Constructor
   public:
      apply_context(const database&d, const action& a)
          : act(a)
          , db(d)
          , receiver(a.account)
          , wasm_if(graphene::chain::wasm_interface::vm_type::binaryen)
      {
        reset_console();
      }

   public:
      const action&       act; ///< message being applied
      const database&     db;
      account_name        receiver;
      wasm_interface      wasm_if;


   private:
      iterator_cache<key_value_object>    keyval_cache;

   /// Execution methods:
   public:
      void exec();
      wasm_interface& get_wasm_interface();

   /// Database methods:
   public:
     void update_db_usage(const account_name &payer, int64_t delta);
     int db_store_i64(uint64_t scope, uint64_t table, const account_name &payer, uint64_t id, const char *buffer, size_t buffer_size);
     void db_update_i64(int iterator, account_name payer, const char *buffer, size_t buffer_size);
     void db_remove_i64(int iterator);
     int db_get_i64(int iterator, char *buffer, size_t buffer_size);
     int db_next_i64(int iterator, uint64_t &primary);
     int db_previous_i64(int iterator, uint64_t &primary);
     int db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
     int db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
     int db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
     int db_end_i64(uint64_t code, uint64_t scope, uint64_t table);

   private:
     const table_id_object *find_table(name code, name scope, name table);
     const table_id_object &find_or_create_table(name code, name scope, name table, const account_name &payer);
     void remove_table(const table_id_object &tid);
     int db_store_i64(uint64_t code, uint64_t scope, uint64_t table, const account_name &payer, uint64_t id, const char *buffer, size_t buffer_size);

     /// Console methods:
   public:
      void reset_console();
      std::ostringstream &get_console_stream() { return _pending_console_output; }
      const std::ostringstream &get_console_stream() const { return _pending_console_output; }

      template<typename T>
      void console_append(T val) {
          _pending_console_output << val;
      }

      template<typename T, typename ...Ts>
      void console_append(T val, Ts ...rest) {
          console_append(val);
          console_append(rest...);
      };

      inline void console_append_formatted(const string& fmt, const variant_object& vo) {
          console_append(fc::format_string(fmt, vo));
      }

   private:
     std::ostringstream _pending_console_output;
};

} } // namespace graphene::chain

//FC_REFLECT(graphene::chain::apply_context::apply_results, (applied_actions)(deferred_transaction_requests)(deferred_transactions_count))
