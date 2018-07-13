#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/name.hpp>

namespace graphene { namespace chain {

   /**
    *  An action is performed by an actor, aka an account. It may
    *  be created explicitly and authorized by signatures or might be
    *  generated implicitly by executing application code.
    *
    *  This follows the design pattern of React Flux where actions are
    *  named and then dispatched to one or more action handlers (aka stores).
    *  In the context of graphene, every action is dispatched to the handler defined
    *  by account 'scope' and function 'name', but the default handler may also
    *  forward the action to any number of additional handlers. Any application
    *  can write a handler for "scope::name" that will get executed if and only if
    *  this action is forwarded to that application.
    *
    *  Each action may require the permission of specific actors. Actors can define
    *  any number of permission levels. The actors and their respective permission
    *  levels are declared on the action and validated independently of the executing
    *  application code. An application code will check to see if the required authorization
    *  were properly declared when it executes.
    */
   struct action {
      account_id_type          contract_id;
      action_name              method_name;
      bytes                    data;

      action(){}

      template<typename T, std::enable_if_t<std::is_base_of<bytes, T>::value, int> = 1>
      action(const T& value ) {
         contract_id     = T::get_account();
         method_name     = T::get_name();
         data.assign(value.data(), value.data() + value.size());
      }

      template<typename T, std::enable_if_t<!std::is_base_of<bytes, T>::value, int> = 1>
      action(const T& value ) {
         contract_id     = T::get_account();
         method_name     = T::get_name();
         data            = fc::raw::pack(value);
      }

      action(uint64_t account, action_name name, const bytes& data )
            : contract_id(account_id_type(account & GRAPHENE_DB_MAX_INSTANCE_ID)), method_name(name), data(data) {
      }

      action(account_id_type account, action_name name, const bytes& data )
            : contract_id((uint64_t)account), method_name(name), data(data) {
      }

      template <typename T>
      T data_as() const
      {
          FC_ASSERT(contract_id == T::get_account());
          FC_ASSERT(method_name == T::get_name());
          return fc::raw::unpack<T>(data);
      }
   };

} } /// namespace graphene::chain

FC_REFLECT( graphene::chain::action, (contract_id)(method_name)(data) )
