#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <vector>

using namespace graphene;

class idx : public contract
{
   public:
    idx(uint64_t id)
        : contract(id)
        , records(_self,_self)
    {
    }
    
    // @abi action
    // @abi payable
    void deposit(uint64_t user_id, std::string character)
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        
        int64_t guess_amount = asset_amount * 0.9;
        _additem(user_id, character,guess_amount);
        
    }
    
    // @abi action
    void deltable()
    {
        for(auto itor = records.begin(); itor!=records.end();){
            itor = records.erase(itor);
        }
        print("deltable success","\n");
    }

    // @abi action
    void gettable(uint64_t uid)
    {
        auto x = records.find(uid);
        print(x->id);
    }
    
    private:
    //@abi table record i64
    struct record {
        uint64_t id;
        uint64_t user_id;
        uint64_t character; //a->10,b->11...and so on
        uint64_t amount;

        uint64_t primary_key() const { return id; }
        uint64_t byidx() const{return character;}
        GRAPHENE_SERIALIZE(record, (id)(user_id)(character)(amount))
    };
    
    typedef graphene::multi_index<N(record),record,
        indexed_by<N(character), const_mem_fun<record, uint64_t, &record::byidx>>
        > record_index;
    record_index records;

    //add guess record
    void _additem(uint64_t user_id,std::string character, uint64_t amount){
        uint64_t ower = get_trx_sender(); 
        print("%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
        records.emplace(_self,[&](auto &obj){
            obj.id = user_id;//records.available_primary_key();
            obj.user_id = user_id;
            obj.character = _charter_to_num(character);
            obj.amount = amount;
        });
    }
    
    //string to int
    int _charter_to_num(std::string character)
    {
        uint64_t number;
        if(character == "a"){
            return 10;
        }else if(character == "b"){
            return 11;
        }else if(character == "c"){
            return 12;
        }else if(character == "d"){
            return 13;
        }else if(character == "e"){
            return 14;
        }else if(character == "f"){
            return 15;
        }else{
            return std::stoi(character);
        }
    }
};

GRAPHENE_ABI(idx,(deposit)(deltable)(gettable))
