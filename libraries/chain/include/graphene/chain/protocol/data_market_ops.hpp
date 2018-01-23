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
#include <graphene/chain/data_market_object.hpp>

namespace graphene { namespace chain {

    struct data_market_category_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        fc::string              category_name;
        uint8_t                 data_market_type;
        uint32_t                order_num;
        asset                   fee;
        account_id_type         issuer;
        time_point_sec          create_date_time;
        extensions_type         extensions;
        account_id_type fee_payer()const { return issuer; }
        void            validate() const {}
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }

    };

    struct data_market_category_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 0;
        };

        optional< fc::string >          new_category_name;
        optional< uint32_t >            new_order_num;
        data_market_category_id_type    data_market_category;
        extensions_type                 extensions;
        uint8_t                         data_market_type;
        asset                           fee;
        optional< uint8_t >             new_status;
        account_id_type fee_payer() const { return account_id_type(); }
        void validate()const{
            FC_ASSERT(fee.amount >= 0);
        }
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };


} } // graphene::chain

FC_REFLECT( graphene::chain::data_market_category_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::data_market_category_update_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::data_market_category_create_operation,
            (category_name)
            (data_market_type)
            (order_num)
            (fee)
            (issuer)
            (create_date_time)
            (extensions)
)
FC_REFLECT( graphene::chain::data_market_category_update_operation,
            (new_category_name)
            (new_order_num)
            (data_market_category)
            (data_market_type)
            (fee)
            (new_status)
            (extensions)
)
