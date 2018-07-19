#include <gxblib/action.h>
#include <gxblib/action.hpp>
#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/print.hpp>
#include <gxblib/types.hpp>

using namespace graphene;

class contractb : public contract
{
  public:
    contractb(account_name uname)
        : contract(uname)
    {
    }

    /// @abi action
    void hi()
    {
        printf("hi");
    }
};

GXB_ABI(contractb, (hi))
