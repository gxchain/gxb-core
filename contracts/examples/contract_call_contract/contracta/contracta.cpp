#include <gxblib/action.h>
#include <gxblib/action.hpp>
#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/print.hpp>
#include <gxblib/types.hpp>

using namespace graphene;

class contracta : public contract
{
  public:
    contracta(account_name uname)
        : contract(uname)
    {
    }

    /// @abi action
    void hicontract(uint64_t act_id)
    {
        printf("hi contract:", act_id);
        action a(act_id, N(hi), 10);
        a.send();
    }
};

GXB_ABI(contracta, (hicontract))
