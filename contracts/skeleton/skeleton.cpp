#include <gxblib/gxb.hpp>

using namespace graphene;

class hello : public eosio::contract {
  public:
      using contract::contract;

      /// @abi action 
      void hi( account_name user ) {
         print( "Hello, ", name{user} );
      }
};

GXB_ABI( hello, (hi) )
