#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <vector>

using namespace graphene;

static const uint64_t contract_asset_id = 1;//GXS

class linear_vesting_asset : public contract
{
  public:
    linear_vesting_asset(uint64_t n)
        : contract(n)
        , vestingrules(_self, _self)
    {
    }

    /// @abi action
    /// @abi payable
    void vestingcreate(std::string to, int64_t lock_duration, int64_t release_duration)
    {
        graphene_assert(contract_asset_id == get_action_asset_id(), "not supported asset");
        contract_asset ast{get_action_asset_amount(), contract_asset_id};

        int64_t to_account_id = get_account_id(to.c_str(), to.size());
        graphene_assert(to_account_id >= 0, "invalid account_name to");

        auto lr = vestingrules.find(to_account_id);
        graphene_assert(lr == vestingrules.end(), "have been locked, can only lock one time");

        vestingrules.emplace(0, [&](auto &o) {
            o.account_id = to_account_id;

            o.vesting_amount = ast.amount;
            o.vested_amount = 0;

            o.lock_time_point = get_head_block_time();
            o.lock_duration = lock_duration;
            o.release_time_point = o.lock_time_point + o.lock_duration;
            o.release_duration = release_duration;
        });
    }

    /// @abi action
    void vestingclaim(std::string who)
    {
        int64_t who_account_id = get_account_id(who.c_str(), who.size());
        graphene_assert(who_account_id >= 0, "invalid account_name to");

        auto lr = vestingrules.find(who_account_id);
        graphene_assert(lr != vestingrules.end(), "current account have no locked asset");

        uint64_t now = get_head_block_time();
        graphene_assert(now > lr->release_time_point, "within lock duration, can not release");

        int percentage = (now - lr->release_time_point) * 100 / lr->release_duration;
        if (percentage > 100)
            percentage = 100;

        int64_t vested_amount = (int64_t)(1.0f * lr->vesting_amount * percentage / 100);
        vested_amount = vested_amount - lr->vested_amount;
        graphene_assert(vested_amount > 0, "vested amount must > 0");

        withdraw_asset(_self, who_account_id, contract_asset_id, vested_amount);

        vestingrules.modify(lr, 0, [&](auto &o) {
            o.vested_amount += vested_amount;
        });

        if (lr->vesting_amount == lr->vested_amount) {
            vestingrules.erase(lr);
        }
    }

  private:
    //@abi table vestingrule i64
    struct vestingrule {
        uint64_t account_id;

        int64_t vesting_amount;     //初始锁定资产
        int64_t vested_amount;      //已释放资产

        int64_t lock_time_point;    //锁定开始时间
        int64_t lock_duration;      //锁定多久开始释放
        int64_t release_time_point; //释放开始时间
        int64_t release_duration;   //释放多久全部释放完毕

        uint64_t primary_key() const { return account_id; }

        GRAPHENE_SERIALIZE(vestingrule,
                           (account_id)(vesting_amount)(vested_amount)(lock_time_point)(lock_duration)(release_time_point)(release_duration))
    };

    typedef graphene::multi_index<N(vestingrule), vestingrule> vestingrule_index;

  private:
    vestingrule_index vestingrules;
};

GRAPHENE_ABI(linear_vesting_asset, (vestingcreate)(vestingclaim))
