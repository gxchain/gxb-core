/*
    Copyright (C) 2017 gxb

    This file is part of gxb-core.

    gxb-core is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gxb-core is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gxb-core.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/transaction.hpp>

namespace graphene { namespace chain {

    /**
     *  @brief caculate Contribution ratio
     *  @ingroup object
     *  @ingroup protocol
     */
     class pocs_object : public graphene::chain::abstract_object<pocs_object>
     {
       public:
          static const uint8_t space_id = protocol_ids;
          static const uint8_t type_id = pocs_object_type;
          league_id_type    league_id;
          account_id_type   account_id;
          uint64_t          total_sell = 0;
          uint64_t          total_buy = 0;
     };

     struct by_id;
     struct by_league_id;
     struct by_account_id;
     struct by_multi_id;

     typedef multi_index_container<
        pocs_object, indexed_by<
          ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
          ordered_non_unique<tag<by_league_id>, composite_key<pocs_object, member<pocs_object, league_id_type, &pocs_object::league_id>>>,
          ordered_non_unique<tag<by_account_id>, composite_key<pocs_object, member<pocs_object, account_id_type, &pocs_object::account_id>>>,
          ordered_unique<tag<by_multi_id>, composite_key<pocs_object, member<pocs_object, league_id_type, &pocs_object::league_id>,
          member<pocs_object, account_id_type, &pocs_object::account_id>>>
        >
     > pocs_multi_index_type;

     typedef generic_index<pocs_object, pocs_multi_index_type> pocs_index;
 }}

 FC_REFLECT_DERIVED (graphene::chain::pocs_object,
                     (graphene::chain::object),
                     (league_id)(account_id)(total_sell)(total_buy))
