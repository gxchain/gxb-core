#include <gxblib/gxb.hpp>

using namespace gxblib;
using namespace graphene;

class test : public graphene::contract {
  public:
      using contract::contract;

      test(account_name self)
      :graphene::contract(self),
      table_idx(_self, _self)
      {}

      /// @abi action
      void hi(account_name user) {
          print("Hello, ", user);
      }

      /// @abi action
      void bye(account_name user) {
          for (int i =0; i < 2; ++i) {
              print("Bye, ", user);
          }
      }

   private:
      //@abi table offer i64
      struct test_table {
         uint64_t          id;
         asset             bet;
         uint64_t          gameid = 0;

         uint64_t primary_key()const { return id; }
         uint64_t by_bet() const { return (uint64_t) bet.amount; }
         GXBLIB_SERIALIZE(test_table, (id)(bet)(gameid))
      };

      typedef graphene::multi_index< N(test_table), test_table,
         indexed_by< N(bet), const_mem_fun<test_table, uint64_t, &test_table::by_bet > >
      > test_table_index;

      test_table_index       table_idx;
};

GXB_ABI(test, (deposit)(withdraw)(hi)(bye))
