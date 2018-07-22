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
        print("hi contract:", act_id);
        std::string s = "abc";
        action a(act_id, N(hi), bytes(s.begin(), s.end()));
        a.send();
    }
};

GXB_ABI(contracta, (hicontract))
