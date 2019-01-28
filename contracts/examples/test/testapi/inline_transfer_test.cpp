#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/asset.h>
#include <vector>

using namespace graphene;

class bank : public contract
{
public:
    bank(uint64_t account_id)
        : contract(account_id)
    {
    }

    // @abi action
    // @abi payable
    void deposit()
    {
        uint64_t owner = get_trx_sender();
        print("hello, ", owner, "\n");
    }

    // @abi action
    void withdraw(std::string to_account, contract_asset amount)
    {
        int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
        graphene_assert(account_id >= 0, "invalid account_name to_account");
        graphene_assert(amount.amount > 0, "invalid amount");

        std::string memo = "withdraw test";;
        inline_transfer(_self, account_id, amount.asset_id, amount.amount, memo.c_str(), memo.size());
    }
};

GRAPHENE_ABI(bank, (deposit)(withdraw))
