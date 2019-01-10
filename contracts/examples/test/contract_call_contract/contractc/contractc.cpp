#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class contractc : public contract
{
  public:
    contractc(uint64_t uname)
        : contract(uname)
    {
    }

    struct p {
    	uint64_t act_id;
    };
    // @abi action
    // @abi payable
    void hi(bool circlecall = false)
    {
        print("hi i'm contractc\n");
        if(circlecall) {//user(dev) --> contracta(function hicontract)-->contractb(function hi)-->contractc-->contracta
        	p p1{523};
        	action b(526, N(hicontract), std::move(p1), _self, {100000, 1});
        	b.send();
        }
    }
};

GRAPHENE_ABI(contractc, (hi))
