#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/dispatcher.hpp>
#include <map>
#include <vector>
#include <math.h>

using namespace graphene;

class swap : public contract{
    public:
        swap(uint64_t account_id)
            : contract(account_id)
            , pools(_self, _self)
            , banks(_self, _self)
            {
            }
    

    //@abi action
    //@abi payable
    void deposit(){
        uint64_t owner = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        
        auto owner_iterator = banks.find(owner);
        if(owner_iterator == banks.end()) {
            banks.emplace( owner, [&](auto &o) {
                o.owner = owner;
                o.assert_bank.insert(std::pair<uint64_t, int64_t>(asset_id,asset_amount));
            });
        } else {
            auto assert_iterator = (*owner_iterator).assert_bank.find(asset_id);
            if(assert_iterator == (*owner_iterator).assert_bank.end()){
                banks.modify(owner_iterator, owner, [&](auto o){
                    o.assert_bank.insert(std::pair<uint64_t, int64_t>(asset_id,asset_amount));
                });
            } else {
                banks.modify(owner_iterator, owner, [&](auto o){
                    o.assert_bank[asset_id] += asset_amount;
                });    
            }
        }
    }

    //@abi action 
    void addliquidity(std::string coin1, std::string coin2
        , int64_t amount1Desire, int64_t amount2Desire
        , int64_t amount1Min, int64_t amount2Min
        , uint64_t deadline
        , std::string to
    ) {
        // 检查超时间.
        _check_deadline(deadline);
        auto sender = get_trx_sender();
        // 检查asset_id.
        auto id1 = get_asset_id(coin1.c_str(), coin1.size());
        auto id2 = get_asset_id(coin2.c_str(), coin2.size());
        graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, "illegal asset id!");

        // 计算pool_index.
        auto pool_index = id1 < id2 ? ::graphenelib::string_to_name((std::to_string(id1) + "x" + std::to_string(id2)).c_str())
            : ::graphenelib::string_to_name((std::to_string(id2) + "x" + std::to_string(id1)).c_str());
        auto pool_itr = pools.find(pool_index);
        // 如果交易对不存在对话则创建.
        if (pool_itr == pools.end()) {
            pool_itr = pools.emplace(sender, [&](pool& p) {
                p.index = pool_index;
                p.balance1.asset_id = id1 < id2 ? id1 : id2;
                p.balance1.amount = 0;
                p.balance2.asset_id = id1 < id2 ? id2 : id1;
                p.balance2.amount = 0;
                p.total_lq = 0;
                p.locked = 0;
            });
        }
        
        // 根据池内的资金数量计算可以质押的金额.
        int64_t amount1, amount2;
        if (pool_itr->balance1.amount == 0 && pool_itr->balance2.amount == 0) {
            amount1 = amount1Desire;
            amount2 = amount2Desire;
        }
        else {
            auto amount2Optimal = _quote(amount1Desire, pool_itr->balance1.amount, pool_itr->balance2.amount);
            if (amount2Optimal <= amount2Desire) {
                graphene_assert(amount2Optimal >= amount2Min, "insufficient coin2 amount");
                amount1 = amount1Desire;
                amount2 = amount2Optimal;
            } else {
                auto amount1Optimal = _quote(amount2Desire, pool_itr->balance2.amount, pool_itr->balance1.amount);
                graphene_assert(amount1Optimal <= amount1Desire && amount1Optimal >= amount1Min, "insufficient coin1 amount");
                amount1 = amount1Optimal;
                amount2 = amount2Desire;
            }
        }
        graphene_assert(amount1 > 0 && amount2 > 0, "insufficient amount");

        // 计算增加的流动性.
        int64_t lq;
        if (pool_itr->total_lq == 0) {
            lq = sqrt(amount1 * amount2) - MINLIQUIDITY;
            // 将最小流动性分配给黑洞账号.
            auto zero_bank_itr = banks.find(0);
            if (zero_bank_itr == banks.end()) {
                banks.emplace(sender, [&](bank& b) {
                    b.owner = 0;
                    b.liquid_bank[pool_index] = MINLIQUIDITY;
                });
            }
            else {
                banks.modify(*zero_bank_itr, sender, [&](bank& b) {
                    b.liquid_bank[pool_index] = MINLIQUIDITY;
                });
            }
        }
        else {
            lq = std::min(amount1 * pool_itr->total_lq / pool_itr->balance1.amount
                , amount2 * pool_itr->total_lq / pool_itr->balance2.amount);
        }
        graphene_assert(lq > 0, "insufficient liquidity");

        // 修改接受者流动性代币的余额.
        auto to_account_id = get_account_id(to.c_str(), to.size());
        if (to_account_id != sender) {
            graphene_assert(to_account_id != -1, "illegal account!");
            auto to_bank_itr = banks.find(to_account_id);
            if (to_bank_itr == banks.end()) {
                banks.emplace(sender, [&](bank& b) {
                    b.owner = to_account_id;
                    b.liquid_bank[pool_index] = lq;
                });
            }
            else {
                banks.modify(*to_bank_itr, sender, [&](bank& b) {
                    b.liquid_bank[pool_index] += lq;
                });
            }
        }

        auto bank_itr = banks.find(sender);
        graphene_assert(bank_itr != banks.end(), "missing user");
        auto asset1_itr = bank_itr->assert_bank.find(id1);
        auto asset2_itr = bank_itr->assert_bank.find(id2);
        graphene_assert(asset1_itr != bank_itr->assert_bank.end()
            && asset2_itr != bank_itr->assert_bank.end()
            && asset1_itr->second >= amount1
            && asset2_itr->second >= amount2
            , "insufficient amount");
        // 修改bank中的代币余额.
        banks.modify(*bank_itr, sender, [&](bank& b) {
            auto _asset1_itr = b.assert_bank.find(id1);
            auto _asset2_itr = b.assert_bank.find(id2);
            _asset1_itr->second -= amount1;
            if (_asset1_itr->second == 0) {
                b.assert_bank.erase(_asset1_itr);
            }
            _asset2_itr->second -= amount2;
            if (_asset2_itr->second == 0) {
                b.assert_bank.erase(_asset2_itr);
            }

            if (to_account_id == sender) {
                b.liquid_bank[pool_index] += lq;
            }
        });
        // 修改pool中的代币余额, 同时增加流动性代币总量.
        pools.modify(*pool_itr, sender, [&](pool& p) {
            p.balance1.amount += amount1;
            p.balance2.amount += amount2;
            p.total_lq += lq;
        });
    }

    //@abi action
    void removeliquidity()
    {

    }
    
    //@abi action
    //@abi payable
    void swapexacttokensfortokens()
    {

    }

    //@abi action
    void swaptokensforexacttokens()
    {

    }


    //@abi action
    void wthdraw()
    {

    }

    //@abi action
    void transferliquidity()
    {

    }

    //@abi action
    void managepool(std::string coin1, std::string coin2
        , bool locked
    ) {
        //检查调用者是否为管理员
        auto sender = get_trx_sender();
        graphene_assert(sender == ADMINACCOUNT, "You do not have access to make pool configuration modifications.");

        //检查交易对是否存在
        auto pool_itr = pools.find(_make_pool_index(coin1, coin2));
        graphene_assert(pool_itr != pools.end(), "The trading pair does not exist.");

        //修改状态
        pools.modify(pool_itr, sender, [&](pool& p) {
            p.locked = locked;
        });
    }

    private:
    inline static uint64_t _make_pool_index(const std::string& coin1, const std::string& coin2) {
        auto id1 = get_asset_id(coin1.c_str(), coin1.size());
        auto id2 = get_asset_id(coin2.c_str(), coin2.size());
        graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, "illegal asset id!");
        return id1 < id2 ? ::graphenelib::string_to_name((std::to_string(id1) + "x" + std::to_string(id2)).c_str())
            : ::graphenelib::string_to_name((std::to_string(id2) + "x" + std::to_string(id1)).c_str());
    }

    inline static void _check_deadline(const uint64_t& deadline) {
        graphene_assert(deadline >= get_head_block_time(), "expired!");
    }

    inline static int64_t _quote(int64_t amount1, int64_t balance1, int64_t balance2) {
        graphene_assert(amount1 > 0 && balance1 > 0 && balance2 > 0, "amount or balance can't less than 1!");
        return int64_t((__int128_t)amount1 * (__int128_t)balance2 / (__int128_t)balance1);
    }

    // 有待商榷.
    const int64_t MINLIQUIDITY = 100;
    const int64_t ADMINACCOUNT = 22;

   //@abi table bank i64
    struct bank{
        uint64_t owner;
        std::map<uint64_t, int64_t> assert_bank;
        std::map<uint64_t, int64_t> liquid_bank;

        uint64_t primary_key() const {return owner;}

        GRAPHENE_SERIALIZE(bank, (owner)(assert_bank)(liquid_bank))
    };

    typedef graphene::multi_index<N(bank), bank> bank_index;

    bank_index banks;

    //@abi table pool i64
    struct pool{
        uint64_t index;
        contract_asset balance1;
        contract_asset balance2;
        std::string symbol;
        int64_t  total_lq;
        bool locked;// 0为开放状态，1为解锁状态，待定
        
        uint64_t primary_key() const {return index;}
        GRAPHENE_SERIALIZE(pool, (index)(balance1)(balance2)(symbol)(total_lq)(locked))
    };
    
    typedef graphene::multi_index<N(pool), pool> pool_index;

    pool_index pools;
};
GRAPHENE_ABI(swap, (deposit)(addliquidity)(removeliquidity)(swapexacttokensfortokens)(swaptokensforexacttokens)(wthdraw)(transferliquidity)(managepool))