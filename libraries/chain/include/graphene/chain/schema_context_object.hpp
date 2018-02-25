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
        /*
         * schema object for free_data_product and league_data_product
         */
        struct  schema_context_object
        {
            fc::string      version;
            fc::string      schema_context;;

            schema_context_object& operator= (const schema_context_object &obj) {
                this->version = obj.version;
                this->schema_context = obj.schema_context;
                return *this;
            }

            friend bool operator< (const schema_context_object& l, const schema_context_object& r) {
                return l.version < r.version;
            }

            // for sort
            friend bool  operator ==  (const schema_context_object& l, const schema_context_object& r) {
                return l.version == r.version;
            }
        };

}}

FC_REFLECT( graphene::chain::schema_context_object,
            (version)
            (schema_context)
)
