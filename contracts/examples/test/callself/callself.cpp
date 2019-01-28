#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class callself : public contract
{
  public:
	callself(uint64_t uname)
        : contract(uname)
    {
    }

    // @abi action
    // @abi payable
    void f1(uint64_t p1)
    {
        print("hi i'm function f1, hello p1:", p1, "\n");
        struct p {
            uint64_t p1;
        };
       p pf1{100};
       action a(_self, N(f2), std::move(p1), _self, {100000, 1});
       a.send();
    }

    // @abi action
    // @abi payable
    void f2(uint64_t p1)
    {
        print("hi i'm function f2, hello p1:", p1, "\n");
    }
};

GRAPHENE_ABI(callself, (f1)(f2))
