#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/dispatcher.hpp>
#include <map>
#include <vector>
#include <tuple>
#include <math.h>

#include "./message.hpp"

using namespace graphene;

// 最小流动性.
static const int64_t MINLIQUIDITY       = 100;
// 管理员账户.
static const int64_t ADMINACCOUNT       = 22;
// 黑洞账户.
static const int64_t BLACKHOLEACCOUNT   = 3;
// 手续费比例.
static const int64_t FEENUMERATOR       = 997;
static const int64_t FEEDENOMINATOR     = 1000;

template<class T>
inline static T _safe_add(const T& a, const T& b) {
    T result = a + b;
    graphene_assert(result >= a && result >= b, NUMBER_OVERFLOW);
    return result;
}

template<class T>
inline static T _safe_sub(const T& a, const T& b) {
    T result = a - b;
    graphene_assert(a >= result && result >= 0, NUMBER_OVERFLOW);
    return result;
}

template<class T, class U = __int128_t>
inline static T _quote(const T& value1, const T& total1, const T& total2) {
    graphene_assert(value1 > 0 && total1 > 0 && total2 > 0, NUMBER_OVERFLOW);
    U large_value1 = static_cast<U>(value1);
    U large_total1 = static_cast<U>(total1);
    U large_total2 = static_cast<U>(total2);
    U mul_result = large_value1 * large_total2;
    graphene_assert(mul_result >= large_value1 && mul_result >= large_total2, NUMBER_OVERFLOW);
    U large_result = mul_result / large_total1;
    T result = static_cast<T>(large_value1);
    graphene_assert(static_cast<U>(result) * large_total1 == mul_result, NUMBER_OVERFLOW);
    return result;
}

inline static uint64_t _make_index(const uint64_t& number1, const uint64_t& number2) {
    uint64_t _number1 = number1 << 32;
    graphene_assert(_number1 > number1, NUMBER_OVERFLOW);
    return _safe_add(_number1, number2);
}

inline static uint64_t _make_pool_index(const std::string& coin1, const std::string& coin2) {
    auto id1 = get_asset_id(coin1.c_str(), coin1.size());
    auto id2 = get_asset_id(coin2.c_str(), coin2.size());
    graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, INVALID_TRADING_PAIR);
    uint64_t number1 = id1 < id2 ? (uint64_t)id1 : (uint64_t)id2; 
    uint64_t number2 = id1 < id2 ? (uint64_t)id2 : (uint64_t)id1;
    return _make_index(number1, number2);
}

inline static int64_t _get_amount_in(int64_t amount_out, int64_t balance_in, int64_t balance_out) {
    graphene_assert(amount_out > 0 && balance_in > 0 && balance_out > amount_out, INSUFFICIENT_AMOUNT);
    __int128_t numerator = (__int128_t)balance_in * (__int128_t)amount_out * FEEDENOMINATOR;
    graphene_assert(numerator >= balance_in && numerator >= amount_out && numerator >= FEEDENOMINATOR, NUMBER_OVERFLOW);
    __int128_t diff = _safe_sub(balance_out, amount_out);
    __int128_t denominator = diff * FEENUMERATOR;
    graphene_assert(denominator >= diff && denominator >= FEENUMERATOR, NUMBER_OVERFLOW);
    return _safe_add<int64_t>(numerator / denominator, 1);
}

inline static int64_t _get_amount_out(int64_t amount_in, int64_t balance_in, int64_t balance_out ){
    graphene_assert(amount_in > 0 && balance_in > 0 && balance_out > 0, INSUFFICIENT_AMOUNT);
    __int128_t amount_in_with_fee = (__int128_t)amount_in * FEENUMERATOR;
    graphene_assert(amount_in_with_fee >= amount_in && amount_in >= FEENUMERATOR, NUMBER_OVERFLOW);
    __int128_t numerator = amount_in_with_fee * (__int128_t)balance_out;
    graphene_assert(numerator >= amount_in_with_fee && numerator >= balance_out, NUMBER_OVERFLOW);
    __int128_t denominator = (__int128_t)balance_in * FEEDENOMINATOR + amount_in_with_fee;
    graphene_assert(denominator >= balance_in && denominator >= amount_in_with_fee && denominator >= FEEDENOMINATOR, NUMBER_OVERFLOW);
    return (numerator / denominator);
}

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
        void deposit() {
            uint64_t sender = get_trx_sender();
            int64_t asset_amount = get_action_asset_amount();
            uint64_t asset_id = get_action_asset_id();
            graphene_assert(asset_amount > 0 && asset_id > 0, INVALID_PARAMS);
            
            auto bank_itr = banks.find(sender);
            if(bank_itr == banks.end()) {
                banks.emplace( sender, [&](bank &o) {
                    o.owner = sender;
                    o.asset_bank.insert(std::pair<uint64_t, int64_t>(asset_id, asset_amount));
                });
            } else {
                auto assert_iterator = bank_itr->asset_bank.find(asset_id);
                if (assert_iterator == bank_itr->asset_bank.end()){
                    banks.modify(bank_itr, sender, [&](auto& o){
                        o.asset_bank.insert(std::pair<uint64_t, int64_t>(asset_id, asset_amount));
                    });
                } else {
                    banks.modify(bank_itr, sender, [&](auto& o){
                        o.asset_bank[asset_id] = _safe_add(o.asset_bank[asset_id], asset_amount);
                    });    
                }
            }
        }

        //@abi action
        void withdraw(std::string coin,
            std::string to,
            int64_t amount
        ) {
            graphene_assert(amount > 0, INVALID_PARAMS);

            auto sender = get_trx_sender();
            auto bank_itr = banks.find(sender);
            graphene_assert(bank_itr != banks.end(), INVALID_SENDER_ACCOUNT);

            auto asset_id = get_asset_id(coin.c_str(), coin.size());
            graphene_assert(asset_id != -1, INVALID_PARAMS);

            bool remove = false;
            banks.modify(bank_itr, sender, [&](bank& b) {
                auto asset_itr = b.asset_bank.find(asset_id);
                graphene_assert(asset_itr != b.asset_bank.end() && asset_itr->second >= amount, INSUFFICIENT_AMOUNT);
                asset_itr->second = _safe_sub(asset_itr->second, amount);
                if (asset_itr->second == 0) {
                    b.asset_bank.erase(asset_itr);
                    remove = b.asset_bank.empty() && b.liquid_bank.empty();
                }
            });
            if (remove) {
                banks.erase(bank_itr);
            }

            // 向接受者转账.
            auto to_id = get_account_id(to.c_str(), to.length());
            withdraw_asset(_self, to_id, asset_id, amount);
        }

        //@abi action 
        void addlq(std::string coin1, std::string coin2
            , int64_t amount1_desired, int64_t amount2_desired
            , int64_t amount1_min, int64_t amount2_min
            , std::string to
        ) {
            graphene_assert(amount1_desired > 0 && amount2_desired > 0 && amount1_min > 0 && amount2_min > 0, INVALID_PARAMS);

            auto sender = get_trx_sender();
            // 检查asset_id.
            auto id1 = get_asset_id(coin1.c_str(), coin1.size());
            auto id2 = get_asset_id(coin2.c_str(), coin2.size());
            graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, INVALID_TRADING_PAIR);

            // 计算pool_index.
            uint64_t number1 = id1 < id2 ? (uint64_t)id1 : (uint64_t)id2; 
            uint64_t number2 = id1 < id2 ? (uint64_t)id2 : (uint64_t)id1;
            uint64_t pool_index = _make_index(number1, number2);
            auto pool_itr = pools.find(pool_index);
            // 如果交易对不存在的话则创建.
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
                graphene_assert(!pool_itr->locked, PAIR_LOCKED);
            }
            
            // 根据池内的资金数量计算可以质押的金额.
            const auto& balance1 = pool_itr->balance1.asset_id == id1 ? pool_itr->balance1 : pool_itr->balance2;
            const auto& balance2 = pool_itr->balance2.asset_id == id2 ? pool_itr->balance2 : pool_itr->balance1;
            int64_t amount1 = 0, amount2 = 0;
            if (balance1.amount == 0 && balance2.amount == 0) {
                amount1 = amount1_desired;
                amount2 = amount2_desired;
            }
            else {
                auto amount2_optimal = _quote(amount1_desired, balance1.amount, balance2.amount);
                if (amount2_optimal <= amount2_desired) {
                    graphene_assert(amount2_optimal >= amount2_min, INSUFFICIENT_AMOUNT);
                    amount1 = amount1_desired;
                    amount2 = amount2_optimal;
                } else {
                    auto amount1_optimal = _quote(amount2_desired, balance2.amount, balance1.amount);
                    graphene_assert(amount1_optimal <= amount1_desired && amount1_optimal >= amount1_min, INSUFFICIENT_AMOUNT);
                    amount1 = amount1_optimal;
                    amount2 = amount2_desired;
                }
            }
            graphene_assert(amount1 > 0 && amount2 > 0, INSUFFICIENT_AMOUNT);

            // 计算增加的流动性.
            int64_t lq = 0;
            if (pool_itr->total_lq == 0) {
                lq = _safe_sub((int64_t)sqrt(static_cast<__int128_t>(amount1) * static_cast<__int128_t>(amount2)), MINLIQUIDITY);
                // 将最小流动性分配给黑洞账号.
                auto black_hole_bank_itr = banks.find(BLACKHOLEACCOUNT);
                if (black_hole_bank_itr == banks.end()) {
                    banks.emplace(sender, [&](bank& b) {
                        b.owner = BLACKHOLEACCOUNT;
                        b.liquid_bank[pool_index] = MINLIQUIDITY;
                    });
                }
                else {
                    banks.modify(*black_hole_bank_itr, sender, [&](bank& b) {
                        b.liquid_bank[pool_index] = MINLIQUIDITY;
                    });
                }
            }
            else {
                graphene_assert(balance1.amount > 0 && balance2.amount > 0, INSUFFICIENT_AMOUNT);
                lq = std::min(_quote(amount1, balance1.amount, pool_itr->total_lq)
                    , _quote(amount2, balance2.amount, pool_itr->total_lq));
            }
            graphene_assert(lq > 0, INSUFFICIENT_LIQUIDITY);

            // 修改接受者流动性代币的余额.
            auto to_account_id = get_account_id(to.c_str(), to.size());
            if (to_account_id != sender) {
                graphene_assert(to_account_id != -1, INVALID_TO_ACCOUNT);
                auto to_bank_itr = banks.find(to_account_id);
                if (to_bank_itr == banks.end()) {
                    banks.emplace(sender, [&](bank& b) {
                        b.owner = to_account_id;
                        b.liquid_bank[pool_index] = lq;
                    });
                }
                else {
                    banks.modify(*to_bank_itr, sender, [&](bank& b) {
                        b.liquid_bank[pool_index] = _safe_add(b.liquid_bank[pool_index], lq);
                    });
                }
            }

            auto bank_itr = banks.find(sender);
            graphene_assert(bank_itr != banks.end(), INVALID_SENDER_ACCOUNT);
            // 修改bank中的代币余额.
            bool remove = false;
            banks.modify(*bank_itr, sender, [&](bank& b) {
                auto asset1_itr = b.asset_bank.find(id1);
                auto asset2_itr = b.asset_bank.find(id2);
                graphene_assert(asset1_itr != bank_itr->asset_bank.end()
                    && asset2_itr != bank_itr->asset_bank.end()
                    && asset1_itr->second >= amount1
                    && asset2_itr->second >= amount2
                    , INSUFFICIENT_AMOUNT);
                asset1_itr->second = _safe_sub(asset1_itr->second, amount1);
                if (asset1_itr->second == 0) {
                    b.asset_bank.erase(asset1_itr);
                }
                asset2_itr->second = _safe_sub(asset2_itr->second, amount2);
                if (asset2_itr->second == 0) {
                    b.asset_bank.erase(asset2_itr);
                }

                if (to_account_id == sender) {
                    b.liquid_bank[pool_index] = _safe_add(b.liquid_bank[pool_index], lq);
                }
                remove = b.asset_bank.empty() && b.liquid_bank.empty();
            });
            if (remove) {
                banks.erase(bank_itr);
            }

            // 修改pool中的代币余额, 同时增加流动性代币总量.
            pools.modify(*pool_itr, sender, [&](pool& p) {
                auto& _balance1 = p.balance1.asset_id == id1 ? p.balance1 : p.balance2;
                auto& _balance2 = p.balance2.asset_id == id2 ? p.balance2 : p.balance1;
                _balance1.amount = _safe_add(_balance1.amount, amount1);
                _balance2.amount = _safe_add(_balance2.amount, amount2);
                p.total_lq = p.total_lq == 0 ? _safe_add(lq, MINLIQUIDITY) : _safe_add(p.total_lq, lq);
            });
        }

        //@abi action
        void rmlq(std::string coin1, std::string coin2
            , int64_t lq
            , int64_t amount1_min, int64_t amount2_min
            , std::string to
        ) {
            graphene_assert(lq > 0 && amount1_min > 0 && amount2_min > 0, INVALID_PARAMS);

            auto sender = get_trx_sender();
            // 检查asset_id.
            auto id1 = get_asset_id(coin1.c_str(), coin1.size());
            auto id2 = get_asset_id(coin2.c_str(), coin2.size());
            graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, INVALID_TRADING_PAIR);

            // 计算pool_index.
            uint64_t number1 = id1 < id2 ? (uint64_t)id1 : (uint64_t)id2; 
            uint64_t number2 = id1 < id2 ? (uint64_t)id2 : (uint64_t)id1;
            uint64_t pool_index = _make_index(number1, number2);
            auto pool_itr = pools.find(pool_index);
            // 检查交易对是否存在.
            graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
            graphene_assert(!pool_itr->locked, PAIR_LOCKED);
            graphene_assert(pool_itr->total_lq > 0, INSUFFICIENT_LIQUIDITY);

            // 计算可以兑换的资产数量.
            const auto& balance1 = pool_itr->balance1.asset_id == id1 ? pool_itr->balance1 : pool_itr->balance2;
            const auto& balance2 = pool_itr->balance2.asset_id == id2 ? pool_itr->balance2 : pool_itr->balance1;
            int64_t amount1 = _quote(lq, pool_itr->total_lq, balance1.amount);
            int64_t amount2 = _quote(lq, pool_itr->total_lq, balance2.amount);
            graphene_assert(amount1 > 0 && amount2 > 0 && amount1 >= amount1_min && amount2 >= amount2_min, INSUFFICIENT_AMOUNT);

            // 扣除bank中流动性代币的余额.
            auto bank_itr = banks.find(sender);
            graphene_assert(bank_itr != banks.end() , INVALID_SENDER_ACCOUNT);
            banks.modify(*bank_itr, sender, [&](bank& b) {
                auto lq_itr = b.liquid_bank.find(pool_index);
                graphene_assert(lq_itr != b.liquid_bank.end() && lq_itr->second >= lq, INSUFFICIENT_LIQUIDITY);
                lq_itr->second = _safe_sub(lq_itr->second, lq);
            });
            // 扣除pool中的余额及流动性代币总量.
            pools.modify(*pool_itr, sender, [&](pool& p) {
                auto& _balance1 = p.balance1.asset_id == id1 ? p.balance1 : p.balance2;
                auto& _balance2 = p.balance2.asset_id == id2 ? p.balance2 : p.balance1;
                graphene_assert(_balance1.amount > amount1 && _balance2.amount > amount2, INSUFFICIENT_AMOUNT);
                graphene_assert(p.total_lq > lq, INSUFFICIENT_LIQUIDITY);
                _balance1.amount = _safe_sub(_balance1.amount, amount1);
                _balance2.amount = _safe_sub(_balance2.amount, amount2);
                p.total_lq = _safe_sub(p.total_lq, lq);
            });

            // 向接受者转账.
            auto to_account_id = get_account_id(to.c_str(), to.size());
            withdraw_asset(_self, to_account_id, balance1.asset_id, amount1);
            withdraw_asset(_self, to_account_id, balance2.asset_id, amount2);
        }
        
        //@abi action
        //@abi payable
        void swapa(std::vector<std::string> path
            , int64_t amount_out_min
            , std::string to
        ) {
            graphene_assert(amount_out_min > 0, INVALID_PARAMS);

            auto sender = get_trx_sender();
            graphene_assert(path.size() >= 2, INVALID_PATH);

            auto first_asset_name = path[0];
            auto first_asset_id = get_asset_id(first_asset_name.c_str(), first_asset_name.size());
            graphene_assert(first_asset_id != -1, INVALID_PATH);

            int64_t amount_in = get_action_asset_amount();
            uint64_t asset_in = get_action_asset_id();
            graphene_assert(amount_in > 0 && asset_in > 0, INVALID_PARAMS);
            graphene_assert(asset_in == first_asset_id, INVALID_PATH);

            std::vector<contract_asset> amounts;
            amounts.resize(path.size());
            amounts[0] = contract_asset{ amount_in, asset_in };
            for (auto i = 0; i < path.size() - 1; i++ ) {
                auto pool_itr = pools.find(_make_pool_index(path[i], path[i + 1]));
                graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
                graphene_assert(!pool_itr->locked, PAIR_LOCKED);
                const auto& current = amounts[i];
                amounts[i + 1] = contract_asset{
                    _get_amount_out(current.amount
                        , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance1.amount : pool_itr->balance2.amount
                        , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.amount : pool_itr->balance1.amount)
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.asset_id : pool_itr->balance1.asset_id};
            }

            // 检查是否满足条件.
            const auto& last_amount = amounts[amounts.size() - 1];
            graphene_assert(last_amount.amount >= amount_out_min, INSUFFICIENT_AMOUNT);
            
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
                    reduce_balance.amount = _safe_sub(reduce_balance.amount, next.amount);
                    increase_balance.amount = _safe_add(increase_balance.amount, current.amount);
                });
            }

            // 向接受者转账.
            auto to_account_id = get_account_id(to.c_str(), to.size());
            withdraw_asset(_self, to_account_id, last_amount.asset_id,last_amount.amount);
        }

        //@abi action
        //@abi payable
        void swapb(std::vector<std::string> path
            , int64_t amount_out
            , std::string to
        ) {
            graphene_assert(amount_out > 0, INVALID_PARAMS);

            auto sender = get_trx_sender();
            graphene_assert(path.size() >= 2, INVALID_PATH);
            
            auto last_asset_name = path[path.size() - 1];
            auto last_asset_id = get_asset_id(last_asset_name.c_str(), last_asset_name.size());
            graphene_assert(last_asset_id != -1, INVALID_PATH);

            auto first_asset_name = path[0];
            auto first_asset_id = get_asset_id(first_asset_name.c_str(), first_asset_name.size());
            graphene_assert(first_asset_id != -1, INVALID_PATH);

            int64_t amount_in_max = get_action_asset_amount();
            uint64_t asset_in = get_action_asset_id();
            graphene_assert(amount_in_max > 0 && asset_in > 0, INVALID_PARAMS);
            graphene_assert(asset_in == first_asset_id, INVALID_PATH);

            std::vector<contract_asset> amounts;
            amounts.resize(path.size());
            amounts[amounts.size() - 1] = contract_asset{ amount_out, static_cast<uint64_t>(last_asset_id) };
            for (auto i = path.size() - 1; i > 0; i--) {
                auto pool_itr = pools.find(_make_pool_index(path[i - 1], path[i]));
                graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
                graphene_assert(!pool_itr->locked, PAIR_LOCKED);
                const auto& current = amounts[i];
                amounts[i - 1] = contract_asset{
                    _get_amount_in(current.amount
                        , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.amount : pool_itr->balance1.amount
                        , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance1.amount : pool_itr->balance2.amount)
                    , pool_itr->balance1.asset_id == current.asset_id ? pool_itr->balance2.asset_id : pool_itr->balance1.asset_id };
            }
            // 检查是否满足条件.
            const auto& first_amount = amounts[0];
            graphene_assert(first_amount.amount <= amount_in_max, INSUFFICIENT_AMOUNT);
            // 转回剩余的金额.
            if (first_amount.amount < amount_in_max) {
                withdraw_asset(_self, sender, asset_in, _safe_sub(amount_in_max, first_amount.amount));
            }

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
                    reduce_balance.amount = _safe_sub(reduce_balance.amount, next.amount);
                    increase_balance.amount = _safe_add(increase_balance.amount, current.amount);
                });
            }

            // 向接受者转账.
            auto to_account_id = get_account_id(to.c_str(), to.size());
            withdraw_asset(_self, to_account_id, static_cast<uint64_t>(last_asset_id), amount_out);
        }

        //@abi action
        void approvelq(std::string coin1, std::string coin2
            , std::string to
            , int64_t amount
        ) {
            graphene_assert(amount >= 0, INVALID_PARAMS);

            // 检查交易对是否存在.
            auto pool_index = _make_pool_index(coin1, coin2);
            auto pool_itr  = pools.find(pool_index);
            graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
            graphene_assert(!pool_itr->locked, PAIR_LOCKED);

            auto sender = get_trx_sender();
            auto to_account_id = get_account_id(to.c_str(), to.size());
            graphene_assert(to_account_id != -1 && to_account_id != sender, INVALID_TO_ACCOUNT);

            // 增加调用者对接受者的授权.
            pools.modify(pool_itr, sender, [&](pool& p) {
                auto allowance_index = _make_index(sender, to_account_id);
                p.allowance[allowance_index] = amount;
                if (amount == 0) {
                    p.allowance.erase(allowance_index);
                }
            });
        }

        //@abi action
        void transferlqa(std::string coin1, std::string coin2
            , std::string to
            , int64_t amount
        ) {
            graphene_assert(amount > 0, INVALID_PARAMS);

            // 检查交易对是否存在.
            auto pool_index = _make_pool_index(coin1, coin2);
            auto pool_itr  = pools.find(pool_index);
            graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
            graphene_assert(!pool_itr->locked, PAIR_LOCKED);

            auto sender = get_trx_sender();
            auto to_account_id = get_account_id(to.c_str(), to.size());
            graphene_assert(to_account_id != -1, INVALID_TO_ACCOUNT);

            // 进行转账.
            _transferlq(sender, sender, to_account_id, pool_index, amount);
        }

        //@abi action
        void transferlqb(std::string coin1, std::string coin2
            , std::string from, std::string to
            , int64_t amount
        ) {
            graphene_assert(amount > 0, INVALID_PARAMS);

            // 检查交易对是否存在.
            auto pool_index = _make_pool_index(coin1, coin2);
            auto pool_itr  = pools.find(pool_index);
            graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);
            graphene_assert(!pool_itr->locked, PAIR_LOCKED);

            // 检查发送及接受人是否合法.
            auto sender = get_trx_sender();
            auto from_account_id = get_account_id(from.c_str(), from.size());
            graphene_assert(from_account_id != -1, INVALID_FROM_ACCOUNT);
            auto to_account_id = get_account_id(to.c_str(), to.size());
            graphene_assert(to_account_id != -1, INVALID_TO_ACCOUNT);

            // 扣除发送人对调用人的授权.
            pools.modify(pool_itr, sender, [&](pool& p) {
                auto allowance_index = _make_index(from_account_id, sender);
                auto allowance_itr = p.allowance.find(allowance_index);
                graphene_assert(allowance_itr != p.allowance.end() && allowance_itr->second >= amount, INSUFFICIENT_ALLOWANCE);
                allowance_itr->second = _safe_sub(allowance_itr->second, amount);
                if (allowance_itr->second == 0) {
                    p.allowance.erase(allowance_itr);
                }
            });
            
            // 进行转账.
            _transferlq(sender, from_account_id, to_account_id, pool_index, amount);
        }

        //@abi action
        void managepool(std::string coin1, std::string coin2
            , bool locked
        ) {
            // 检查调用者是否为管理员.
            auto sender = get_trx_sender();
            graphene_assert(sender == ADMINACCOUNT, INVALID_SENDER_ACCOUNT);

            // 检查交易对是否存在.
            auto pool_itr = pools.find(_make_pool_index(coin1, coin2));
            graphene_assert(pool_itr != pools.end(), INVALID_TRADING_PAIR);

            // 修改状态.
            pools.modify(pool_itr, sender, [&](pool& p) {
                p.locked = locked;
            });
        }

    private:
        void _transferlq(uint64_t payer, uint64_t from, uint64_t to, uint64_t pool_index, int64_t amount) {
            // 增加接受者余额.
            auto to_bank_itr = banks.find(to);
            if (to_bank_itr == banks.end()) {
                banks.emplace(payer, [&](bank& b) {
                    b.owner = to;
                    b.liquid_bank[pool_index] = amount;
                });
            }
            else {
                banks.modify(to_bank_itr, payer, [&](bank& b) {
                    b.liquid_bank[pool_index] = _safe_add(b.liquid_bank[pool_index], amount);
                });
            }

            // 扣除发送者余额.
            auto from_bank_itr = banks.find(from);
            graphene_assert(from_bank_itr != banks.end(), INVALID_SENDER_ACCOUNT);
            bool remove = false;
            banks.modify(from_bank_itr, payer, [&](bank& b) {
                auto lq_itr = b.liquid_bank.find(pool_index);
                graphene_assert(lq_itr != b.liquid_bank.end() && lq_itr->second >= amount, INSUFFICIENT_LIQUIDITY);
                lq_itr->second = _safe_sub(lq_itr->second, amount);
                if (lq_itr->second == 0) {
                    b.liquid_bank.erase(lq_itr);
                    remove = b.asset_bank.empty() && b.liquid_bank.empty();
                }
            });
            if (remove) {
                banks.erase(from_bank_itr);
            }
        }

        //@abi table bank i64
        struct bank {
            uint64_t owner;
            std::map<uint64_t, int64_t> asset_bank;
            std::map<uint64_t, int64_t> liquid_bank;

            uint64_t primary_key() const { return owner; }

            GRAPHENE_SERIALIZE(bank, (owner)(asset_bank)(liquid_bank))
        };

        typedef graphene::multi_index<N(bank), bank> bank_index;

        bank_index banks;

        //@abi table pool i64
        struct pool {
            uint64_t index;
            contract_asset balance1;
            contract_asset balance2;
            int64_t  total_lq;
            bool locked; // 0为解锁, 1为锁定.

            std::map<uint64_t, int64_t> allowance;
            
            uint64_t primary_key() const { return index; }
            GRAPHENE_SERIALIZE(pool, (index)(balance1)(balance2)(total_lq)(locked)(allowance))
        };
        
        typedef graphene::multi_index<N(pool), pool> pool_index;

        pool_index pools;
};
GRAPHENE_ABI(swap, (deposit)(addlq)(rmlq)(swapa)(swapb)(withdraw)(managepool)(transferlqa)(transferlqb)(approvelq))