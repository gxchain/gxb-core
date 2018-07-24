#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/print.hpp>
#include <gxblib/types.h>
#include <gxblib/crypto.h>

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
        int pub_key_len = pub_key.length();
        char *pub_key_data = (char *)malloc((pub_key_len + 1) * sizeof(char));
        pub_key.copy(pub_key_data, pub_key_len, 0);

        int sig_data_len = sig.length();
        char *sig_data = (char *)malloc((sig_data_len + 1) * sizeof(char));
        sig.copy(sig_data, sig_data_len, 0);

        int len = raw_string.length();
        char *data = (char *) malloc((len + 1) * sizeof(char));
        raw_string.copy(data, len, 0);

        print("raw_string, ", raw_string, "\n");
        print("pub_key, ", pub_key, "\n");

        int ret = verify_signature(data, raw_string.length(), sig_data, sig.length(), w, pub_key_data, pub_key.length());
        print("ret code, ", ret, "\n");
    }
};
};

GXB_ABI(verify_sign, (verify))
