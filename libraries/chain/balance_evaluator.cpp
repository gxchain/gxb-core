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
#include <graphene/chain/balance_evaluator.hpp>

namespace graphene { namespace chain {

void_result balance_claim_evaluator::do_evaluate(const balance_claim_operation& op)
{
   database& d = db();
   balance = &op.balance_to_claim(d);

   GRAPHENE_ASSERT(
             op.balance_owner_key == balance->owner ||
             pts_address(op.balance_owner_key, false, 56) == balance->owner ||
             pts_address(op.balance_owner_key, true, 56) == balance->owner ||
             pts_address(op.balance_owner_key, false, 0) == balance->owner ||
             pts_address(op.balance_owner_key, true, 0) == balance->owner,
             balance_claim_owner_mismatch,
             "Balance owner key was specified as '${op}' but balance's actual owner is '${bal}'",
             ("op", op.balance_owner_key)
             ("bal", balance->owner)
             );
   if( !(d.get_node_properties().skip_flags & (database::skip_authority_check |
                                               database::skip_transaction_signatures)) )

   FC_ASSERT(op.total_claimed.asset_id == balance->asset_type());

   if( balance->is_vesting_balance() )
   {
      GRAPHENE_ASSERT(
         balance->vesting_policy->is_withdraw_allowed(
            { balance->balance,
              d.head_block_time(),
              op.total_claimed } ),
         balance_claim_invalid_claim_amount,
         "Attempted to claim ${c} from a vesting balance with ${a} available",
         ("c", op.total_claimed)("a", balance->available(d.head_block_time()))
         );
      GRAPHENE_ASSERT(
         d.head_block_time() - balance->last_claim_date >= fc::days(1),
         balance_claim_claimed_too_often,
         "Genesis vesting balances may not be claimed more than once per day."
         );
      return {};
   }

   FC_ASSERT(op.total_claimed == balance->balance);
   return {};
}

/**
 * @note the fee is always 0 for this particular operation because once the
 * balance is claimed it frees up memory and it cannot be used to spam the network
 */
void_result balance_claim_evaluator::do_apply(const balance_claim_operation& op)
{
   database& d = db();

   if( balance->is_vesting_balance() && op.total_claimed < balance->balance )
      d.modify(*balance, [&](balance_object& b) {
         b.vesting_policy->on_withdraw({b.balance, d.head_block_time(), op.total_claimed});
         b.balance -= op.total_claimed;
         b.last_claim_date = d.head_block_time();
      });
   else
      d.remove(*balance);

   d.adjust_balance(op.deposit_to_account, op.total_claimed);
   return {};
}

void_result balance_locked_evaluator::do_evaluate(const balance_locked_operation& op)
{
    database& _db = db();
    auto& index = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>();
    auto itr = index.find(boost::make_tuple(op.locked_account, op.asset_type));
    FC_ASSERT(itr != index.end(),"account id or asset id error");
    const chain_parameters& chain_params = _db.get_global_properties().parameters;
    vector< pair<string, interest_rate_t> > params;
    for (auto& ext : chain_params.extensions) {
        if (ext.which() == future_extensions::tag<lock_balance_params_t>::value) {
            params = ext.get<lock_balance_params_t>().params;
            break;
        }
    }
    FC_ASSERT(params.size() != 0, "no lock balance params");
    auto iter_param = find_if(params.begin(), params.end(), [&](pair<string, interest_rate_t> p) {
        return p.first == op.locked_time_type;
    } );
    FC_ASSERT(iter_param != params.end(), "locked_time_type invalid");
    FC_ASSERT(iter_param->second.interest_rate == op.interest_rate, "input interest_rate invalid");
    locked_balance_time = iter_param->second.interest_rate_days;
    FC_ASSERT(op.locked_balance <= itr->get_balance().amount, "balance of the asset is not enough");
    FC_ASSERT(op.create_time >= _db.head_block_time() - chain_params.maximum_time_until_expiration);
    return {};
}

object_id_type balance_locked_evaluator::do_apply(const balance_locked_operation& op)
{
    database& _db = db();
    const auto& new_object = _db.create<account_balance_locked_object>([&](account_balance_locked_object& b){
        b.owner = op.locked_account;
        b.asset_type = op.asset_type;
        b.create_time = op.create_time;
        b.locked_balance_time = locked_balance_time;
        b.locked_time_type = op.locked_time_type;
        b.locked_balance = op.locked_balance;
        b.interest_rate = op.interest_rate;
        b.memo = op.memo;
    });
    asset locked_asset;
    locked_asset.amount = op.locked_balance;
    locked_asset.asset_id = op.asset_type;
    _db.adjust_balance(op.locked_account, -locked_asset);
    return new_object.id;
}

void_result balance_unlocked_evaluator::do_evaluate(const balance_unlocked_operation& op)
{
    database& _db = db();
    account_balance_locked_obj = &op.account_balance_locked(_db);
    FC_ASSERT(nullptr != account_balance_locked_obj, "invalid account_balance_locked_id");
    FC_ASSERT(account_balance_locked_obj->owner == op.unlocked_account, "unlock_balance account does not match the account of the balance_unlocked_operation");
    //Whether locked time has expired
    uint32_t caculate_locked_time = (_db.head_block_time().sec_since_epoch() - account_balance_locked_obj->create_time.sec_since_epoch()) / SECONDS_PER_DAY;
    //T+1 mode
    FC_ASSERT(account_balance_locked_obj->locked_balance_time <= caculate_locked_time - 1, "Not expired");
    return {};
}

void_result balance_unlocked_evaluator::do_apply(const balance_unlocked_operation& op)
{
    database& _db = db();
    if (nullptr != account_balance_locked_obj){
        asset locked_asset;
        locked_asset.amount = account_balance_locked_obj->locked_balance;
        locked_asset.asset_id = account_balance_locked_obj->asset_type;
        _db.adjust_balance(op.unlocked_account, locked_asset);
        //remove no use account_balance_locked_object
        _db.remove(*account_balance_locked_obj);
    }
    return {};
}
} } // namespace graphene::chain
