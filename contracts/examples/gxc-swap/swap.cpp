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
        uint64_t sender = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        
        auto owner_iterator = banks.find(sender);
        if(owner_iterator == banks.end()) {
            banks.emplace( sender, [&](bank &o) {
                o.owner = sender;
                o.asset_bank.insert(std::pair<uint64_t, int64_t>(asset_id,asset_amount));
            });
        } else {
            auto assert_iterator = (*owner_iterator).asset_bank.find(asset_id);
            if(assert_iterator == (*owner_iterator).asset_bank.end()){
                banks.modify(owner_iterator, sender, [&](auto& o){
                    o.asset_bank.insert(std::pair<uint64_t, int64_t>(asset_id,asset_amount));
                });
            } else {
                banks.modify(owner_iterator, sender, [&](auto& o){
                    o.asset_bank[asset_id] += asset_amount;
                });    
            }
        }
    }

    //@abi action 
    void addlq(std::string coin1, std::string coin2
        , int64_t amount1_desired, int64_t amount2_desired
        , int64_t amount1_min, int64_t amount2_min
        , std::string to
    ) {
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
        else {
            graphene_assert(!pool_itr->locked, "The trading pair has been locked.");
        }
        
        // 根据池内的资金数量计算可以质押的金额.
        int64_t amount1, amount2;
        if (pool_itr->balance1.amount == 0 && pool_itr->balance2.amount == 0) {
            amount1 = amount1_desired;
            amount2 = amount2_desired;
        }
        else {
            auto amount2_optimal = _quote(amount1_desired, pool_itr->balance1.amount, pool_itr->balance2.amount);
            if (amount2_optimal <= amount2_desired) {
                graphene_assert(amount2_optimal >= amount2_min, "insufficient coin2 amount");
                amount1 = amount1_desired;
                amount2 = amount2_optimal;
            } else {
                auto amount1_optimal = _quote(amount2_desired, pool_itr->balance2.amount, pool_itr->balance1.amount);
                graphene_assert(amount1_optimal <= amount1_desired && amount1_optimal >= amount1_min, "insufficient coin1 amount");
                amount1 = amount1_optimal;
                amount2 = amount2_desired;
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
        auto asset1_itr = bank_itr->asset_bank.find(id1);
        auto asset2_itr = bank_itr->asset_bank.find(id2);
        graphene_assert(asset1_itr != bank_itr->asset_bank.end()
            && asset2_itr != bank_itr->asset_bank.end()
            && asset1_itr->second >= amount1
            && asset2_itr->second >= amount2
            , "insufficient amount");
        // 修改bank中的代币余额.
        banks.modify(*bank_itr, sender, [&](bank& b) {
            auto _asset1_itr = b.asset_bank.find(id1);
            auto _asset2_itr = b.asset_bank.find(id2);
            _asset1_itr->second -= amount1;
            if (_asset1_itr->second == 0) {
                b.asset_bank.erase(_asset1_itr);
            }
            _asset2_itr->second -= amount2;
            if (_asset2_itr->second == 0) {
                b.asset_bank.erase(_asset2_itr);
            }

            if (to_account_id == sender) {
                b.liquid_bank[pool_index] += lq;
            }
        });
        // 修改pool中的代币余额, 同时增加流动性代币总量.
        pools.modify(*pool_itr, sender, [&](pool& p) {
            p.balance1.amount += amount1;
            p.balance2.amount += amount2;
            p.total_lq = p.total_lq == 0 ? lq + MINLIQUIDITY : p.total_lq + lq;
        });
    }

    //@abi action
    void rmlq(std::string coin1, std::string coin2
        , int64_t lq
        , int64_t amount1_min, int64_t amount2_min
        , std::string to
    ) {
        auto sender = get_trx_sender();
        // 检查交易对是否存在.
        auto pool_index = _make_pool_index(coin1, coin2);
        auto pool_itr = pools.find(pool_index);
        graphene_assert(pool_itr != pools.end(), "The trading pair does not exist.");
        graphene_assert(!pool_itr->locked, "The trading pair has been locked.");

        // 计算可以兑换的资产数量.
        int64_t amount1 = (__int128_t)lq * (__int128_t)pool_itr->balance1.amount / (__int128_t)pool_itr->total_lq;
        int64_t amount2 = (__int128_t)lq * (__int128_t)pool_itr->balance2.amount / (__int128_t)pool_itr->total_lq;
        graphene_assert(amount1 > 0 && amount2 > 0 && amount1 >= amount1_min && amount2 >= amount2_min, "Insufficient amount");

        // 向接受者转账.
        auto to_account_id = get_account_id(to.c_str(), to.size());
        graphene_assert(to_account_id != -1, "illegal account!");
        withdraw_asset(_self, to_account_id, pool_itr->balance1.asset_id, amount1);
        withdraw_asset(_self, to_account_id, pool_itr->balance2.asset_id, amount2);

        // 扣除bank中流动性代币的余额.
        auto bank_itr = banks.find(sender);
        graphene_assert(bank_itr != banks.end() , "Insufficient liquidity");
        banks.modify(*bank_itr, sender, [&](bank& b) {
            graphene_assert(b.liquid_bank[pool_index] >= lq, "Insufficient liquidity");
            b.liquid_bank[pool_index] -= lq;
        });
        // 扣除pool中的余额及流动性代币总量.
        pools.modify(*pool_itr, sender, [&](pool& p) {
            graphene_assert(p.balance1.amount > amount1 && p.balance2.amount > amount2, "Insufficient amount");
            p.balance1.amount -= amount1;
            p.balance2.amount -= amount2;
            p.total_lq -= lq;
        });
    }
    
    //@abi action
    void swapetkfortk(
        std::vector<std::string> path
        , int64_t amount_out_min
        , int64_t amount_in
        , std::string to
    ) {
        auto sender = get_trx_sender();
        graphene_assert(path.size() >= 2, "Invalid path");

        auto first_asset_name = path[0];
        auto first_asset_id = get_asset_id(first_asset_name.c_str(), first_asset_name.size());
        graphene_assert(first_asset_id != -1, "Invalid path");

        std::vector<contract_asset> amounts;
        amounts.resize(path.size());
        amounts[0] = contract_asset{ amount_in, static_cast<uint64_t>(first_asset_id) };
        for (auto i = 0; i < path.size() - 1; i++ ){
            auto pool_itr = pools.find(_make_pool_index(path[i], path[i+1]));
            graphene_assert(pool_itr != pools.end(), "The trading pair does not exist.");
            graphene_assert(!pool_itr->locked, "The trading pair has been locked.");
            const auto& current = amounts[i];
            amounts[i+1] = contract_asset{
                _get_amount_out(current.amount
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.amount : pool_itr->balance1.amount
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance1.amount : pool_itr->balance2.amount)
                , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.asset_id : pool_itr->balance1.asset_id};
        }
        graphene_assert(amounts[amounts.size()-1].amount >= amount_out_min, "Insufficient amount");

        // 增加接受者余额.
        auto bank_itr = banks.find(sender);
        graphene_assert(bank_itr != banks.end(), "Invalid sender");
        auto to_account_id = get_account_id(to.c_str(), to.size());
        if (sender != to_account_id) {
            graphene_assert(to_account_id != -1, "illegal account!");
            auto to_bank_itr = banks.find(to_account_id);
            if (to_bank_itr == banks.end()) {
                banks.emplace(sender, [&](bank& b) {
                    b.owner = to_account_id;
                    b.asset_bank[first_asset_id] = amounts[amounts.size()-1].amount;
                });
            }
            else {
                banks.modify(*to_bank_itr, sender, [&](bank& b) {
                    b.asset_bank[first_asset_id] += amounts[amounts.size()-1].amount;
                });
            }
        }

        // 扣除调用者在bank中的余额
        banks.modify(bank_itr, sender, [&](bank& b) {
            auto asset_itr = b.asset_bank.find(first_asset_id);
            graphene_assert(asset_itr != b.asset_bank.end() && asset_itr->second >= amount_in, "Insufficient amount");
            asset_itr->second -= amount_in;

            if (sender == to_account_id) {
                b.asset_bank[amounts[amounts.size() - 1].asset_id] += amounts[amounts.size() - 1].amount;
            }
        });

        // 依次更改每个pool中的资金数量.
        for (auto i = 0; i < path.size() - 1; i++) {
            auto pool_itr = pools.find(_make_pool_index(path[i], path[i + 1]));
            pools.modify(pool_itr, sender, [&](pool& p) {
                const auto& current = amounts[i];
                const auto& next = amounts[i + 1];
                auto& reduce_balance = p.balance1.asset_id == current.asset_id
                    ? p.balance2
                    : p.balance1;
                auto& increase_balance = p.balance1.asset_id == current.asset_id
                    ? p.balance1
                    : p.balance2;
                reduce_balance.amount -= next.amount;
                increase_balance.amount += current.amount;
            });
        }
    }

    //@abi action
    //@abi payable
    void swaptkforetk(std::vector<std::string> path
        , int64_t amount_out
        , std::string to
    ) {
        auto sender = get_trx_sender();
        graphene_assert(path.size() >= 2, "Invalid path");
        
        auto last_asset_name = path[path.size() - 1];
        auto last_asset_id = get_asset_id(last_asset_name.c_str(), last_asset_name.size());
        graphene_assert(last_asset_id != -1, "Invalid path");

        int64_t amount_in_max = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        auto first_asset_name = path[0];
        auto first_asset_id = get_asset_id(first_asset_name.c_str(), first_asset_name.size());
        graphene_assert(first_asset_id == asset_id, "Invalid path");

        std::vector<contract_asset> amounts;
        amounts.resize(path.size());
        amounts[amounts.size() - 1] = contract_asset{ amount_out, static_cast<uint64_t>(last_asset_id) };
        for (auto i = path.size() - 1; i > 0; i--) {
            auto pool_itr = pools.find(_make_pool_index(path[i - 1], path[i]));
            graphene_assert(pool_itr != pools.end(), "The trading pair does not exist.");
            graphene_assert(!pool_itr->locked, "The trading pair has been locked.");
            const auto& current = amounts[i];
            amounts[i - 1] = contract_asset{
                _get_amount_in(current.amount
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.amount : pool_itr->balance1.amount
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance1.amount : pool_itr->balance2.amount)
                , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.asset_id : pool_itr->balance1.asset_id };
        }
        const auto& first_amount = amounts[0];
        graphene_assert(first_amount.amount <= amount_in_max, "Insufficient amount");
        // 转回剩余的金额.
        if (first_amount.amount < amount_in_max) {
            withdraw_asset(_self, sender, asset_id, amount_in_max - first_amount.amount);
        }

        auto to_account_id = get_account_id(to.c_str(), to.size());
        graphene_assert(to_account_id != -1, "illegal account!");
        withdraw_asset(_self, to_account_id, static_cast<uint64_t>(last_asset_id), amount_out);

        // 依次更改每个pool中的资金数量.
        for (std::size_t i = 0; i < path.size() - 1; i++) {
            auto pool_itr = pools.find(_make_pool_index(path[i], path[i + 1]));
            pools.modify(*pool_itr, sender, [&](pool& p) {
                const auto& current = amounts[i];
                const auto& next = amounts[i + 1];
                auto& reduce_balance = p.balance1.asset_id == current.asset_id
                    ? p.balance2
                    : p.balance1;
                auto& increase_balance = p.balance1.asset_id == current.asset_id
                    ? p.balance1
                    : p.balance2;
                reduce_balance.amount -= next.amount;
                increase_balance.amount += current.amount;
            });
        }
    }


    //@abi action
    void withdraw(
        std::string coin,
        std::string to,
        int64_t amount
    ) {
        auto sender = get_trx_sender();
        auto bank_itr = banks.find(sender);
        graphene_assert(bank_itr != banks.end(), "missing user");

        //检查用户余额是否足够
        auto asset_id = get_asset_id(coin.c_str(), coin.size());
        auto asset_itr = bank_itr->asset_bank.find(asset_id);

        graphene_assert(asset_itr != bank_itr->asset_bank.end()
            && asset_itr->second >= amount
            , "insufficient amount");

        banks.modify(bank_itr, sender, [&](bank& b) {
            auto _asset_itr = b.asset_bank.find(asset_id);
            _asset_itr->second -= amount;
            if (_asset_itr->second == 0) {
                b.asset_bank.erase(_asset_itr);
            }
        });        
        //取回
        auto to_id = get_account_id(to.c_str(), to.length());
        withdraw_asset(_self, to_id, asset_id, amount);
    }

    //@abi action
    void transferlq(
        std::string coin1, std::string coin2
        , std::string to
        , int64_t amount
    ) {
        auto sender = get_trx_sender();
        auto pool_index = _make_pool_index(coin1, coin2);
        auto pool_itr  = pools.find(pool_index);
        graphene_assert( pool_itr != pools.end(),"illegal liquidty pair");
        graphene_assert( pool_itr->locked == 0, "locked pair!");

        auto to_account_id = get_account_id(to.c_str(), to.size());
        graphene_assert(to_account_id != -1, "illegal account!");

        auto sender_bank_itr = banks.find(sender);
        auto bank_lp_itr = sender_bank_itr->liquid_bank.find(pool_index);
        graphene_assert( sender_bank_itr != banks.end() 
                         && bank_lp_itr != sender_bank_itr->liquid_bank.end() 
                         && bank_lp_itr->second >= amount, "illegal amount!" );
        auto to_bank_itr = banks.find(to_account_id);
        if (to_bank_itr == banks.end()) {
                banks.emplace(sender, [&](bank& b) {
                    b.owner = to_account_id;
                    b.liquid_bank[pool_index] = amount;
                });
        }
        else {
            banks.modify(to_bank_itr, sender, [&](bank& b) {
                b.liquid_bank[pool_index] += amount;
            });
        }
        banks.modify(sender_bank_itr, sender, [&](bank& b) {
            b.liquid_bank[pool_index] -= amount;
        });
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

    inline static int64_t _quote(int64_t amount1, int64_t balance1, int64_t balance2) {
        graphene_assert(amount1 > 0 && balance1 > 0 && balance2 > 0, "amount or balance can't less than 1!");
        return int64_t((__int128_t)amount1 * (__int128_t)balance2 / (__int128_t)balance1);
    }

    inline static int64_t _get_amount_in(int64_t amount_out, int64_t balance_in, int64_t balance_out) {
        graphene_assert(amount_out > 0 && balance_in > 0 && balance_out > 0, "Insufficient liquidity or amount");
        __int128_t numerator = (__int128_t)balance_in * (__int128_t)amount_out * 1000;
        __int128_t denominator = (balance_out - amount_out) * 997;
        return (numerator / denominator) + 1;
    }

    inline static int64_t _get_amount_out(int64_t amount_in, int64_t balance_in, int64_t balance_out ){
        graphene_assert(amount_in > 0 && balance_in > 0 && balance_out > 0, "Insufficient liquidity or amount");
        __int128_t amountInWithFee = (__int128_t)amount_in * 997;
        __int128_t numerator = amountInWithFee *  (__int128_t)balance_out;
        __int128_t denominator = (__int128_t)balance_in * 1000 + amountInWithFee;
        return (numerator / denominator);
    }
    // 有待商榷.
    const int64_t MINLIQUIDITY = 100;
    const int64_t ADMINACCOUNT = 22;

   //@abi table bank i64
    struct bank{
        uint64_t owner;
        std::map<uint64_t, int64_t> asset_bank;
        std::map<uint64_t, int64_t> liquid_bank;

        uint64_t primary_key() const {return owner;}

        GRAPHENE_SERIALIZE(bank, (owner)(asset_bank)(liquid_bank))
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
GRAPHENE_ABI(swap, (deposit)(addlq)(rmlq)(swapetkfortk)(swaptkforetk)(withdraw)(transferlq)(managepool))