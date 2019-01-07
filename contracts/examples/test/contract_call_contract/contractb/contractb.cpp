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

    /// @abi action
    void hi(uint64_t a, uint64_t b)
    {
        uint64_t c = a+b;
        print("hi", c, "\n");
    }
};

GRAPHENE_ABI(contractb, (hi))
