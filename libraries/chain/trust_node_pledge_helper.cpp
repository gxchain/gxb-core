#include <graphene/chain/trust_node_pledge_helper.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

namespace graphene { namespace chain {

bool                        trust_node_pledge_helper::pledge_object_exist = false;
trust_node_pledge_object*   trust_node_pledge_helper::pledge_obj_ptr      = nullptr;
witness_object*             trust_node_pledge_helper::witness_obj_ptr     = nullptr;
committee_member_object*    trust_node_pledge_helper::committee_obj_ptr   = nullptr;
asset                       trust_node_pledge_helper::pledge_to_charge    = asset{0, asset_id_type(1)};

void trust_node_pledge_helper::do_evaluate(database& db, const witness_create_operation& op)
{
	reset();
	do_evaluate(db, op.witness_account);
}

void trust_node_pledge_helper::do_apply(database& db, const witness_create_operation& op)
{
	do_apply(db, op.witness_account);
}

void trust_node_pledge_helper::do_evaluate(database& db, const witness_update_operation& op)
{
	reset();
	do_evaluate(db, op.witness_account);
}

void trust_node_pledge_helper::do_apply(database& db, const witness_update_operation& op)
{
	do_apply(db, op.witness_account);
	db.modify(
	   db.get(op.witness),
	   [](witness_object &witness_obj) {
		   witness_obj.is_valid = true;
	   }
	);
}

void trust_node_pledge_helper::do_evaluate(database& db, const trust_node_pledge_withdraw_operation& op)
{
	reset();

    auto &pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
    auto pledge_itr = pledge_idx.find(op.witness_account);
    FC_ASSERT(pledge_itr != pledge_idx.end(), "have no pledge");
    pledge_obj_ptr = const_cast<trust_node_pledge_object*>(&(*pledge_itr));

    auto &witness_idx = db.get_index_type<witness_index>().indices().get<by_account>();
    auto witness_itr = witness_idx.find(op.witness_account);
    if(witness_itr != witness_idx.end()) {
    	witness_obj_ptr = const_cast<witness_object*>(&(*witness_itr));
    }

    auto &committee_idx = db.get_index_type<committee_member_index>().indices().get<by_account>();
    auto committee_itr = committee_idx.find(op.witness_account);
    if(committee_itr != committee_idx.end()) {
        committee_obj_ptr = const_cast<committee_member_object*>(&(*committee_itr));
    }

    FC_ASSERT(witness_itr != witness_idx.end() || committee_itr != committee_idx.end(), "witness and committee of this account not exist");
}

void trust_node_pledge_helper::do_apply(database& db, const trust_node_pledge_withdraw_operation& op)
{
    db.deposit_lazy_vesting(
        optional<vesting_balance_id_type>(),
		pledge_obj_ptr->amount.amount,
        db.get_global_properties().parameters.cashback_vesting_period_seconds,
        op.witness_account, true
    );

    db.remove(*pledge_obj_ptr);

    if(witness_obj_ptr != nullptr) {
        db.modify(
            *witness_obj_ptr,
            [](witness_object &witness_obj) {
                witness_obj.is_valid = false;
            }
        );
    }

    if(committee_obj_ptr != nullptr) {
        db.modify(
            *committee_obj_ptr,
            [](committee_member_object &committee_obj) {
        	committee_obj.is_valid = false;
            }
        );
    }
}

void trust_node_pledge_helper::do_evaluate(database& db, const committee_member_create_operation& op)
{
	reset();
	do_evaluate(db, op.committee_member_account);
}
void trust_node_pledge_helper::do_apply(database& db, const committee_member_create_operation& op)
{
	do_apply(db, op.committee_member_account);
}

void trust_node_pledge_helper::do_evaluate(database& db, const committee_member_update_operation& op)
{
	reset();
	do_evaluate(db, op.committee_member_account);
}

void trust_node_pledge_helper::do_apply(database& db, const committee_member_update_operation& op)
{
	do_apply(db, op.committee_member_account);
	db.modify(
	   db.get(op.committee_member),
	   [](committee_member_object &committee_obj) {
		committee_obj.is_valid = true;
	   }
	);
}

void trust_node_pledge_helper::reset()
{
    pledge_object_exist = false;
    pledge_obj_ptr = nullptr;
    witness_obj_ptr = nullptr;
    committee_obj_ptr = nullptr;
    pledge_to_charge = asset(0, asset_id_type(1));
}

inline void trust_node_pledge_helper::do_evaluate(database& db, const account_id_type& account_id)
{
	int64_t pledge_needed = db.get_trust_node_pledge().amount;
	FC_ASSERT(pledge_needed >= 0, "trust node pledge must >= 0");

	const auto &pledge_idx = db.get_index_type<trust_node_pledge_index>().indices().get<by_account>();
	auto pledge_itr = pledge_idx.find(account_id);
   if(pledge_itr == pledge_idx.end()) {
	   pledge_to_charge = asset{pledge_needed, asset_id_type(1)};
   } else {
	   pledge_object_exist = true;
	   pledge_to_charge = asset{pledge_needed, asset_id_type(1)} - pledge_itr->amount;
	   pledge_obj_ptr = const_cast<trust_node_pledge_object*>(&(*pledge_itr));
   }

   FC_ASSERT(pledge_to_charge.amount <= 0 || db.get_balance(account_id, asset_id_type(1)) >= pledge_to_charge, "account balance not enough");
}

inline void trust_node_pledge_helper::do_apply(database& db, const account_id_type& account_id)
{
    if(pledge_to_charge.amount != 0) {
        if(pledge_object_exist) {
			db.modify(*pledge_obj_ptr, [](trust_node_pledge_object &obj) {
			    obj.amount += pledge_to_charge;
			});
        } else {
            db.create<trust_node_pledge_object>([&account_id](trust_node_pledge_object& obj) {
                obj.owner_account  = account_id;
                obj.amount         = pledge_to_charge;
            });
        }

        db.adjust_balance(account_id, -pledge_to_charge);
    }
}

} }
