/*
    Copyright (C) 2019 gxb

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

namespace graphene { namespace chain {

    /**
     *  @brief caculate Contribution ratio
     *  @ingroup object
     *  @ingroup protocol
     */
     class staking_object : public graphene::chain::abstract_object<staking_object>
     {
       public:
          static const uint8_t space_id = protocol_ids;
          static const uint8_t type_id = staking_object_type;

          account_id_type   owner;
          time_point_sec    create_time;
          time_point_sec    staking_seconds;
          asset             amount;
          account_id_type   trust_node;
     };
     struct by_owner;
     struct by_trust_node;

     /**
     * @ingroup object_index
     */
     typedef multi_index_container<
        trust_node_pledge_object,indexed_by<
            ordered_unique<tag<by_id>,member<object,object_id_type,&object::id>>,
            ordered_unique<tag<by_owner>,member<staking_object,account_id_type,&staking_object::owner>>,
            ordered_unique<tag<by_trust_node>,member<staking_object,account_id_type, &staking_object::trust_node>>> 
     > staking_multi_index_type;

     typedef generic_index<staking_object, staking_multi_index_type> staking_index;

 }}

 FC_REFLECT_DERIVED (graphene::chain::staking_object,
                     (graphene::chain::object),
                     (owner)(create_time)(staking_seconds)(amount)(trust_node))
