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

namespace graphene
{
namespace chain
{

/**
 *  @brief datasource_copyright_object
 *  @ingroup object
 *  @ingroup protocol
 */
class datasource_copyright_object : public graphene ::chain ::abstract_object<datasource_copyright_object>
{
   public:
    static const uint8_t space_id = protocol_ids;
    static const uint8_t type_id  = datasource_copyright_object_type;

    string          copyright_hash;
    account_id_type datasource_account_id;
    object_id_type  product_id;
    string          data_transaction_request_id;
    time_point_sec  create_date_time;
};

struct by_copyright_hash;
struct by_multi_id;
using datasource_copyright_multi_index_type = multi_index_container<
    datasource_copyright_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
        ordered_unique<tag<by_copyright_hash>,
            composite_key<
                datasource_copyright_object,
                member<datasource_copyright_object, string, &datasource_copyright_object::copyright_hash>
            >
        >,
        ordered_non_unique<tag<by_multi_id>,
            composite_key<
                datasource_copyright_object,
                member<datasource_copyright_object, account_id_type, &datasource_copyright_object::datasource_account_id>,
                member<datasource_copyright_object, object_id_type, &datasource_copyright_object::product_id>
            >
        >
    >
>;

    using datasource_copyright_index = generic_index<datasource_copyright_object, datasource_copyright_multi_index_type>;

} }  // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::datasource_copyright_object, (graphene::chain::object),
                   (copyright_hash)(datasource_account_id)(product_id)(data_transaction_request_id)(create_date_time))
