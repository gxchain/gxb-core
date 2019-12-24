#include<graphenelib/contract.hpp>
#include<graphenelib/dispatcher.hpp>
#include<graphenelib/print.hpp>
#include<graphenelib/types.h>
#include <graphenelib/multi_index.hpp>

using namespace graphene;

CONTRACT helloworld : public contract   // you can use macro CONTRACT repalce class or not
{
    public :
        helloworld(uint64_t id)
            : contract(id)
            , offers(_self, _self)
        {
        }
    
    ACTION hi1(std::string user1) //now you can use macro ACTION repalce "//@abi action" and "void" 
    {
        print("hi,",user1,"\n");
    }

    //@abi action 
    void hi2(std::string user2)  //you also can use "//@abi action" like before   
    {
        print("hi,",user2,"\n");  
    }

    PAYABLE hi3(std::string user3)//now you can use macro PAYABLE repalce "//@abi payable" and "void"
    {
        print("hi,",user3,"\n");
    }

    //@abi action
    //@abi payable
    void hi4(std::string user4)//you can also use "//@abi payable" like before
    {
        print("hi,",user4,"\n");
    }

    private:
    TABLE offer {     //now you can use macro TABLE replace "//@abi table [tablename] i64" and "struct"
        uint64_t id;  //or you can also use "//@abi table [tablename] i64 "
        uint64_t idx1;
        uint64_t idx2;
        uint64_t stringidx;

        uint64_t primary_key() const { return id; }

        uint64_t by_index1() const { return idx1; }

        uint64_t by_index2() const { return idx2; }
        
        uint64_t by_stringidx() const {return stringidx; }

        GRAPHENE_SERIALIZE(offer, (id)(idx1)(idx2)(stringidx))
    };

    typedef multi_index<N(offer), offer,
                        indexed_by<N(idx1), const_mem_fun<offer, uint64_t, &offer::by_index1>>,
                        indexed_by<N(idx2), const_mem_fun<offer, uint64_t, &offer::by_index2>>,
                        indexed_by<N(stringidx), const_mem_fun<offer, uint64_t, &offer::by_stringidx>>>
        offer_index;

    offer_index offers;
};

GRAPHENE_ABI(helloworld,(hi1)(hi2)(hi3)(hi4))