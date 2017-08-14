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
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/chain/protocol/buyback.hpp>
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/special_authority.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/data_market_object.hpp>
#include <graphene/chain/free_data_product_object.hpp>
#include <graphene/chain/league_object.hpp>
#include <graphene/chain/league_data_product_object.hpp>

namespace graphene { namespace chain { 
    struct stale_data_market_category_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        string                  category_name;
        uint8_t                 data_market_type;
        uint32_t                order_num;
        asset                   fee;
        account_id_type         issuer;
        time_point_sec          create_date_time;

        account_id_type fee_payer() const { return issuer; }
        void validate() const {}
        share_type      calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

    struct stale_data_market_category_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };
        optional< string >              new_category_name;
        optional< uint32_t >            new_order_num;
        data_market_category_id_type    data_market_category;
        uint8_t                         data_market_type;
        asset                           fee;
        optional< account_id_type >     new_issuer;
        optional< uint8_t >             new_status;

        account_id_type fee_payer() const { return *new_issuer; }
        void validate() const {}
        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

    struct stale_free_data_product_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        string                          product_name;
        string                          brief_desc;
        account_id_type                 datasource;
        data_market_category_id_type    category_id;
        uint64_t                        price;
        string                          icon;
        vector<schema_context_object>   schema_contexts;
        string       parent_id;
        account_id_type                 issuer;
        asset                           fee;
        time_point_sec                  create_date_time;;
        time_point_sec                  recommend_expiration_date_time;

        account_id_type fee_payer()const { return issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }
    };

    struct stale_free_data_product_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        free_data_product_id_type                    free_data_product;
        optional < string >                          new_product_name;
        optional < string >                          new_brief_desc;
        optional <account_id_type>                   new_datasource;
        optional < data_market_category_id_type >    new_category_id;
        optional < uint64_t >                        new_price;
        optional < string >                          new_icon;
        optional < vector<schema_context_object> >   new_schema_contexts;
        optional < string >       new_parent_id;
        optional < uint8_t >                         new_status;
        optional < account_id_type >                 new_issuer;
        optional < time_point_sec >                  new_recommend_expiration_date_time;
        asset                                        fee;

        account_id_type fee_payer()const { return *new_issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }
    };

    struct stale_league_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_id_type                       league;
        string                               league_name; 
        string                               brief_desc; 
        vector<league_data_product_id_type>  data_products;
        vector<uint64_t>                     prices;
        data_market_category_id_type         category_id;
        string                               icon;
        uint64_t                             total;
        asset                                fee;
        account_id_type                      issuer;
        time_point_sec                       create_date_time;;
        time_point_sec                       recommend_expiration_date_time;

        account_id_type fee_payer()const { return issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }
    };

    struct stale_league_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_id_type                                  league;
        optional <string>                               new_league_name; 
        optional <string>                               new_brief_desc; 
        optional <vector<league_data_product_id_type>>  new_data_products;
        optional <vector<uint64_t>>                     new_prices;
        optional <uint8_t>                              new_status;
        optional <data_market_category_id_type>         new_category_id;
        optional <string>                               new_icon;
        optional <account_id_type>                      new_issuer;
        uint64_t                                        new_total;
        asset                                           fee;
        optional < time_point_sec >                     new_recommend_expiration_date_time;

        account_id_type fee_payer()const { return *new_issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }
    };
    struct stale_league_data_product_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        string                              product_name;
        string                              brief_desc;
        data_market_category_id_type        category_id;
        uint64_t                            refer_price;
        string                              icon;
        vector<schema_context_object>       schema_contexts;
        account_id_type                     issuer;
        time_point_sec                      create_date_time;;
        asset                               fee;
        // time_point_sec                      recommend_expiration_date_time;
        
        account_id_type fee_payer()const { return issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }

    };
    struct stale_league_data_product_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        league_data_product_id_type                  league_data_product;
        optional < string >                          new_product_name;
        optional < string >                          new_brief_desc;
        optional < data_market_category_id_type >    new_category_id;
        optional < uint64_t >                        new_refer_price;
        optional < uint8_t >                         new_status;
        optional < string >                          new_icon;
        optional < vector<schema_context_object> >  new_schema_contexts;
        optional < account_id_type >                 new_issuer;
        asset                                        fee;
        // optional < time_point_sec >                  new_recommend_expiration_date_time;
        
        account_id_type fee_payer()const { return *new_issuer; }
        void validate()const{

        }
        share_type      calculate_fee( const fee_parameters_type& k ) const{
            return k.fee;
        }
    };



} } // graphene::chain

FC_REFLECT( graphene::chain::stale_data_market_category_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_data_market_category_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_free_data_product_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_free_data_product_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_league_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_league_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_league_data_product_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::stale_league_data_product_update_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::stale_data_market_category_create_operation,
            (category_name)
            (data_market_type)
            (order_num)
            (fee)
            (issuer)
            (create_date_time)
)
FC_REFLECT( graphene::chain::stale_data_market_category_update_operation,
            (new_category_name)
            (new_order_num)
            (data_market_category)
            (data_market_type)
            (fee)
            (new_issuer)
            (new_status)
)
FC_REFLECT( graphene::chain::stale_free_data_product_create_operation,
            (product_name)
            (brief_desc)
            (datasource)
            (category_id)
            (price)
            (icon)
            (schema_contexts)
            (parent_id)
            (fee)
            (issuer)
            (create_date_time)
            (recommend_expiration_date_time)
)

FC_REFLECT( graphene::chain::stale_free_data_product_update_operation,
            (free_data_product)
            (new_product_name)
            (new_brief_desc)
            (new_datasource)
            (new_category_id)
            (new_price)
            (new_icon)
            (new_schema_contexts)
            (new_parent_id)
            (new_status)
            (new_issuer)
            (new_recommend_expiration_date_time)
            (fee)
)
FC_REFLECT( graphene::chain::stale_league_create_operation,
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
)
FC_REFLECT( graphene::chain::stale_league_update_operation,
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
)
FC_REFLECT( graphene::chain::stale_league_data_product_create_operation,
            (product_name)
            (brief_desc)
            (category_id)
            (refer_price)
            (icon)
            (schema_contexts)
            (fee)
            (create_date_time)
            (issuer)
)

FC_REFLECT( graphene::chain::stale_league_data_product_update_operation,
            (league_data_product)
            (new_product_name)
            (new_brief_desc)
            (new_category_id)
            (new_refer_price)
            (new_status)
            (new_icon)
            (new_schema_contexts)
            (fee)
            (new_issuer)
)
