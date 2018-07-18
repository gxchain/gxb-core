#include <gxblib/gxb.hpp>

using namespace graphene;

class skeleton
{
  public:
    skeleton(account_name n)
        : _self(n)
        , accounts(_self, _self)
        , lockrules(_self, _self)
    {
    }

    //@abi table account i64
    struct account {
        account_name owner;
        vector<asset> assets;

        uint64_t primary_key() const { return owner; }

        GXBLIB_SERIALIZE(account, (owner)(assets))
    };

    typedef graphene::multi_index<N(account), account> account_index;

    inline account_name get_self() const { return _self; }

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

        transfer_asset(from, _self, value.asset_id, value.amount);
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
        transfer_asset(_self, to, value.asset_id, value.amount);
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
        for (auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
            print("asset.id=", asset_it->asset_id, ", asset.amount=", asset_it->amount, "\n");
            if (asset_it->asset_id == value.asset_id) {
                if (asset_it->amount == value.amount) {
                    accounts.modify(it, owner, [&](auto &a) {
                        a.assets.erase(asset_it);
                    });

                    if (it->assets.size() == 0) {
                        accounts.erase(it);
                    }
                } else if (asset_it->amount > value.amount) {
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
            for (auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
                if (asset_it->asset_id == value.asset_id) {
                    asset_exist = true;
                    accounts.modify(it, 0, [&](auto &a) {
                        a.assets[asset_index] += value;
                    });

                    break;
                }

                asset_index++;
            }

            if (!asset_exist) {
                print("asset not exist, to add asset\n");
                accounts.modify(it, 0, [&](auto &a) {
                    a.assets.reserve(a.assets.size() + 1);
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

        for (auto asset_it = it->assets.begin(); asset_it != it->assets.end(); ++asset_it) {
            if (asset_it->asset_id == asset_id) {
                return asset_it->amount;
            }
        }

        return 0;
    }

    //////////////////////////////////////////////////////

    //@abi table lockrule i64
    struct lockrule {
        uint64_t account_name;            //某个合约参与者的账号
        uint64_t lock_time_point;         //锁定开始时间
        uint64_t lock_time;               //锁定多久开始释放
        uint64_t lock_release_time_point; //释放开始时间
        uint64_t lock_release_time;       //释放多久全部释放完毕
        uint64_t asset_it;                //资产id

        uint64_t primary_key() const { return account_name; }

        GXBLIB_SERIALIZE(lockrule, (account_name)(lock_time_point)(lock_time)(lock_release_time_point)(lock_release_time)(asset_it))
    };

    typedef graphene::multi_index<N(lockrule), lockrule> lock_rule_index;

  public:
    /// @abi action
    void lockasset(uint64_t from, account_name to, asset value)
    {
        auto lr = lockrules.find(from);
        gxb_assert(lr == lockrules.end(), "have been locked, can only lock one time");

        lockrules.emplace(from, [&](auto &a) {
            a.account_name = to;
            a.lock_time_point = get_head_block_time();
            a.lock_time = 30;
            a.lock_release_time_point = a.lock_time_point + a.lock_time;
            a.lock_release_time = 3 * 100;
            a.asset_it = value.asset_id;
        });

        transfer_asset(from, _self, value.asset_id, value.amount);
        addbalance(to, value);
    }

    /// @abi action
    void tryrelease(uint64_t who)
    {
        if (canrelease(who)) {
            release(who);
        }
    }

  private:
    bool canrelease(uint64_t who)
    {
        auto lr = lockrules.find(who);
        gxb_assert(lr != lockrules.end(), "have no locked asset");

        uint64_t cur_time = get_head_block_time();
        if (cur_time > lr->lock_release_time_point) {
            return true;
        }

        return false;
    }

    int64_t getreleaseamount(uint64_t who)
    {
        auto lr = lockrules.find(who);
        auto who_it = accounts.find(who);
        gxb_assert(lr != lockrules.end(), "have no locked asset");
        gxb_assert(who_it != accounts.end(), "have no locked asset");

        uint64_t cur_time = get_head_block_time();
        int percentage = (cur_time - lr->lock_release_time_point) * 100 / lr->lock_release_time;
        print("percentage = ${p}", ("p", percentage));

        return (int64_t)(1.0f * who_it->assets[0].amount * percentage / 100);
    }

    void release(uint64_t who)
    {
        int64_t amt = getreleaseamount(who);
        asset a{amt, 0};
        withdraw(_self, who, a);
    }

  private:
    account_name _self;
    account_index accounts;
    lock_rule_index lockrules;
};

GXB_ABI(skeleton, (deposit)(withdraw)(lockasset)(tryrelease))
