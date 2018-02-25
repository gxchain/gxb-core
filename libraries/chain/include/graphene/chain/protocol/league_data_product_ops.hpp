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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/league_data_product_object.hpp>

namespace graphene { namespace chain {

    struct league_data_product_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        fc::string                          product_name;
        fc::string                          brief_desc;
        data_market_category_id_type        category_id;
        uint64_t                            refer_price;
        fc::string                          icon;
        vector<schema_context_object>       schema_contexts;
        account_id_type                     issuer;
        time_point_sec                      create_date_time;;
        asset                               fee;
        extensions_type                     extensions;
        // time_point_sec                      recommend_expiration_date_time;
        account_id_type fee_payer() const { return issuer; }
        void validate() const {}
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }

    };

    struct league_data_product_update_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee  = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_data_product_id_type                  league_data_product;
        optional < fc::string >                      new_product_name;
        optional < fc::string >                      new_brief_desc;
        optional < data_market_category_id_type >    new_category_id;
        optional < uint64_t >                        new_refer_price;
        optional < uint8_t >                         new_status;
        optional < fc::string >                      new_icon;
        optional < vector<schema_context_object> >   new_schema_contexts;
        asset                                        fee;
        extensions_type                              extensions;
        account_id_type fee_payer() const { return account_id_type(); }
        void validate() const {
            FC_ASSERT(fee.amount >= 0);
        }
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

} } // graphene::chain

FC_REFLECT( graphene::chain::league_data_product_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::league_data_product_update_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::league_data_product_create_operation,
            (product_name)
            (brief_desc)
            (category_id)
            (refer_price)
            (icon)
            (schema_contexts)
            (fee)
            (create_date_time)
            (issuer)
            (extensions)
)

FC_REFLECT( graphene::chain::league_data_product_update_operation,
            (league_data_product)
            (new_product_name)
            (new_brief_desc)
            (new_category_id)
            (new_refer_price)
            (new_status)
            (new_icon)
            (new_schema_contexts)
            (fee)
            (extensions)
)
