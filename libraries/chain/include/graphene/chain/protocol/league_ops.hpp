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
#include <graphene/chain/league_object.hpp>

namespace graphene { namespace chain {

    struct league_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_id_type                       league;
        fc::string                           league_name;
        fc::string                           brief_desc;
        vector<league_data_product_id_type>  data_products;
        vector<uint64_t>                     prices;
        data_market_category_id_type         category_id;
        fc::string                           icon;
        uint64_t                             total;
        asset                                fee;
        account_id_type                      issuer;
        time_point_sec                       create_date_time;;
        time_point_sec                       recommend_expiration_date_time;
        vector<account_id_type>              members;
        extensions_type                      extensions;

        account_id_type fee_payer() const { return issuer; }
        void validate() const {}
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

    // updated by committee_member
    struct league_update_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee  = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_id_type                                  league;
        optional <fc::string>                           new_league_name;
        optional <fc::string>                           new_brief_desc;
        optional <vector<league_data_product_id_type>>  new_data_products;
        optional <vector<uint64_t>>                     new_prices;
        optional <uint8_t>                              new_status;
        optional <data_market_category_id_type>         new_category_id;
        optional <fc::string>                           new_icon;
        optional <account_id_type>                      new_issuer;
        uint64_t                                        new_total;
        asset                                           fee;
        optional < time_point_sec >                     new_recommend_expiration_date_time;
        optional <vector<account_id_type>>              new_members;
        extensions_type                                 extensions;

        account_id_type fee_payer() const {
            return account_id_type();
        }
        void validate() const {
            FC_ASSERT(fee.amount >= 0);
        }
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

} } // graphene::chain

FC_REFLECT( graphene::chain::league_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::league_update_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::league_create_operation,
            (league)
            (league_name)
            (brief_desc)
            (data_products)
            (prices)
            (category_id)
            (icon)
            (fee)
            (issuer)
            (create_date_time)
            (recommend_expiration_date_time)
            (members)
            (extensions)
)
FC_REFLECT( graphene::chain::league_update_operation,
            (league)
            (new_league_name)
            (new_brief_desc)
            (new_data_products)
            (new_prices)
            (new_status)
            (new_category_id)
            (new_icon)
            (new_issuer)
            (fee)
            (new_recommend_expiration_date_time)
            (new_members)
            (extensions)
)
