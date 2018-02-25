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

        class league_object : public graphene::db::abstract_object<league_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = league_object_type;

            fc::string                              league_name;
            fc::string                              brief_desc;
            // data products in league
            vector<league_data_product_id_type>     data_products;
            // prices of data products in league,  price  = x * GRAPHENE_BLOCKCHAIN_PRECISION;
            vector<uint64_t>                        prices;
            uint8_t                                 status;
            // data markeat categodry id
            data_market_category_id_type            category_id;
            // league icon, base64 decoded string
            fc::string                              icon;
            time_point_sec                          create_date_time;
            time_point_sec                          recommend_expiration_date_time;
            // total data_transaction count
            uint64_t                                total;
            // issuer of league
            account_id_type                         issuer;
            vector<account_id_type>                 members;

            // trigger pocs thresholds
            vector<uint64_t>                        pocs_thresholds;
            // the base fee rate
            vector<uint64_t>                        fee_bases;
            // the weight of the pocs
            vector<uint64_t>                        product_pocs_weights;
        };

        // sort method for league_object
        struct sort_league_object_by_name {
            bool operator() (const league_object &l, const league_object &r) {
                return l.league_name.compare(r.league_name) < 0 ? true : false;
            }
        };

        class league_search_results_object : public graphene::db::abstract_object<league_search_results_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_league_search_results_object_type;

            uint64_t total;
            uint64_t filtered_total;
            vector <league_object> data;
        };

        struct by_data_market_category_id;
        struct by_recommend_expiration_date_time;
        /**
         * @ingroup object_index
         */
        using league_multi_index_type = multi_index_container<
           league_object,
           indexed_by<
              ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
              ordered_non_unique< tag<by_data_market_category_id>,
                composite_key<
                   league_object,
                     member<league_object, data_market_category_id_type,  &league_object::category_id>
                >
              >,
              ordered_non_unique< tag<by_recommend_expiration_date_time>,
                composite_key<
                   league_object,
                    member<league_object, time_point_sec,  &league_object::recommend_expiration_date_time>
                >
              >
           >
        >;
        /**
         * @ingroup object_index
         */
        using league_index = generic_index<league_object, league_multi_index_type>;

} }

FC_REFLECT_DERIVED(graphene::chain::league_object,
                   (graphene::db::object),
                   (league_name)(brief_desc)
                   (data_products)(prices)(status)
                   (category_id)(icon)(issuer)(create_date_time)(recommend_expiration_date_time)(total)(members)(pocs_thresholds)(fee_bases)(product_pocs_weights))

FC_REFLECT_DERIVED(graphene::chain::league_search_results_object,
                   (graphene::db::object),
                   (total)(filtered_total)(data))
