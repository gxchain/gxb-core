#include <graphenelib/system.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/global.h>
#include <graphenelib/asset.h>
#include <graphenelib/crypto.h>
#include <graphenelib/action.h>


using namespace graphene;


class testcase : public contract
{
  public:
    testcase(uint64_t id)
        : contract(id), tables(_self, _self){}

    //current_receiver 
    // @abi action
    void currreceiver(){
        uint64_t ins_id = current_receiver();
        print("current contract account id: ", ins_id, "\n");
    }

    //get_action_asset_id 
    // @abi action
    // @abi payable
    void getactassid(){
        uint64_t ast_id = get_action_asset_id();
        print("call action asset id: ",ast_id, "\n");
    }

    //get_action_asset_amount 
    // @abi action
    // @abi payable
    void getactassamo(){
        uint64_t amount = get_action_asset_amount();
        print("call action asset amount: ", amount, "\n");      
    }
    
    //deposit
    // @abi action
    // @abi payable
    void getactass(){
        uint64_t ast_id = get_action_asset_id();
        int64_t amount = get_action_asset_amount();
        
        print("call action asset id: ", ast_id, "\n");
        print("call action asset amount: ", amount, "\n");  
    }
    //withdraw_asset 
    // @abi action
    void withdrawass(uint64_t to, uint64_t asset_id, int64_t amount){
        withdraw_asset(_self,to,asset_id,amount);
        print("withdraw_asset example\n");
    }

    //get_balance 
    // @abi action
    void getbalance(int64_t account, int64_t asset_id){
        int64_t balance = get_balance(account, asset_id);
        print("account balance: ", balance, "\n");
    }
    //sha256 
    // @abi action
    void sha25(std::string data){
        checksum256 hash;
        sha256(data.c_str(),data.length(),&hash);
        printhex(hash.hash,32);
        print("\n");
    }

    //sha512 
    // @abi action
    void sha51(std::string data){
        checksum512 hash;
        sha512(data.c_str(),data.length(),&hash);
        printhex(hash.hash,64);
        print("\n");
    }

    //ripemd160 
    // @abi action
    void ripemd(std::string data){
        checksum160 hash;
        ripemd160(data.c_str(),data.length(),&hash);
        printhex(hash.hash,20);
        print("\n");
    }

    //verify_signature (other example: redpacket)
    // @abi action
    void verifysign(std::string data,signature sig,std::string pk){
        bool result;
        result = verify_signature(data.c_str(), data.length(), &sig, pk.c_str(), pk.length());
        print("verify result: ", result, "\n");
    }

    // assertreckey
    // @abi action
    void assertreckey(checksum256 hash,signature sig,std::string pkey)
    {
        assert_recover_key(&hash, &sig, pkey.c_str(), pkey.length());
    }
    //get_head_block_num 
    // @abi action
    void getheadblnum(){
        int64_t head_num = get_head_block_num();
        print("head block num: ",head_num, "\n");
    }

    //get_head_block_id 
    // @abi action
    void getheadblid(){
        checksum160 block_hash;
        get_head_block_id(&block_hash);
        printhex(block_hash.hash,20);
        print("\n");
    }

    //get_block_id_for_num 
    // @abi action
    void getblidbynum(uint64_t num){
        checksum160 block_hash;
        get_block_id_for_num(&block_hash,num);             
        printhex(block_hash.hash,20);
        print("\n");
    }

    //get_head_block_time 
    // @abi action
    void getheadbltim(){
        int64_t head_time;
        head_time = get_head_block_time();
        print("head block time: ", head_time, "\n");
    }

    //get_trx_sender 
    // @abi action
    void gettrxsender(){
        int64_t sender_id;
        sender_id = get_trx_sender();
        print("call action instance id: ", sender_id, "\n");
    }

    //get_account_id 
    // @abi action
    void getaccid(std::string data){
        int64_t acc_id;
        acc_id = get_account_id(data.c_str(), data.length());
        print("account id: ", acc_id, "\n");
    }

    //get_account_name_by_id 
    // @abi action
    void getaccname(int64_t accid){
        char data[13]={0};
        int64_t result;
        result = get_account_name_by_id(data,13,accid);
        prints(data);
        print("\n");
    }

    //get_asset_id 
    // @abi action
    void getassid(std::string data){
        int64_t assid;
        assid = get_asset_id(data.c_str(),data.length());
        print("asset id: ", assid, "\n");
    }

    //read_transaction
    // @abi action
    void readtransact(){
        int dwsize;
        dwsize =transaction_size();
        char* pBuffer = new char[dwsize];
        uint32_t size = read_transaction(pBuffer,dwsize);
        print("hex buffer: ");
        printhex(pBuffer,dwsize);
        print("\n");
        delete[] pBuffer;
    }
    
    // @abi action
    void transactsize(){
        int dwsize;
        dwsize =transaction_size();
        print("the size of the serialize trx: ", dwsize, "\n");
    }

    //expiration 
    // @abi action
    void expirationt(){
        uint64_t timenum = expiration();
        print("the expiration time: ", timenum, "\n");
    }

    //tapos_block_num 
    // @abi action
    void taposblknum(){
        uint64_t tapos_num;
        tapos_num = tapos_block_num();
        print("ref block num: ",tapos_num, "\n");
    }

    //tapos_block_prefix
    // @abi action
    void taposblkpref(){
        uint64_t tapos_prefix;
        tapos_prefix = tapos_block_prefix();
        print("ref block id: ",tapos_prefix, "\n");
    }

    //graphene_assert 
    // @abi action
    void graphassert(){
        uint64_t number=1;
        graphene_assert(number == 1, "wrong!");
    }

    //graphene_assert_message 
    // @abi action
    void graphassertm(){
        uint64_t number=1;
        std::string msg = "wrong!!!";
        graphene_assert_message(number == 1, msg.c_str(),msg.length()); 
    }

    //print
    // @abi action
    void printmsg(){
        print("example example example!!!\n");
    }
    struct substruct{
        int32_t number;
        int64_t result;
    };
    // @abi action
    void batchadd(uint32_t field1, uint16_t field2, uint8_t field3,std::vector<uint32_t> vec4,
    std::string str5,uint32_t sub_number6,uint64_t sub_result6, contract_asset asset7, signature sig8, public_key pk9, checksum256 hash25610, checksum160 hash16011, 
    uint64_t count){
        uint64_t number = 0;
        while(true){
            tables.emplace(_self, [&](auto &obj) {
                    obj.id = tables.available_primary_key();                      //o.owner = 1、2、3、4...
                    obj.field1 = field1;
                    obj.field2 = field2 ;
                    obj.field3 = field3;

                    obj.vec4 = vec4;
                    obj.str5 = str5;

                    obj.sct6.number = sub_number6;
                    obj.sct6.result = sub_result6;
                    obj.asset7 = asset7;

                    obj.sig8 = sig8;

                    obj.pk9 = pk9;

                    obj.hash25610 = hash25610;

                    obj.hasn16011 = hash16011;
            });
            number++;
            if(number>count)
                break;
        }
    }
    // @abi action
    void batchmodify(uint32_t field1, uint16_t field2, uint8_t field3, std::vector<uint32_t> vec4, std::string str5,
        uint32_t sub_number6,uint64_t sub_result6, contract_asset asset7, signature sig8, public_key pk9, checksum256 hash25610, checksum160 hash16011){
        for(auto itor = tables.begin(); itor != tables.end(); itor++){
            tables.modify(itor,0,[&](auto &obj){
                    obj.field1 = field1;
                    obj.field2 = field2 ;
                    obj.field3 = field3;

                    obj.vec4 = vec4;
                    obj.str5 = str5;

                    obj.sct6.number = sub_number6;
                    obj.sct6.result = sub_result6;
                    obj.asset7 = asset7;

                    obj.sig8 = sig8;

                    obj.pk9 = pk9;

                    obj.hash25610 = hash25610;

                    obj.hasn16011 = hash16011;
            });
        }
    }
    // @abi action
    void batchdel(){
        for(auto itor = tables.begin(); itor != tables.end();){
            itor = tables.erase(itor);
        }
    }
    struct param {
        std::string name;
    };
    // @abi action
    void inlinecall(){
        param par{"gxchain "};
        action contract_b_name("hello", N(hi), std::move(par), _self, {1000,1});
        contract_b_name.send();
    }
private:
    //@abi table mytable i64
    struct mytable {
        uint64_t id;
        uint32_t field1;
        uint16_t  field2;
        uint8_t   field3;
        
        std::vector<uint32_t>    vec4;
        std::string         str5; 
        
        substruct sct6;

        contract_asset asset7;
        signature sig8;
        public_key pk9;
        checksum256 hash25610;
        checksum160 hasn16011;


        uint64_t primary_key() const { return id; }
        uint64_t byfield1() const { return field1; } 

        GRAPHENE_SERIALIZE(mytable, (id)(field1)
        (field2)(field3)
        (vec4)(str5)
        (sct6)
        (asset7)
        (sig8)(pk9)(hash25610)(hasn16011)
        )
    };
    typedef graphene::multi_index<N(mytable), mytable,
        indexed_by<N(field1), const_mem_fun<mytable, uint64_t, &mytable::byfield1>>
        > table_index;

    table_index tables;

};

GRAPHENE_ABI(testcase,(currreceiver)(getactassid)(getactassamo)(getactass)(withdrawass)(getbalance)(sha25)(sha51)(ripemd)(verifysign)(assertreckey)(getheadblnum)(getheadblid)(getblidbynum)(getheadbltim)(gettrxsender)(getaccid)(getaccname)(getassid)(readtransact)(transactsize)(expirationt)(taposblknum)(taposblkpref)(graphassert)(graphassertm)(printmsg)(batchadd)(batchmodify)(batchdel)(inlinecall))
