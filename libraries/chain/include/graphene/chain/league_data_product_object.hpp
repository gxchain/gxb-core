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
#include <graphene/chain/schema_context_object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <vector>
namespace graphene {
    namespace chain {

    using namespace std;

        class league_data_product_object : public graphene::db::abstract_object<league_data_product_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = league_data_product_object_type;

            fc::string                              product_name;
            // product brief description
            fc::string                              brief_desc;
            // data market category id
            data_market_category_id_type            category_id;
            // default reference price of data product, real price is in league object
            // refer_price  = x * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint64_t                                refer_price;
            // data product status
            // 0 --> init, 1 --> online, 2 --> offline
            uint8_t                                 status;
            // icon of data product
            fc::string                              icon;
            // schema of input/output in data_transaction
            vector<schema_context_object>           schema_contexts;
            time_point_sec                          create_date_time;
            // total data_transaction count
            uint64_t                                total = 0;
            // issuer of product
            account_id_type                         issuer;
            // Trigger pocs threshold
            uint64_t                                pocs_threshold = 0;
        };

        class league_data_product_search_results_object
                : public graphene::db::abstract_object<league_data_product_search_results_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_league_data_product_search_results_object_type;

            uint64_t                                total;
            uint64_t                                filtered_total;
            vector <league_data_product_object>     data;
        };

        struct by_data_market_category_id;
        /**
         * @ingroup object_index
         */
        using league_data_product_multi_index_type = multi_index_container<
           league_data_product_object,
           indexed_by<
              ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
              ordered_non_unique< tag<by_data_market_category_id>,
                composite_key<
                   league_data_product_object,
                     member<league_data_product_object, data_market_category_id_type,  &league_data_product_object::category_id>
                >
              >
           >
        >;

        struct by_recommend_expiration_date_time;
        /**
         * @ingroup object_index
         */
        using league_data_product_index = generic_index<league_data_product_object, league_data_product_multi_index_type>;
}}

FC_REFLECT_DERIVED(graphene::chain::league_data_product_object,
                   (graphene::db::object),
                   (product_name)(brief_desc)
                   (category_id)(refer_price)
                   (status)(icon)(schema_contexts)(issuer)(total)(create_date_time)(pocs_threshold))

FC_REFLECT_DERIVED(graphene::chain::league_data_product_search_results_object,
                   (graphene::db::object),
                   (total)(filtered_total)(data))
