#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
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
        contract_asset amount{asset_amount, asset_id};

        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            accounts.emplace(owner, [&](auto &o) {
                o.owner = owner;
                o.balances.emplace_back(amount);
            });
        } else {
            uint16_t asset_index = std::distance(it->balances.begin(),
                                                 find_if(it->balances.begin(), it->balances.end(), [&](const auto &a) { return a.asset_id == asset_id; }));
            if (asset_index < it->balances.size()) {
                accounts.modify(it, 0, [&](auto &o) { o.balances[asset_index] += amount; });
            } else {
                accounts.modify(it, 0, [&](auto &o) { o.balances.emplace_back(amount); });
            }
        }
    }

    // @abi action
    void withdraw(std::string to_account, contract_asset amount)
    {
        int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
        graphene_assert(account_id >= 0, "invalid account_name to_account");
        graphene_assert(amount.amount > 0, "invalid amount");

        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);
        graphene_assert(it != accounts.end(), "owner has no asset");

        int asset_index = 0;
        for (auto asset_it = it->balances.begin(); asset_it != it->balances.end(); ++asset_it) {
            if ((amount.asset_id) == asset_it->asset_id) {
                graphene_assert(asset_it->amount >= amount.amount, "balance not enough");
                if (asset_it->amount == amount.amount) {
                    accounts.modify(it, 0, [&](auto &o) {
                        o.balances.erase(asset_it);
                    });
                    if (it->balances.size() == 0) {
                        accounts.erase(it);
                    }
                } else {
                    accounts.modify(it, 0, [&](auto &o) {
                        o.balances[asset_index] -= amount;
                    });
                }

                break;
            }
            asset_index++;
        }

        withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
    }

  private:
    //@abi table account i64
    struct account {
        uint64_t owner;
        std::vector<contract_asset> balances;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balances))
    };

    typedef graphene::multi_index<N(account), account> account_index;

    account_index accounts;
};

GRAPHENE_ABI(bank, (deposit)(withdraw))
