#include <gxblib/action.h>
#include <gxblib/action.hpp>
#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/print.hpp>

using namespace graphene;

class contractb : public contract
{
  public:
    contractb(uint64_t uname)
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
