#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <vector>

using namespace graphene;

class linear_vesting_asset : public contract
{
  public:
    linear_vesting_asset(uint64_t n)
        : contract(n)
        , accounts(_self, _self)
        , vesting_rules(_self, _self)
    {
    }

    /// @abi action
    /// @abi payable
    void vestingcreate(uint64_t to, int64_t lock_duration, int64_t release_duration)
    {
        contract_asset ast{get_action_asset_amount(), get_action_asset_id()};
        uint64_t pk = vestingrule::get_primary_key_by_account_and_asset(to, ast.asset_id);
        auto lr = vesting_rules.find(pk);
        graphene_assert(lr == vesting_rules.end(), "have been locked, can only lock one time");

        vesting_rules.emplace(pk, [&](auto &a) {
            a.account_id = to;
            a.lock_time_point = get_head_block_time();
            a.lock_duration = lock_duration;
            a.release_time_point = a.lock_time_point + a.lock_duration;
            a.release_duration = release_duration;
            a.asset_id = ast.asset_id;
            a.asset_amount = ast.amount;
            a.released_amount = 0;
            a.id = pk;
        });

        addbalance(to, ast);
    }

    /// @abi action
    void vestingclaim(uint64_t who, uint64_t asset_id)
    {
        uint64_t pk = vestingrule::get_primary_key_by_account_and_asset(who, asset_id);
        auto lr = vesting_rules.find(pk);
        graphene_assert(lr != vesting_rules.end(), "have no locked asset, no vesting_rule");

        uint64_t now = get_head_block_time();
        if (now <= lr->release_time_point) {
            print("within lock duration, can not release", "\n");
            return;
        }

        auto who_it = accounts.find(who);
        graphene_assert(who_it != accounts.end(), "have no locked asset, no asset");

        int percentage = (now - lr->release_time_point) * 100 / lr->release_duration;
        if (percentage > 100)
            percentage = 100;
        print("percentage=", percentage, "\n");

        int64_t should_release_amount = (int64_t)(1.0f * lr->asset_amount * percentage / 100);
        should_release_amount = should_release_amount - lr->released_amount;
        if (should_release_amount <= 0) {
            print("release amount reach max");
            return;
        }

        contract_asset a{should_release_amount, asset_id};
        subbalance(who, a);
        withdraw_asset(_self, who, a.asset_id, a.amount);

        vesting_rules.modify(lr, 0, [&](auto &l) {
            l.released_amount += should_release_amount;
        });

        if (lr->released_amount == lr->asset_amount) {
            print("release finished");
            vesting_rules.erase(lr);
        }
    }

  private:
    void subbalance(uint64_t owner, contract_asset value)
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
                    graphene_assert(false, "asset_it->amount < value.amount");
                }

                break;
            }

            asset_index++;
        }
    }

    void addbalance(uint64_t owner, contract_asset value)
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

    uint64_t getbalance(uint64_t owner, uint64_t asset_id)
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

  private:
    //@abi table account i64
    struct account {
        uint64_t owner;
        std::vector<contract_asset> assets;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(assets))
    };
    typedef graphene::multi_index<N(account), account> account_index;

    //@abi table vestingrule i64
    struct vestingrule {
        uint64_t id;                //id=get_primary_key_by_account_and_asset(account_id, asset_id)
        uint64_t account_id;        //某个合约参与者的账号
        int64_t lock_time_point;    //锁定开始时间
        int64_t lock_duration;      //锁定多久开始释放
        int64_t release_time_point; //释放开始时间
        int64_t release_duration;   //释放多久全部释放完毕
        uint64_t asset_id;          //资产id
        int64_t asset_amount;       //总共锁定多少资产
        int64_t released_amount;    //总共已经释放了多少资产

        uint64_t primary_key() const { return id; }

        static inline uint64_t get_primary_key_by_account_and_asset(uint64_t account_id, uint64_t asset_id)
        {
            return (uint64_t)(account_id << 32 | (asset_id & 0xFFFFFFFF));
        }

        GRAPHENE_SERIALIZE(vestingrule, (id)(account_id)(lock_time_point)(lock_duration)(release_time_point)(release_duration)(asset_id)(asset_amount)(released_amount))
    };

    typedef graphene::multi_index<N(vestingrule), vestingrule> vesting_index;

  private:
    account_index       accounts;
    vesting_index       vesting_rules;
};

GRAPHENE_ABI(linear_vesting_asset, (vestingcreate)(vestingclaim))
