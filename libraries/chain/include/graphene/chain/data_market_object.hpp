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

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <vector>
namespace graphene {
    namespace chain {

    using namespace std;

        class database;

        class data_market_category_object : public graphene::db::abstract_object<data_market_category_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = data_market_category_object_type;

            string          category_name;
            uint8_t         data_market_type;
            uint32_t        order_num;
            account_id_type issuer;
            uint8_t         status = 0; // status 0 --> init, 1 --> online, 2 --> offline
            time_point_sec  create_date_time;
        };

        struct by_issuer;
        struct by_data_market_type;
        /**
         * @ingroup object_index
         */
        using data_market_category_multi_index_type = multi_index_container<
           data_market_category_object,
           indexed_by<
              ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
              //ordered_unique< tag<by_data_market_type>, member<data_market_category_object, uint8_t, &data_market_category_object::data_market_type> >,
              ordered_non_unique< tag<by_issuer>, member<data_market_category_object, account_id_type, &data_market_category_object::issuer > >,
              ordered_non_unique< tag<by_data_market_type>,
                composite_key<
                   data_market_category_object,
                   member<data_market_category_object, uint8_t, &data_market_category_object::data_market_type>
                >
              >
           >
        >;
        /**
         * @ingroup object_index
         */
        using data_market_category_index = generic_index<data_market_category_object, data_market_category_multi_index_type>;

    }
}


FC_REFLECT_DERIVED(graphene::chain::data_market_category_object,
                   (graphene::db::object),
                   (category_name)
                   (data_market_type)
                   (order_num)
                   (issuer)
                   (status)
                   (create_date_time)
                   )
