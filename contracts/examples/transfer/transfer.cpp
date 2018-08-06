#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/system.h>
#include <vector>

using namespace graphene;

class transfer : public contract
{
  public:
    transfer(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
    {}

    // @abi action
    // @abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        contract_asset amount{asset_amount, asset_id};

        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            print("owner not exist, to add owner");
            accounts.emplace(owner, [&](auto &o) {
                o.owner = owner;
                o.balances.emplace_back(amount);
            });
        } else {
            print("owner exist");
            uint16_t asset_index = std::distance(it->balances.begin(),
                    find_if(it->balances.begin(), it->balances.end(), [&](contract_asset a) { return a.asset_id == asset_id; })
                    );
            if (asset_index < it->balances.size()) {
                print("asset exist, to update");
                accounts.modify(it, 0, [&](auto &o) { o.balances[asset_index] += amount; });
            } else {
                print("asset not exist, to add");
                accounts.modify(it, 0, [&](auto &o) { o.balances.emplace_back(amount); });
            }
        }
    }

    // @abi action
    void withdraw(uint64_t to_account, uint64_t contract_asset_id, int64_t amount)
    {
        uint64_t asset_id = contract_asset_id & GRAPHENE_DB_MAX_INSTANCE_ID;
        contract_asset asset{amount, asset_id};
        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            print("owner has no asset");
            return;
        }

        int asset_index = 0;
        for (auto asset_it = it->balances.begin(); asset_it != it->balances.end(); ++asset_it) {
            if ((asset.asset_id) == asset_it->asset_id) {
                graphene_assert(asset_it->amount >= asset.amount, "balance not enough");
                print("asset_it->amount=", asset_it->amount);
                print("amount.amount=", asset.amount);
                if (asset_it->amount == asset.amount) {
                    accounts.modify(it, 0, [&](auto &o) {
                        o.balances.erase(asset_it);
                    });
                    if (it->balances.size() == 0) {
                        accounts.erase(it);
                    }
                } else {
                    accounts.modify(it, 0, [&](auto &o) {
                        o.balances[asset_index] -= asset;
                    });
                }

                break;
            }
            asset_index++;
        }

        withdraw_asset(_self, to_account, asset.asset_id, asset.amount);
    }

  private:
    //@abi table account i64
    struct account {
        uint64_t owner;
        std::vector<contract_asset> balances;

        uint64_t primary_key() const { return owner; }

        GXBLIB_SERIALIZE(account, (owner)(balances))
    };

    typedef graphene::multi_index<N(account), account> account_index;

    account_index accounts;
};

GXB_ABI(transfer, (deposit)(withdraw))
