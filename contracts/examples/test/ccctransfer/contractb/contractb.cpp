#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class contractb : public contract
{
  public:
    contractb(uint64_t uname)
        : contract(uname)
    {
    }

    // @abi action
    // @abi payable
    void hi()
    {
        print("hi i'm contractb\n");
    }
};

GRAPHENE_ABI(contractb, (hi))
