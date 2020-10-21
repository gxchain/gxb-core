#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <boost/hana/optional.hpp>

#define INVALID_PARAMS          "Invalid params."
#define INVALID_TRADING_PAIR    "Invalid trading pair."
#define INVALID_ASSET           "Invalid asset, not supported."
#define NO_REWARD               "No available reward rate."

#define NUMBER_OVERFLOW         "Number overflow."

using namespace graphene;

static const uint64_t POOLASSETID   = 1;
static const uint64_t ADMINACCOUNT  = 22;
static const uint64_t SWAPACCOUNT   = 100;

static const uint64_t ASSETFLAG = 1ULL << 32;

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

template<class T>
inline static T _safe_mul(const T& a, const T& b) {
    T result = a * b;
    graphene_assert(result >= a && result >= b, NUMBER_OVERFLOW);
    return result;
}

template<class U, class T>
inline static U _safe_convert(const T& t) {
    U u = static_cast<T>(t);
    graphene_assert(static_cast<T>(u) == t, NUMBER_OVERFLOW);
    return u;
}

// inline static uint64_t _make_index(const uint64_t& number1, const uint64_t& number2) {
//     uint64_t _number1 = number1 << 32;
//     graphene_assert(_number1 > number1, NUMBER_OVERFLOW);
//     return _safe_add(_number1, number2);
// }

// inline static uint64_t _make_pool_index(const std::string& coin1, const std::string& coin2) {
//     auto id1 = get_asset_id(coin1.c_str(), coin1.size());
//     auto id2 = get_asset_id(coin2.c_str(), coin2.size());
//     graphene_assert(id1 != -1 && id2 != -1 && id1 != id2, INVALID_TRADING_PAIR);
//     uint64_t number1 = id1 < id2 ? (uint64_t)id1 : (uint64_t)id2; 
//     uint64_t number2 = id1 < id2 ? (uint64_t)id2 : (uint64_t)id1;
//     return _make_index(number1, number2);
// }

class stakepool : public contract {
    public:
        stakepool(uint64_t account_id)
            : contract(account_id), pools(_self, _self) {}
        
        // @abi action
        // @abi payable
        void stake() {
            int64_t asset_amount = get_action_asset_amount();
            uint64_t asset_id = get_action_asset_id();
            graphene_assert(asset_amount > 0 && asset_id > 0, INVALID_PARAMS);
            _stake(get_trx_sender(), asset_id, asset_amount);
        }

        // @abi action
        void stakelq(uint64_t asset_id, int64_t amount) {
            graphene_assert(asset_id > ASSETFLAG && amount > 0, INVALID_PARAMS);
            _stake(get_trx_sender(), asset_id, amount, true);
        }

        // @abi action
        void withdraw(uint64_t asset_id, int64_t amount) {
            
        }

        // @abi action
        void getreward(uint64_t asset_id) {
            
        }

        // @abi action
        void exit(uint64_t asset_id) {
            
        }

        // @abi action
        // @abi payable
        void notifyreward(uint64_t asset_id, int64_t new_duration) {
            
        }

        // @abi action
        void newpool(uint64_t asset_id, int64_t start_time, int64_t duration) {
            
        }

        // @abi action
        void managepool(uint64_t asset_id, bool locked) {
            
        }

    private:
        void _stake(uint64_t sender, uint64_t asset_id, int64_t asset_amount, bool transferlq = false) {
            auto itr = pools.find(asset_id);
            graphene_assert(itr != pools.end(), INVALID_ASSET);
            if (transferlq) {
                _transferlqb(itr->coin1, itr->coin2, sender, _self, asset_amount);
            }
            pools.modify(itr, sender, [&](pool& p){
                _update_reward_per_token(p, sender);
                p.total_amount += asset_amount;
                p.stake[sender] += asset_amount;
            });
        }

        std::string _get_account_name(uint64_t id) {
            char data[65] = { 0 };
            graphene_assert(get_account_name_by_id(data, 65, id) == 0, "Unkonw account id.");
            return std::string{ data };
        }

        void _transferlqb(const std::string& coin1, const std::string& coin2, uint64_t from, uint64_t to, int64_t amount) {
            transferlqb_params params{ coin1, coin2, _get_account_name(from), _get_account_name(to), amount};
            action act(SWAPACCOUNT, N(transferlqb), std::move(params), _self);
            act.send();
        }

        void _transferlqa(const std::string& coin1, const std::string& coin2, uint64_t to, int64_t amount) {
            transferlqa_params params{ coin1, coin2, _get_account_name(to), amount};
            action act(SWAPACCOUNT, N(transferlqa), std::move(params), _self);
            act.send();
        }

        struct transferlqb_params {
            std::string coin1;
            std::string coin2;
            std::string from;
            std::string to;
            int64_t amount;
        };

        struct transferlqa_params {
            std::string coin1;
            std::string coin2;
            std::string to;
            int64_t amount;
        };

        //@abi table pool i64
        struct pool {
            uint64_t asset_id;
            bool locked;
            bool is_lq;
            std::string coin1;
            std::string coin2;
            int64_t total_amount;
            int64_t reward_rate;
            int64_t reward_per_token;
            int64_t start_time;
            int64_t last_update_time;
            int64_t period_finish;
            int64_t duration;
            std::map<uint64_t, int64_t> last_reward_per_token;
            std::map<uint64_t, int64_t> reward;
            std::map<uint64_t, int64_t> stake;

            uint64_t primary_key() const { return asset_id; }
            GRAPHENE_SERIALIZE(pool, (asset_id)(locked)(is_lq)(coin1)(coin2)(total_amount)(reward_rate)(reward_per_token)(start_time)(last_update_time)(period_finish)(duration)(last_reward_per_token)(reward)(stake))
        };

        typedef graphene::multi_index<N(pool), pool> pool_index;
        pool_index pools;

        int64_t _get_current_time(const pool& p) {
            auto current = get_head_block_time();
            return current < p.period_finish ? current : p.period_finish;
        }

        void _update_reward_per_token(pool& p, boost::hana::optional<uint64_t> sender = {}) {
            if (p.total_amount == 0) {
                return;
            }
            else {
                auto sep = _get_current_time(p) - p.last_update_time;
                if (sep > 0) {
                    auto reward_amount_of_sep = _safe_mul<__uint128_t>(sep, p.reward_rate);
                    p.reward_per_token = _safe_convert<int64_t>(reward_amount_of_sep / (__uint128_t)p.total_amount);
                    graphene_assert(p.reward_per_token > 0, NO_REWARD);
                    if (!boost::hana::is_nothing(sender)) {
                        const auto& const_p = p;
                        auto itr = const_p.stake.find(*sender);
                        if (itr != p.stake.cend()) {
                            p.reward[*sender] += _safe_mul(itr->second, _safe_sub(p.reward_per_token, p.last_reward_per_token[*sender]));
                        }
                    }
                }
            }
        }
};