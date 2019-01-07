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

    /// @abi action
    void hicontract(uint64_t act_id)
    {
        print("hi contract:", act_id, "\n");
        struct p {
            uint64_t a;
            uint64_t b;
        };

        p p1{1, 2};
        action a(act_id, N(hi), std::move(p1));
        a.send();
    }
};

GRAPHENE_ABI(contracta, (hicontract))
