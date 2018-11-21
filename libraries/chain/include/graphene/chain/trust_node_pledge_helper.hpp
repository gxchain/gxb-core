#pragma once
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

class trust_node_pledge_helper
{
public:
    static void do_evaluate(database& db, const witness_create_operation& op);
    static void do_apply(database& db, const witness_create_operation& op);

    static void do_evaluate(database& db, const witness_update_operation& op);
    static void do_apply(database& db, const witness_update_operation& op);

    static void do_evaluate(database& db, const trust_node_pledge_withdraw_operation& op);
    static void do_apply(database& db, const trust_node_pledge_withdraw_operation& op);

    static void do_evaluate(database& db, const committee_member_create_operation& op);
    static void do_apply(database& db, const committee_member_create_operation& op);

    static void do_evaluate(database& db, const committee_member_update_operation& op);
    static void do_apply(database& db, const committee_member_update_operation& op);


private:
    static void reset();
    static void do_evaluate(database& db, const account_id_type& account_id);
    static void do_apply(database& db, const account_id_type& account_id);

private:
    static bool                       pledge_object_exist;
    static trust_node_pledge_object*  pledge_obj_ptr;
    static witness_object*            witness_obj_ptr;
    static committee_member_object*   committee_obj_ptr;
    static asset                      pledge_to_charge;
};

} }
