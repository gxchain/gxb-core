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

   /// Console methods:
   public:

      void reset_console();
      std::ostringstream &get_console_stream() { return _pending_console_output; }
      const std::ostringstream &get_console_stream() const { return _pending_console_output; }

      template<typename T>
      void console_append(T val) {
          _pending_console_output << val;
      }

      template<typename T, typename ...Ts>
      void console_append(T val, Ts ...rest) {
          console_append(val);
          console_append(rest...);
      };

      inline void console_append_formatted(const string& fmt, const variant_object& vo) {
          console_append(fc::format_string(fmt, vo));
      }

   private:
     std::ostringstream _pending_console_output;
};

} } // namespace graphene::chain

//FC_REFLECT(graphene::chain::apply_context::apply_results, (applied_actions)(deferred_transaction_requests)(deferred_transactions_count))
