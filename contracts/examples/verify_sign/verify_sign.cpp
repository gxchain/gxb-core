#include <graphenelib/contract.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>

using namespace graphene;

class verify_sign : public contract
{
  public:
    verify_sign(uint64_t id)
        : contract(id)
    {
    }

    void verify(std::string &raw_string, std::string &sig, std::string &pub_key)
    {
        int ret = verify_signature(raw_string.c_str(), raw_string.length(), sig.c_str(), sig.length(), pub_key.c_str(), pub_key.length());
        print("ret code, ", ret, "\n");
    }
};

GRAPHENE_ABI(verify_sign, (verify))
