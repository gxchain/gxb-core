#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/name.hpp>
#include <graphene/chain/protocol/asset.hpp>

namespace graphene { namespace chain {

   struct action {
      int64_t                  contract_id;
      action_name              method_name;
      bytes                    data;

      action() {}

      action(uint64_t account, action_name name, const bytes& d )
            : contract_id(account & GRAPHENE_DB_MAX_INSTANCE_ID), method_name(name), data(d) {
      }

      action(account_id_type account, action_name name, const bytes& d )
            : contract_id(account.instance), method_name(name), data(d) {
      }

   };

} } /// namespace graphene::chain

FC_REFLECT(graphene::chain::action, (contract_id)(method_name)(data))
