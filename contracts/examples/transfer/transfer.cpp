#include <gxblib/asset.h>
#include <gxblib/asset.hpp>
#include <gxblib/contract.hpp>
#include <gxblib/dispatcher.hpp>
#include <gxblib/global.h>
#include <gxblib/multi_index.hpp>
#include <gxblib/print.hpp>
#include <gxblib/system.h>
#include <vector>

using namespace graphene;

class transfer : public contract
{
  public:
    transfer(uint64_t account_id)
        : contract(account_id)
        , ownerassets_index(_self, _self)
    {
    }

    // @abi action
    void deposit()
    {
        uint64_t owner = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        gxb_assert(asset_amount > 0, "deposit amount must > 0");
        gxb_assert(asset_amount >= 0, "deposit asset_id must >= 0");

        asset amount{asset_amount, asset_id};

        auto it = ownerassets_index.find(owner);
        if (it == ownerassets_index.end()) {
            print("owner not exist, to add owner");
            ownerassets_index.emplace(owner, [&](auto &o) {
                o.owner = owner;
                o.assets.emplace_back(amount);
            });
        } else {
            print("owner exist");
            bool asset_exist = false;
            int asset_index = 0;
            for (auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
                if (asset_it->asset_id == asset_id) {
                    print("asset exist, to update");
                    asset_exist = true;
                    ownerassets_index.modify(it, 0, [&](auto &o) {
                        o.assets[asset_index] += amount;
                    });
                    break;
                }

                asset_index++;
            }

            if (!asset_exist) {
                print("asset not exist, to add");
                ownerassets_index.modify(it, 0, [&](auto &o) {
                    o.assets.emplace_back(amount);
                });
            }
        }
    }

    // @abi action
    void withdraw(asset amount)
    {
        print("amount.id=", amount.asset_id);
        print("amount.amount=", amount.amount);
        int64_t asset_amount = get_action_asset_amount();
        if (asset_amount > 0) {//skip trap for lose asset
            deposit();
        }

        uint64_t owner = get_trx_sender();
        auto it = ownerassets_index.find(owner);
        if (it == ownerassets_index.end()) {
            print("owner has no asset");
            return;
        }

        int asset_index = 0;
        for (auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
            if (asset_it->asset_id == amount.asset_id) {
                gxb_assert(asset_it->amount >= amount.amount, "balance not enough");
                if (asset_it->amount == amount.amount) {
                    ownerassets_index.modify(it, 0, [&](auto &o) {
                        o.assets.erase(asset_it);
                    });
                    if (it->assets.size() == 0) {
                        ownerassets_index.erase(it);
                    }
                } else {
                    ownerassets_index.modify(it, 0, [&](auto &o) {
                        o.assets[asset_index] -= amount;
                    });
                }
            }
            asset_index++;
        }

        withdraw_asset(_self, owner, amount.asset_id, amount.amount);
    }

  private:
    //@abi table ownerassets i64
    struct ownerassets {
        uint64_t owner;
        std::vector<asset> assets;

        uint64_t primary_key() const { return owner; }

        GXBLIB_SERIALIZE(ownerassets, (owner)(assets))
    };

    typedef graphene::multi_index<N(ownerassets), ownerassets> ownerassets_index_type;

    ownerassets_index_type ownerassets_index;
};

GXB_ABI(transfer, (deposit)(withdraw))
