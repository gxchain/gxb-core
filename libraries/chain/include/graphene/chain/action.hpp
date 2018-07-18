#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/name.hpp>
#include <graphene/chain/protocol/asset.hpp>

namespace graphene { namespace chain {

   struct action {
      account_id_type          contract_id;
      fc::optional<asset>      amount;
      action_name              method_name;
      bytes                    data;

      action() {}

      action(uint64_t account, optional<asset> amnt, action_name name, const bytes& d )
            : contract_id(account_id_type(account & GRAPHENE_DB_MAX_INSTANCE_ID)), amount(amnt), method_name(name), data(d) {
      }

      action(account_id_type account, optional<asset> amnt, action_name name, const bytes& d )
            : contract_id(account), amount(amnt), method_name(name), data(d) {
      }

   };

} } /// namespace graphene::chain

FC_REFLECT(graphene::chain::action, (contract_id)(amount)(method_name)(data))
