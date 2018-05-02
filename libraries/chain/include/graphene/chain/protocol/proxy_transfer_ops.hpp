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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/ext.hpp>

namespace graphene { namespace chain {
    struct proxy_transfer_params
    {
        account_id_type         from;
        account_id_type         to;
        account_id_type         proxy_account;
        uint16_t                percentage = 0;
        asset                   amount;
        fc::string              memo;
        fc::time_point_sec      expiration;
        vector<signature_type>  signatures;

        bool verify_proxy_transfer_signature(const fc::ecc::public_key& expected_signee) const
        {
            auto p = *this;
            p.signatures.clear();
            digest_type::encoder enc;
            fc::raw::pack(enc, p);

            for (const auto& sig : this->signatures) {
                if (fc::ecc::public_key(sig, enc.result(), true) == expected_signee) {
                    return true;
                }
            }
            return false;
        }
    };

    struct proxy_transfer_operation : public base_operation {
        struct fee_parameters_type {
            uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        };

        fc::string                      proxy_memo;
        asset                           fee;
        proxy_transfer_params           request_params;
        extensions_type                 extensions;

        account_id_type fee_payer() const { return request_params.proxy_account; }
        account_id_type get_from_account() const { return request_params.from; }
        account_id_type get_to_account() const { return request_params.to; }
        account_id_type get_proxy_account() const { return request_params.proxy_account; }

        void validate() const
        {
            FC_ASSERT(fee.amount >= 0);
            FC_ASSERT(request_params.amount.amount >= 0);
            FC_ASSERT(proxy_memo.size() > 0);
            FC_ASSERT(request_params.percentage >= 0 && request_params.percentage <= GRAPHENE_100_PERCENT);
            FC_ASSERT(get_from_account() != get_to_account());
        }

        share_type calculate_fee(const fee_parameters_type &k) const
        {
            return k.fee;
        }
    };


} } // graphene::chain

FC_REFLECT(graphene::chain::proxy_transfer_params,
            (from)
            (to)
            (proxy_account)
            (amount)
            (percentage)
            (memo)
            (expiration)
            (signatures))

FC_REFLECT(graphene::chain::proxy_transfer_operation::fee_parameters_type, (fee))

FC_REFLECT(graphene::chain::proxy_transfer_operation,
            (proxy_memo)
            (fee)
            (request_params)
            (extensions))
