/*
    Copyright (C) 2018 gxb

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

        /**
         * Query the data_storge_baas_object
         * @brief
         */
        class data_storage_baas_object : public graphene::db::abstract_object<data_storage_baas_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = data_storage_baas_object_type;

            // user's signature who use baas service
            signature_type              signature;
            // expiration time
            fc::time_point_sec          expiration;
        };

        /**
         * @ingroup object_index
         */
        struct by_signature {};
        struct by_expiration {};
        using data_storage_baas_multi_index_type = multi_index_container<
            data_storage_baas_object,
            indexed_by<
                ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
                ordered_unique< tag<by_signature>, member<data_storage_baas_object, signature_type, &data_storage_baas_object::signature> >,
                ordered_non_unique< tag<by_expiration>, member<data_storage_baas_object, time_point_sec, &data_storage_baas_object::expiration > >
             >
        >;
        /**
         * @ingroup object_index
         */
        using data_storage_index = generic_index<data_storage_baas_object, data_storage_baas_multi_index_type>;

    }
}

FC_REFLECT_DERIVED(graphene::chain::data_storage_baas_object,
                   (graphene::db::object),
                   (signature)
                   (expiration))
