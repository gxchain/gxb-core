#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <vector>

using namespace graphene;

class bank : public contract
{
  public:
    bank(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

        uint64_t pk = account::gen_id(owner, asset_id);
        auto it = accounts.find(pk);
        if (it == accounts.end()) {
            print("record not exist, to add");
            accounts.emplace(pk, [&](auto &o) {
                o.id = pk;
                o.amount = asset_amount;
            });
        } else {
            print("account_id:", owner, "asset_id:", asset_id, " record exist, to update");
            accounts.modify(it, 0, [&](auto &o) {
                o.amount += asset_amount;
            });
        }
    }

    // @abi action
    void withdraw(uint64_t to_account, uint64_t contract_asset_id, int64_t amount)
    {
        uint64_t asset_id = contract_asset_id & GRAPHENE_DB_MAX_INSTANCE_ID;
        uint64_t owner = get_trx_sender();

        uint64_t pk = account::gen_id(owner, asset_id);

        auto it = accounts.find(pk);
        graphene_assert(it != accounts.end(), "asset_id not found");
        graphene_assert(it->amount >= amount, "balance not enough");

        if (it->amount == amount) {
            print("withdraw all, to delete the record");
            accounts.erase(it);
        } else {
            print("withdraw part, to udpate");
            accounts.modify(it, 0, [&](auto &o) {
                o.amount -= amount;
            });
        }

        withdraw_asset(_self, to_account, asset_id, amount);
    }

    // @abi action
    void transfer(uint64_t to_account, uint64_t contract_asset_id, int64_t amount)
    {
        withdraw(to_account, contract_asset_id, amount);
    }

  private:
    //@abi table account i64
    struct account {
        //48bit -->account_id (account_id = id >> 16)
        //16bit -->asset_id (asset_id = id & 0xFFFF)
        //id = ((account_id << 16) | (asset_id & 0xFFFF))
        uint64_t id;
        int64_t amount;

        uint64_t primary_key() const { return id; }

        static inline uint64_t gen_id(uint64_t account_id, uint64_t asset_id)
        {
            return ((account_id << 16) | (asset_id & 0xFFFF));
        }

        uint64_t get_asset_id()
        {
            return id & 0xFFFF;
        }

        uint64_t get_account_id()
        {
            return id >> 16;
        }

        GRAPHENE_SERIALIZE(account, (id)(amount))
    };

    typedef graphene::multi_index<N(account), account> account_index;
    account_index accounts;
};

GRAPHENE_ABI(bank, (deposit)(withdraw)(transfer))
