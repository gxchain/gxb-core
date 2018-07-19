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
    void hicontract(std::string contractname)
    {
        printf("hi contract:", contractname.c_str());
        action a(gxblib::string_to_name(contractname.c_str()), asset(), N(hia), 10);
        a.send();
    }
};

GXB_ABI(contracta, (hicontract))
