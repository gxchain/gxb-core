/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/exceptions.hpp>

namespace graphene { namespace chain {

#define LOCKED_BALANCE_EXPIRED_TIME 30

class balance_claim_evaluator : public evaluator<balance_claim_evaluator>
{
public:
   typedef balance_claim_operation operation_type;

   const balance_object* balance = nullptr;

   void_result do_evaluate(const balance_claim_operation& op);

   /**
    * @note the fee is always 0 for this particular operation because once the
    * balance is claimed it frees up memory and it cannot be used to spam the network
    */
   void_result do_apply(const balance_claim_operation& op);
};

class balance_locked_evaluator : public evaluator<balance_locked_evaluator>
{
public:
    typedef balance_lock_operation operation_type;

    uint32_t locked_balance_time = 0;

    void_result do_evaluate(const balance_lock_operation& op);

    object_id_type do_apply(const balance_lock_operation& op);
};

class balance_unlocked_evaluator : public evaluator<balance_unlocked_evaluator>
{
public:
    typedef balance_unlock_operation operation_type;

    const account_balance_locked_object* account_balance_locked_obj = nullptr;

    void_result do_evaluate(const balance_unlock_operation& op);
    
    void_result do_apply(const balance_unlock_operation& op);
};


} } // graphene::chain
