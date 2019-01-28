#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class contracta : public contract
{
  public:
    contracta(uint64_t uname)
        : contract(uname)
    {
    }

    // @abi action
    // @abi payable
    void hi(uint64_t contratb_id)
    {
        print("hi i'm contracta's function hi\n");

        action b(contratb_id, N(hi), "", _self, {100000, 1});
        b.send();
    }
};

GRAPHENE_ABI(contracta, (hi))
