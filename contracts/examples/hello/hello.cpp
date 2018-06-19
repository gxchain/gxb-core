#include <gxblib/gxb.hpp>

using namespace graphene;
using namespace gxblib;

class hello : public graphene::contract {
  public:
      using contract::contract;

      /// @abi action
      void hi(account_name user) {
          print("Hello, ", name{user}, "\n");
          name n = name{string_to_name("dan")};
          std::string s = n.to_string();
          for (int i = 0; i < 3; ++i) {
              print("loop ", i, " name, ", s, "\n");
          }
      }

      /// @abi action
      void bye(account_name user) {
          print("Bye, ", name{user});
      }
};

GXB_ABI(hello, (hi)(bye))
