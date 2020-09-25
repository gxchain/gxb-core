#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/global.h>
#include <vector>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/dispatcher.hpp>
#include <map>

using namespace graphene;

class swap : public contract{
    public:
        swap(uint64_t account_id)
            :contract(account_id)
            ,pools(_self,_self)
            ,banks(_self, _self)
            ,liquiditys(_self,_self)
            {}
    
    //@abi action
    //是否需要payable
    void createpair(std::string coin1, std::string coin2){
        auto coin1_id = get_asset_id(coin1.c_str(),coin1.length());
        auto coin2_id = get_asset_id(coin2.c_str(),coin2.length());
        graphene_assert(coin1_id != coin2_id, "you can not set the self equivalent pair");
        auto id1 = (coin1_id < coin2_id) ? coin1_id : coin2_id;
        auto id2 = (coin1_id < coin2_id) ? coin2_id : coin1_id;
        std::string mark = std::to_string(id1)+"and"+std::to_string(id2);
        auto index = graphenelib::string_to_name(mark.c_str());
        auto map_iterator = pools.find(index);
        graphene_assert(map_iterator == pools.end(),"There had the coin pair, you can not create it again");
        uint64_t creator = get_trx_sender();
        pools.emplace(creator,[&](auto &o){
            o.index = index;
            o.balance1.asset_id = id1;
            o.balance1.amount = 0;
            o.balance2.asset_id = id2;
            o.balance2.amount = 0;
            o.symbol = mark;
            o.symbolTurn = std::to_string(id2)+"and"+std::to_string(id1);
            o.state = 0; 
            o.creator = creator;
        });
    }

    //@abi action
    //@abi payable
    void deposit(std::string coin1, std::string coin2){
        auto coin1_id = get_asset_id(coin1.c_str(),coin1.length());
        auto coin2_id = get_asset_id(coin2.c_str(),coin2.length());
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        graphene_assert(asset_amount >= MINNUMBER,"the min number is 100000");
        graphene_assert(coin1_id != coin2_id, "The two coins are same");
        auto id1 = (coin1_id < coin2_id) ? coin1_id : coin2_id;
        auto id2 = (coin1_id < coin2_id) ? coin2_id : coin1_id;
        std::string mark = std::to_string(id1)+"and"+std::to_string(id2);
        auto index = graphenelib::string_to_name(mark.c_str());
        auto map_iterator = pools.find(index);
        graphene_assert(map_iterator != pools.end(),"There is no such pair existing");
        auto sender = get_trx_sender();
        graphene_assert(sender == (*map_iterator).creator,"You have no authority");
        bool is_valid = (asset_id == (*map_iterator).balance1.asset_id || asset_id == (*map_iterator).balance2.asset_id);
        graphene_assert(is_valid,"The asset you sent is not match");
        pools.modify(map_iterator, sender, [&](auto &o){
            if(asset_id == (*map_iterator).balance1.asset_id){
                o.balance1.amount += asset_amount;
            }
            if(asset_id == (*map_iterator).balance2.asset_id){
                o.balance2.amount += asset_amount;
            }
        });
    }

    //@abi action 
    void active(std::string coin1, std::string coin2){
        auto coin1_id = get_asset_id(coin1.c_str(),coin1.length());
        auto coin2_id = get_asset_id(coin2.c_str(),coin2.length());
        graphene_assert(coin1_id != coin2_id, "The two coins are same");
        auto id1 = (coin1_id < coin2_id) ? coin1_id : coin2_id;
        auto id2 = (coin1_id < coin2_id) ? coin2_id : coin1_id;
        std::string mark = std::to_string(id1)+"and"+std::to_string(id2);
        auto index = graphenelib::string_to_name(mark.c_str());
        auto map_iterator = pools.find(index);
        graphene_assert(map_iterator != pools.end(),"There is no such pair existing");
        auto sender = get_trx_sender();
        graphene_assert(sender == (*map_iterator).creator,"You have no authority");
        __int128_t multiply = (*map_iterator).balance1.amount * (*map_iterator).balance2.amount;
        graphene_assert( multiply != 0,"The multiply is 0 ,plz deposit asset");
        pools.modify(map_iterator, sender, [&](auto &o){
            o.state = 1;
        });
    }

    //@abi action
    //@abi payable
    void swapcoin(std::string target, int64_t minnumber){
        auto coin_id = get_asset_id(target.c_str(),target.length());
        auto asset_id = get_action_asset_id();
        int64_t asset_amount = get_action_asset_amount();
        auto sender = get_trx_sender();
        graphene_assert(asset_amount >= MINNUMBER,"the min number is 100000");
        graphene_assert(coin_id != asset_id, "The two coins are same");
        auto id1 = (coin_id < asset_id) ? coin_id : asset_id;
        auto id2 = (coin_id < asset_id) ? asset_id : coin_id;
        std::string mark = std::to_string(id1)+"and"+std::to_string(id2);
        auto index = graphenelib::string_to_name(mark.c_str());
        auto map_iterator = pools.find(index);
        graphene_assert(map_iterator != pools.end(),"There is no such pair existing");
        __int128_t multiply = __int128_t((*map_iterator).balance1.amount) * __int128_t((*map_iterator).balance2.amount);
        graphene_assert( multiply != 0,"The multiply is 0 ,plz deposit asset");
        graphene_assert((*map_iterator).state !=0," the swap pair is not active");
        
        if(asset_id == (*map_iterator).balance1.asset_id){
            __int128_t service_amount = __int128_t(asset_amount) * __int128_t(SERVICERATE1)/ (__int128_t(SERVICERATE2));
            __int128_t temp_amount1 = __int128_t((*map_iterator).balance1.amount) + __int128_t(asset_amount) - service_amount;
            __int128_t temp_amount2 = multiply/temp_amount1;
            __int128_t exchange_amount = (*map_iterator).balance2.amount- temp_amount2;
            __int128_t _minnumber = minnumber;
            graphene_assert(exchange_amount >= _minnumber, "Slippage is set too small");
            withdraw_asset(_self, (*map_iterator).creator, (*map_iterator).balance1.asset_id, service_amount);
            withdraw_asset(_self, sender, (*map_iterator).balance2.asset_id, exchange_amount);
            pools.modify(map_iterator, sender, [&](auto &o){
                o.balance1.amount = temp_amount1;
                o.balance2.amount = temp_amount2;
            });
        }
        if(asset_id == (*map_iterator).balance2.asset_id){
            __int128_t service_amount = __int128_t(asset_amount) * __int128_t(SERVICERATE1)/ (__int128_t(SERVICERATE2));
            __int128_t temp_amount1 = __int128_t((*map_iterator).balance2.amount) + __int128_t(asset_amount) - service_amount;
            __int128_t temp_amount2 = multiply/temp_amount1;
            __int128_t exchange_amount = (*map_iterator).balance1.amount- temp_amount2;
            __int128_t _minnumber = minnumber;
            graphene_assert(exchange_amount >= _minnumber, "Slippage is set too small");
            withdraw_asset(_self, (*map_iterator).creator, (*map_iterator).balance2.asset_id, service_amount);
            withdraw_asset(_self, sender, (*map_iterator).balance1.asset_id, exchange_amount);
            pools.modify(map_iterator, sender, [&](auto &o){
                o.balance2.amount = temp_amount1;
                o.balance1.amount = temp_amount2;
            });
        }
    }

    //@abi action 
    void withdrawswap(uint64_t id, uint64_t number){
        auto sender = get_trx_sender();
        graphene_assert( sender == ADMINACCOUNT,"you have no authority");
        withdraw_asset(_self, sender, id, number);
    }

    private:
    const int64_t PRECOSION = 100000;
    const int64_t NEWPAIR = 500*PRECOSION;
    const int64_t SERVICERATE1 = 3;
    const int64_t SERVICERATE2 = 1000;
    const int64_t MINNUMBER = 100000;
    const int64_t ADMINACCOUNT = 22;
    //@abi table pool i64
    struct pool{
        uint64_t index;
        contract_asset balance1;
        contract_asset balance2;
        std::string symbol;
        std::string symbolTurn;
        uint64_t creator;
        uint64_t state;
        uint64_t primary_key() const {return index;}
        GRAPHENE_SERIALIZE(pool, (index)(balance1)(balance2)(symbol)(symbolTurn)(creator)(state))
    };

    typedef graphene::multi_index<N(pool), pool> pool_index;

    pool_index pools;

    //@abi table 
    struct bank{
        uint64_t owner;
        std::vector<contract_asset> balances;

        uint64_t primary_key() const {return owner;}
        GRAPHENE_SERIALIZE(bank, (owner)(balances))
    };

    typedef graphene::multi_index<N(bank),bank> bank_index;

    account_index banks;

    //@abi table
    struct liquidity{
        uint64_t owner;
        std::map<std::string,uint64_t> lpbalances;

        uint64_t primary_key() const {return owner;}
        
    };

};
GRAPHENE_ABI(swap, (createpair)(deposit)(active)(swapcoin)(withdrawswap))