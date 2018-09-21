#include <graphenelib/contract.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>

using namespace graphene;

class testapi : public contract
{
  public:
    testapi(uint64_t id)
        : contract(id)
    {
    }

    /// @abi action
    void test()
    {
        char dst[1024] = {0};
        //获取当前合约调用所属transaction序列化后的数据到的长度
        int dst_len = transaction_size();
        print("transaction_size() = ", dst_len, "\n");

        //获取当前合约调用所属transaction序列化后的数据到dst中
        read_transaction(dst, dst_len);

        //计算dst的sha256并输出前20byte。trx_id的值就是transaction的序列化数据的sha256的前20个字节
        checksum256 hs;
        sha256(dst, dst_len, &hs);
        printhex(hs.hash, 20);
        print("\n");

        printhex(dst, dst_len);
        print("\n");

        //获取当前合约调用所属transaction的过期时间，单位秒
        print("expiration() = ", expiration(), "\n");

        //获取当前合约调用所属block的ref block number
        print("tapos_block_num() = ", tapos_block_num(), "\n");

        //获取当前合约调用所属block的ref block prefix
        print("tapos_block_prefix() = ", tapos_block_prefix(), "\n");
    }
};

GRAPHENE_ABI(testapi, (test))
