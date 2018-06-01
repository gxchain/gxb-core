#pragma once

#include <graphene/chain/multi_index_includes.hpp>
#include <fc/uint128.hpp>
#include <softfloat.hpp>

#include <array>
#include <type_traits>

namespace graphene { namespace chain {

typedef __uint128_t uint128_t;

   /**
    * @brief The table_id_object class tracks the mapping of (scope, code, table) to an opaque identifier
    */
class table_id_object : public graphene::db::abstract_object<table_id_object>
{
  public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_table_id_object_type;

    account_name            code;
    scope_name              scope;
    table_name              table;
    account_name            payer;
    uint32_t                count = 0; /// the number of elements in the table
};

struct by_code_scope_table;

using table_id_multi_index = multi_index_container<
  table_id_object,
  indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique<tag<by_code_scope_table>,
        composite_key< table_id_object,
           member<table_id_object, account_name, &table_id_object::code>,
           member<table_id_object, scope_name,   &table_id_object::scope>,
           member<table_id_object, table_name,   &table_id_object::table>
        >
     >
  >
>;

using table_id = table_id_object_id_type;

struct by_scope_primary;
struct by_scope_secondary;
struct by_scope_tertiary;

class key_value_object : public graphene::db::abstract_object<key_value_object>
{
  public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_key_value_object_type;

    typedef uint64_t key_type;
    static const int number_of_keys = 1;

    table_id                    t_id;
    uint64_t                    primary_key;
    account_name                payer = 0;
    bytes                       value;
};

using key_value_index = multi_index_container<
  key_value_object,
  indexed_by<
     ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
     ordered_unique<tag<by_scope_primary>,
        composite_key< key_value_object,
           member<key_value_object, table_id, &key_value_object::t_id>,
           member<key_value_object, uint64_t, &key_value_object::primary_key>
        >,
        composite_key_compare< std::less<table_id>, std::less<uint64_t> >
     >
  >
>;

struct by_primary;
struct by_secondary;

template <typename SecondaryKey, uint64_t ObjectTypeId, typename SecondaryKeyLess = std::less<SecondaryKey>>
struct secondary_index {
    class index_object : public graphene::db::abstract_object<index_object> {
        typedef SecondaryKey secondary_key_type;
    public:

        table_id t_id;
        uint64_t primary_key;
        account_name payer = 0;
        SecondaryKey secondary_key;
    };

    typedef multi_index_container<
        index_object,
        indexed_by<
            ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
            ordered_unique<tag<by_primary>,
               composite_key<index_object,
                 member<index_object, table_id, &index_object::t_id>,
                 member<index_object, uint64_t, &index_object::primary_key>>,
               composite_key_compare<std::less<table_id>, std::less<uint64_t>>>,
            ordered_unique<tag<by_secondary>,
               composite_key<index_object,
                 member<index_object, table_id, &index_object::t_id>,
                 member<index_object, SecondaryKey, &index_object::secondary_key>,
                 member<index_object, uint64_t, &index_object::primary_key>>,
               composite_key_compare<std::less<table_id>, SecondaryKeyLess, std::less<uint64_t>>
            >
        >
    > index_index;
};

typedef secondary_index<uint64_t, index64_object_type>::index_object index64_object;
typedef secondary_index<uint64_t, index64_object_type>::index_index index64_index;

typedef secondary_index<uint128_t, index128_object_type>::index_object index128_object;
typedef secondary_index<uint128_t, index128_object_type>::index_index index128_index;

typedef std::array<uint128_t, 2> key256_t;
typedef secondary_index<key256_t, index256_object_type>::index_object index256_object;
typedef secondary_index<key256_t, index256_object_type>::index_index index256_index;

} }  // namespace graphene::chain

FC_REFLECT_DERIVED(graphene::chain::table_id_object,
                   (graphene::db::object),
                   (id)(code)(scope)(table))

FC_REFLECT_DERIVED(graphene::chain::key_value_object,
                   (graphene::db::object),
                   (id)(t_id)(primary_key)(value)(payer))
