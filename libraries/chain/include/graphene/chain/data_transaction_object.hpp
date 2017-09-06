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
    class league_object;

        struct  data_transaction_datasource_status_object
        {
            account_id_type     datasource;
            uint8_t             status = 0;
        };

        /**
         * 查询数据对象
         * @brief
         *
         *
         */
        class data_transaction_object : public graphene::db::abstract_object<data_transaction_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = data_transaction_object_type;

            string                                  request_id;
            object_id_type                          product_id;
            string                                  version;
            string                                  params;
            // 跟踪查询的处理状态
            uint8_t                                 status;
            // 查询数据的创建时间
            time_point_sec                          create_date_time;
            // 查询的请求者
            account_id_type                         requester;
            fc::optional<league_id_type>            league_id = fc::optional<league_id_type>();
            string                                  memo;
            vector<data_transaction_datasource_status_object>             datasources_status;
            // product fee
            uint64_t                                product_pay = 0;
            // Data transaction fee
            uint64_t                                transaction_fee = 0;
            // commission
            uint64_t                                commission = 0;
        };

        // data_transaction_object 的排序函数
        struct sort_data_transaction_object_by_create_date_time {
            bool operator() (const data_transaction_object &l, const data_transaction_object &r) {
                return l.create_date_time > r.create_date_time;
            }
        };

        class data_transaction_search_results_object : public graphene::db::abstract_object<data_transaction_search_results_object> {
        public:
            static const uint8_t space_id = implementation_ids;
            static const uint8_t type_id = impl_data_transaction_search_results_object_type;

            uint64_t total;
            vector <data_transaction_object> data;
        };

        /**
         * @ingroup object_index
         */
        struct by_request_id {};
        struct by_requester {};
        using data_transaction_multi_index_type = multi_index_container<
            data_transaction_object,
            indexed_by<
                ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
                ordered_unique< tag<by_request_id>, member<data_transaction_object, string, &data_transaction_object::request_id> >,
                ordered_non_unique< tag<by_requester>, 
                    composite_key<
                        data_transaction_object,
                         member<data_transaction_object, account_id_type, &data_transaction_object::requester>
                     >
                >
              >
            >;
        /**
         * @ingroup object_index
         */
        using data_transaction_index = generic_index<data_transaction_object, data_transaction_multi_index_type>;




    }
}

FC_REFLECT( graphene::chain::data_transaction_datasource_status_object,
            (datasource)
            (status)
)

FC_REFLECT_DERIVED(graphene::chain::data_transaction_object,
                   (graphene::db::object),
                   (request_id)
                   (product_id)
                   (version)
                   (params)
                   (status)
                   (create_date_time)
                   (requester)
                   (league_id)
                   (memo)
                   (datasources_status)
                   (product_pay)
                   (transaction_fee)
                   (commission))
FC_REFLECT_DERIVED(graphene::chain::data_transaction_search_results_object,
                   (graphene::db::object),
                   (total)
                   (data))
