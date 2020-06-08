#pragma once
#include <graphene/db/object.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

   class op_entry_object : public abstract_object<op_entry_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_op_entry_history_object_type;

         op_entry_object(){}

         uint32_t               block_num;  
         uint32_t               trx_in_block;
         uint32_t               op_in_trx;
         std::string            op_index;
         uint32_t               sequence; //operation in account history number 
         bool                   is_virtual = false;  // wheather virtual operation
         operation              virtual_op;//if vitual op, record 
   };

   struct by_id;
   struct by_opindex;
   struct by_blocknum;

   typedef multi_index_container<
      op_entry_object,
      indexed_by<
         ordered_unique< tag<by_id>,           member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_opindex>,      member< op_entry_object, std::string, &op_entry_object::op_index > >,
         ordered_non_unique< tag<by_blocknum>, member< op_entry_object, uint32_t, &op_entry_object::block_num > >
      >
      
   > op_entry_multi_index_type;

   typedef generic_index<op_entry_object, op_entry_multi_index_type> op_entry_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::op_entry_object, (graphene::chain::object),
                    (block_num)(trx_in_block)(op_in_trx)(op_index)(sequence)(is_virtual)(virtual_op))
