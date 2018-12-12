#include <graphenelib/contract.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>
#include <graphenelib/asset.h>

using namespace graphene;

class testapi1 : public contract
{
  public:
    testapi1(uint64_t id)
        : contract(id)
    {
    }

    /// @abi action
    void test1()
    {
        int dst_len = transaction_size();
        print("transaction_size() = ", dst_len, "\n");
    }

    /// @abi action
    void test2()
    {
        int trx_len = read_transaction(nullptr, 1024);
        print("trx_len = ", trx_len, "\n");
    }

    /// @abi action
    void test3()
    {
        int trx_len1 = read_transaction(nullptr, 0);
        print("trx_len1 = ", trx_len1, "\n");
    }

    /// @abi action
    void test4()
    {
        int trx_len2 = read_transaction((char*)100, 1024);
        print("trx_len2 = ", trx_len2, "\n");
    }

    /// @abi action
    void test5()
    {
        int trx_len2 = read_transaction((char*)100, 0);
        print("trx_len2 = ", trx_len2, "\n");
    }

    /// @abi action
    void test11()
    {
        int dst_len = transaction_size();
        char dst[1024] = { 0 };
        read_transaction(dst, dst_len);

        //计算dst的sha256并输出前20byte。trx_id的值就是transaction的序列化数据的sha256的前20个字节
        checksum256 hs;
        sha256(dst, dst_len, &hs);
        printhex(hs.hash, 20);
        print("\n");

        printhex(dst, dst_len);
        print("\n");
    }

    /// @abi action
    //@abi payable
    void test12()//depsite_and_withdraw test
    {
        uint64_t caller = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        withdraw_asset(_self, 17, asset_id, asset_amount);
    }
};

GRAPHENE_ABI(testapi1, (test1)(test2)(test3)(test4)(test5)(test11)(test12))
