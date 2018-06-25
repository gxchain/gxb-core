#include <gxblib/gxb.hpp>

using namespace graphene;
using namespace gxblib;

class hello : public graphene::contract {
  public:
      using contract::contract;

      /// @abi action
      void hi(account_name user) {
          print("Hello, ", user, "\n");
          for (int i = 0; i < 3; ++i) {
              print("loop ", i, " name, ", user, "\n");
          }
      }

      /// @abi action
      void bye(account_name user) {
          print("Bye, ", user, "\n");
      }
};

GXB_ABI(hello, (hi)(bye))
