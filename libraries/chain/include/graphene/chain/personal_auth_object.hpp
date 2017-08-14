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
#include <vector>

namespace graphene {
    namespace chain {

    using namespace std;

        class personal_auth_object : public graphene::db::abstract_object<personal_auth_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = personal_auth_object_type;

            string              auth_context;
            time_point_sec      create_date_time;
            account_id_type     issuer;
        };

        /**
         * @ingroup object_index
         */
        using personal_auth_multi_index_type = multi_index_container<
           personal_auth_object,
           indexed_by<
              ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >
           >
        >;

        /**
         * @ingroup object_index
         */
        using personal_auth_index = generic_index<personal_auth_object, personal_auth_multi_index_type>;
} }

FC_REFLECT_DERIVED(graphene::chain::personal_auth_object,
                   (graphene::db::object),
                   (auth_context)(create_date_time)(issuer)
)
