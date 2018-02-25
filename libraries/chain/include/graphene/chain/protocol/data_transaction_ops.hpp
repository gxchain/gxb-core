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
#include <graphene/chain/protocol/buyback.hpp>
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/special_authority.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/data_transaction_object.hpp>

namespace graphene { namespace chain {

    struct data_transaction_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        };
        fc::string                                  request_id;
        object_id_type                              product_id;
        fc::string                                  version;
        fc::string                                  params;
        asset                                       fee;
        account_id_type                             requester;
        time_point_sec                              create_date_time;
        fc::optional<league_id_type>                league_id = fc::optional<league_id_type>();
        extensions_type                             extensions;

        account_id_type fee_payer() const { return requester; }
        void validate() const { }

        fc::string get_request_id() const {
            return request_id;
        }

        share_type calculate_fee( const fee_parameters_type& k ) const {
            return k.fee;
        }
    };

    struct data_transaction_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 0;
        };
        fc::string                                      request_id;
        uint8_t                                         new_status = 0;
        account_id_type                                 new_requester;
        asset                                           fee;
        fc::optional<fc::string>                        new_memo = fc::optional<fc::string>();
        extensions_type                                 extensions;


        account_id_type fee_payer() const { return new_requester; }
        void            validate() const{ }
        share_type      calculate_fee(const fee_parameters_type& k) const{
            return k.fee;
        }
    };

    struct data_transaction_datasource_upload_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 0;
        };
        fc::string                                      request_id;
        account_id_type                                 requester;
        account_id_type                                 datasource;
        asset                                           fee;
        extensions_type                                 extensions;

        account_id_type fee_payer() const { return requester; }
        void            validate() const{ }
        share_type      calculate_fee(const fee_parameters_type& k) const{
            return k.fee;
        }
    };

    struct data_transaction_datasource_validate_error_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = 0;
        };
        fc::string                                      request_id;
        account_id_type                                 datasource;
        asset                                           fee;
        extensions_type                                 extensions;

        account_id_type fee_payer() const { return datasource; }
        void            validate() const{ }
        share_type      calculate_fee(const fee_parameters_type& k) const{
            return k.fee;
        }
    };

    struct data_transaction_complain_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee = 0;
        };
        fc::string                                      request_id;
        account_id_type                                 requester;
        account_id_type                                 datasource;
        asset                                           fee;
        fc::string                                      merchant_copyright_hash;
        fc::string                                      datasource_copyright_hash;
        time_point_sec                                  create_date_time;
        extensions_type                                 extensions;

        account_id_type fee_payer() const { return requester; }
        void            validate() const{ }
        share_type      calculate_fee(const fee_parameters_type& k) const{
            return k.fee;
        }
    };

} } // graphene::chain
FC_REFLECT( graphene::chain::data_transaction_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::data_transaction_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::data_transaction_datasource_upload_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::data_transaction_datasource_validate_error_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::data_transaction_complain_operation::fee_parameters_type, (fee) )


FC_REFLECT( graphene::chain::data_transaction_create_operation,
            (request_id)
            (product_id)
            (version)
            (params)
            (fee)
            (requester)
            (create_date_time)
            (league_id)
            (extensions)
)
FC_REFLECT( graphene::chain::data_transaction_update_operation,
            (request_id)
            (new_status)
            (fee)
            (new_requester)
            (new_memo)
            (extensions)
)
FC_REFLECT( graphene::chain::data_transaction_datasource_upload_operation,
            (request_id)
            (requester)
            (datasource)
            (fee)
            (extensions)
)
FC_REFLECT( graphene::chain::data_transaction_datasource_validate_error_operation,
            (request_id)
            (datasource)
            (fee)
            (extensions)
)

FC_REFLECT( graphene::chain::data_transaction_complain_operation,
            (request_id)
            (requester)
            (datasource)
            (fee)
            (merchant_copyright_hash)
            (datasource_copyright_hash)
            (create_date_time)
            (extensions)
)
