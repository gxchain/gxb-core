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
#include <graphene/chain/schema_context_object.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <vector>

namespace graphene {
    namespace chain {

    using namespace std;

        /**
         * @brief This class represents a data_product
         * @ingroup object
         * @ingroup protocol
         *
         * Accounts are the primary unit of authority on the graphene system. Users must have an account in order to use
         * assets, trade in the markets, vote for committee_members, etc.
         */
        class free_data_product_object : public graphene::db::abstract_object<free_data_product_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = free_data_product_object_type;
            fc::string                              product_name;
            fc::string                              brief_desc;
            // provider of data product
            account_id_type                         datasource;
            // data market categoiry id
            data_market_category_id_type            category_id;
            // data price,   price  = x * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint64_t                                price;
            // data product status
            // 0 --> init, 1 --> online, 2 --> offline
            uint8_t                                 status;
            // icon of data product
            fc::string                              icon;
            // schema of input/output in data_transaction
            vector<schema_context_object>           schema_contexts;
            // product parent_id, can be empty
            fc::string                              parent_id;
            time_point_sec                          create_date_time;
            time_point_sec                          recommend_expiration_date_time;
            // total data_transaction count
            uint64_t                                total = 0;
            // issuer of product
            account_id_type                         issuer;

        };

        class free_data_product_search_results_object
                : public graphene::db::abstract_object<free_data_product_search_results_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_free_data_product_search_results_object_type;

            uint64_t total;
            uint64_t filtered_total;
            vector <free_data_product_object> data;

        };

        struct by_data_market_category_id;
        struct by_recommend_expiration_date_time;
        /**
         * @ingroup object_index
         */
        using free_data_product_multi_index_type = multi_index_container<
           free_data_product_object,
           indexed_by<
              ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
              ordered_non_unique< tag<by_data_market_category_id>,
                composite_key<
                   free_data_product_object,
                     member<free_data_product_object, data_market_category_id_type,  &free_data_product_object::category_id>
                >
              >,
                ordered_non_unique< tag<by_recommend_expiration_date_time>,
                  composite_key<
                     free_data_product_object,
                       member<free_data_product_object, time_point_sec,  &free_data_product_object::recommend_expiration_date_time>
                  >
                >
           >
        >;

        /**
         * @ingroup object_index
         */
        using free_data_product_index = generic_index<free_data_product_object, free_data_product_multi_index_type>;
} }

FC_REFLECT_DERIVED(graphene::chain::free_data_product_object,
                   (graphene::db::object),
                   (product_name)(brief_desc)
                           (datasource)(category_id)(price)
                           (status)(icon)(schema_contexts)(issuer)(parent_id)(create_date_time)(recommend_expiration_date_time)(total))

FC_REFLECT_DERIVED(graphene::chain::free_data_product_search_results_object,
                   (graphene::db::object),
                   (total)(filtered_total)(data))
