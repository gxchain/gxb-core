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
            {}
    

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
    void addliquidity()
    {

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
    void managepool()
    {

    }

    private:
    const int64_t PRECOSION = 100000;
    const int64_t NEWPAIR = 500*PRECOSION;
    const int64_t SERVICERATE1 = 3;
    const int64_t SERVICERATE2 = 1000;
    const int64_t MINNUMBER = 100000;
    const int64_t ADMINACCOUNT = 22;
   //@abi table
    struct bank{
        uint64_t owner;
        std::map<uint64_t, int64_t> assert_bank;
        std::map<uint64_t, int64_t> liquid_bank;

        uint64_t primary_key() const {return owner;}

        GRAPHENE_SERIALIZE(bank, (owner)(assert_bank)(liquid_bank))
    };

    typedef graphene::multi_index<N(bank), bank> bank_index;

    bank_index banks;

    //@abi table
    struct pool{
        uint64_t index;
        contract_asset balance1;
        contract_asset balance2;
        std::string symbol;
        int64_t  total_lq;
        
        uint64_t primary_key() const {return index;}
        GRAPHENE_SERIALIZE(pool, (index)(balance1)(balance2)(symbol)(total_lq))
    };
    
    typedef graphene::multi_index<N(pool), pool> pool_index;

    pool_index pools;
};
GRAPHENE_ABI(swap, (deposit)(addliquidity)(removeliquidity)(swapexacttokensfortokens)(swaptokensforexacttokens)(wthdraw)(transferliquidity)(managepool))