#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/print.hpp>
#include <gxblib/types.h>

using namespace graphene;

class skeleton : public contract
{
  public:
    skeleton(account_name n)
        : contract(n)
    {
    }

    /// @abi action
    void hi(account_name user)
    {
        for (int i = 0; i < 2; ++i) {
            print("hi, ", user, "\n");
        }
    }
};

GXB_ABI(skeleton, (hi))
