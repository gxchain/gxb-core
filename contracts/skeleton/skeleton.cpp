#include <gxblib/gxb.hpp>

using namespace graphene;

class skeleton
{
  public:
    skeleton(account_name n)
        : _self(n)
        , accounts(_self, _self)
    {
    }

    //@abi table account i64
    struct account {
        account_name    owner;
        vector<asset>   assets;

        uint64_t primary_key() const { return owner; }

        GXBLIB_SERIALIZE(account, (owner)(assets))
    };

    typedef graphene::multi_index<N(account), account> account_index;

    inline account_name get_self() const { return _self; }

    /// @abi action
    void hi(account_name user)
    {
        print("Hello, ", user, "\n");
    }

    /// @abi action
    void bye(account_name user)
    {
        for (int i = 0; i < 2; ++i) {
            print("Bye, ", user, "\n");
        }
    }

    /// @abi action
    void deposit(account_name from, asset value)
    {
        // from must be msg.sender
        print("deposit trx origin: ", get_trx_origin(), "\n");
        print("deposit trx from: ", from, "\n");
        print("deposit asset_id: ", value.asset_id, ", amount:", value.amount, "\n");
        gxb_assert(get_trx_origin() == from, "no deposit permission");
        // check amount
        // gxb_assert(get_trx_value() == value.amount);

        deposit_asset(from, _self, value.asset_id, value.amount);
        addbalance(from, value);
        print("balance: ", getbalance(from, value.asset_id), "\n");
    }

    /// @abi action
    void withdraw(account_name from, account_name to, asset value)
    {
        // from must be msg.sender
        print("withdraw trx origin: ", get_trx_origin(), "\n");
        print("withdraw trx from: ", from, "\n");
        print("withdraw asset_id: ", value.asset_id, ", amount:", value.amount, "\n");
        // gxb_assert(get_trx_origin() == from, "no withdraw permission");
        // check amount
        // gxb_assert(get_balance(from, value.symbol) >= value.amount);

        subbalance(from, value);
        withdraw_asset(_self, to, value.asset_id, value.amount);
    }

  private:
    void subbalance(account_name owner, asset value)
    {
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            print("account not found\n");
            return;
        }


        int asset_index = 0;
        for(auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
            print("asset.id=", asset_it->asset_id, ", asset.amount=", asset_it->amount, "\n");
            if(asset_it->asset_id == value.asset_id) {
                if(asset_it->amount == value.amount) {
                    accounts.modify(it, owner, [&](auto &a) {
                        a.assets.erase(asset_it);
                    });

                    if(it->assets.size() == 0) {
                        accounts.erase(it);
                    }
                } else if(asset_it->amount > value.amount) {
                    accounts.modify(it, owner, [&](auto &a) {
                        a.assets[asset_index] -= value;
                    });
                } else {
                    gxb_assert(false, "asset_it->amount < value.amount");
                }

                break;
            }

            asset_index++;
        }
    }

    void addbalance(account_name owner, asset value)
    {
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            print("owner not exist, to add owner\n");
            accounts.emplace(owner, [&](auto &a) {
                print("addbalance, owner: ", owner, ", asset_id: ", value.asset_id, ", amount: ", value.amount, "\n");
                a.owner = owner;
                a.assets.emplace_back(value);
            });
        } else {
            print("owner exist, to modify\n");
            bool asset_exist = false;
            int asset_index = 0;
            for(auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
                if(asset_it->asset_id == value.asset_id) {
                    asset_exist = true;
                    accounts.modify(it, 0, [&](auto &a) {
                        a.assets[asset_index] += value;
                    });

                    break;
                }

                asset_index++;
            }

            if(!asset_exist) {
                print("asset not exist, to add asset\n");
                accounts.modify(it, 0, [&](auto &a) {
                    a.assets.emplace_back(value);
                });
            }
        }
    }

    uint64_t getbalance(account_name owner, uint64_t asset_id)
    {
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            print("account not found\n");
            return 0;
        }

        for(auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
            if(asset_it->asset_id == asset_id) {
                return asset_it->amount;
            }
        }

        return 0;
    }

  private:
    account_name _self;
    account_index accounts;
};

GXB_ABI(skeleton, (deposit)(withdraw)(hi)(bye))
