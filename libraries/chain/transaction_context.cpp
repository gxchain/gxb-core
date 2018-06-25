#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/transaction_object.hpp>

namespace graphene { namespace chain {

   transaction_context::transaction_context() :
        start(fc::time_point::now()),
        _deadline(start + config::cpu_duration_limit),
        transaction_cpu_usage_us(0)
   {
   }

   void transaction_context::init()
   {
   }

   void transaction_context::init_for_implicit_trx()
   {
   }

   void transaction_context::init_for_input_trx()
   {
   }

   void transaction_context::init_for_deferred_trx()
   {
   }

   void transaction_context::exec()
   {
   }

   void transaction_context::finalize()
   {
   }

   void transaction_context::squash()
   {
   }

   void transaction_context::checktime() const
   {
       auto now = fc::time_point::now();
       transaction_cpu_usage_us = (now - start).count();//TODO
       if (BOOST_UNLIKELY(now > _deadline)) {
           GRAPHENE_THROW(tx_cpu_usage_exceeded,
                          "transaction was executing for too long",
                          ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - start));
       }
   }

   void transaction_context::pause_billing_timer()
   {
   }

   void transaction_context::resume_billing_timer()
   {
   }

   void transaction_context::validate_cpu_usage_to_bill() const
   {
   }

   void transaction_context::add_ram_usage()
   {
   }

} } /// graphene::chain
