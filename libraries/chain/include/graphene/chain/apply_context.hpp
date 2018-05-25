#pragma once
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

#include <graphene/chain/action.hpp>

namespace graphene { namespace chain {

class apply_context {
   private:

      template<typename>
      struct array_size;

      template<typename T, size_t N>
      struct array_size< std::array<T,N> > {
          static constexpr size_t size = N;
      };


   /// Constructor
   public:
      apply_context(const action& a)
          : act(a)
          , receiver(a.account)
      {
      }

   public:
      const action&       act; ///< message being applied
      uint64_t            receiver;

   /// Execution methods:
   public:

      void exec() {}
};

} } // namespace graphene::chain

//FC_REFLECT(graphene::chain::apply_context::apply_results, (applied_actions)(deferred_transaction_requests)(deferred_transactions_count))
