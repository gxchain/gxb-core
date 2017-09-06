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

#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

/**
 *  @brief Recorded secondary data information for sale
 *  @ingroup object
 *  @ingroup protocol
 */
 class second_hand_data_object : public graphene :: chain :: abstract_object<second_hand_data_object>
 {
    public:
     static const uint8_t space_id = protocol_ids;
     static const uint8_t type_id  = second_hand_data_object_type;

     account_id_type      second_hand_datasource_id;
     string               copyright_hash;
     account_id_type      datasource_account_id;
     string               data_transaction_request_id;
     time_point_sec       create_date_time;
 };

 struct by_second_hand_datasource;
 struct by_create_date_time;
 using second_hand_data_multi_index_type = multi_index_container<
     second_hand_data_object,
     indexed_by<
         ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
         ordered_non_unique<tag<by_second_hand_datasource>,
                            composite_key<second_hand_data_object,
                                          member<second_hand_data_object, account_id_type, &second_hand_data_object::second_hand_datasource_id>>>,
         ordered_non_unique<tag<by_create_date_time>, composite_key<second_hand_data_object, member<second_hand_data_object, time_point_sec,
                                                                                                    &second_hand_data_object::create_date_time>>>>>;

 using second_hand_data_index = generic_index<second_hand_data_object, second_hand_data_multi_index_type>;
}}

FC_REFLECT_DERIVED(graphene::chain::second_hand_data_object, (graphene::chain::object),
                   (second_hand_datasource_id)
                   (copyright_hash)
                   (datasource_account_id)
                   (data_transaction_request_id)
                   (create_date_time))
