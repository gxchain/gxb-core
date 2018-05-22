#pragma once
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

namespace graphene { namespace chain {

class transaction_context;

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
      apply_context()
      {
      }


   /// Execution methods:
   public:

      void exec();
};

} } // namespace graphene::chain

//FC_REFLECT(graphene::chain::apply_context::apply_results, (applied_actions)(deferred_transaction_requests)(deferred_transactions_count))
