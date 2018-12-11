#include <graphenelib/system.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/global.h>
#include <graphenelib/asset.h>
#include <graphenelib/crypto.h>



using namespace graphene;


class example : public contract
{
  public:
    example(uint64_t id)
        : contract(id){}

    //current_receiver 
    // @abi action
    void examcurr(){
        uint64_t ins_id = current_receiver();
        print("current contract account id: ", ins_id);
    }

    //get_action_asset_id 
    // @abi action
    // @abi payable
    void examgetast(){
        uint64_t ast_id = get_action_asset_id();
        print("call action asset id: ",ast_id);
    }

    //get_action_asset_amount 
    // @abi action
    // @abi payable
    void examgetamo(){
        uint64_t amount = get_action_asset_amount();
        print("call action asset amount: ",amount);      
    }
    
    //deposit
    // @abi action
    // @abi payable
    void examdepo(){
        uint64_t ast_id = get_action_asset_id();
        int64_t amount = get_action_asset_amount();
        
        print("call action asset id: ",ast_id);
        print("\n");
        print("call action asset amount: ",amount);  
    }
    //withdraw_asset 
    // @abi action
    void examwith(uint64_t to, uint64_t asset_id, int64_t amount){
        withdraw_asset(_self,to,asset_id,amount);
        print("withdraw_asset example");
    }

    //get_balance 
    // @abi action
    void examgetbl(int64_t account, int64_t asset_id){
        int64_t balance = get_balance(account, asset_id);
        print("account balance: ",balance);
    }
    //sha256 
    // @abi action
    void examsha25(std::string data){
        checksum256 hash;
        sha256(data.c_str(),data.length(),&hash);
        printhex(hash.hash,32);
    }

    //sha512 
    // @abi action
    void examsha512(std::string data){
        checksum512 hash;
        sha512(data.c_str(),data.length(),&hash);
        printhex(hash.hash,64);
    }

    //ripemd160 
    // @abi action
    void examripemd(std::string data){
        checksum160 hash;
        ripemd160(data.c_str(),data.length(),&hash);
        printhex(hash.hash,20);
    }

    //verify_signature (other example: redpacket)
    // @abi action
    void examverify(std::string data,signature sig,std::string pk){
        bool result;
        result = verify_signature(data.c_str(), data.length(), &sig, pk.c_str(), pk.length());
        print("verify result: ",result);
    }

    //get_head_block_num 
    // @abi action
    void examgetnum(){
        int64_t head_num = get_head_block_num();
        print("head block num: ",head_num);
    }

    //get_head_block_id 
    // @abi action
    void examgetid(){
        checksum160 block_hash;
        get_head_block_id(&block_hash);
        printhex(block_hash.hash,20);
    }

    //get_block_id_for_num 
    // @abi action
    void examidnum(uint64_t num){
        checksum160 block_hash;
        get_block_id_for_num(&block_hash,num);             
        printhex(block_hash.hash,20);
    }

    //get_head_block_time 
    // @abi action
    void examgettime(){
        int64_t head_time;
        head_time = get_head_block_time();
        print("head block time: ",head_time);
    }

    //get_trx_sender 
    // @abi action
    void examgettrx(){
        int64_t sender_id;
        sender_id = get_trx_sender();
        print("call action instance id: ",sender_id);
    }

    //get_account_id 
    // @abi action
    void examgetacid(std::string data){
        int64_t acc_id;
        acc_id = get_account_id(data.c_str(), data.length());
        print("account id: ",acc_id);
    }

    //get_account_name_by_id 
    // @abi action
    void examgetname(int64_t accid){
        char data[13]={0};
        int64_t result;
        result = get_account_name_by_id(data,13,accid);
        prints(data);
    }

    //get_asset_id 
    // @abi action
    void examassid(std::string data){
        int64_t assid;
        assid = get_asset_id(data.c_str(),data.length());
        print("asset id: ",assid);
    }

    //read_transaction
    // @abi action
    void examreadtrx(){
        int dwsize;
        dwsize =transaction_size();
        char* pBuffer = new char[dwsize];
        uint32_t size = read_transaction(pBuffer,dwsize);
        print("hex buffer: ");
        printhex(pBuffer,dwsize);
        delete[] pBuffer;
    }
    
    // @abi action
    void examtrxsize(){
        int dwsize;
        dwsize =transaction_size();
        print("the size of the serialize trx: ",dwsize);
    }

    //expiration 
    // @abi action
    void exampira(){
        uint64_t timenum = expiration();
        print("the expiration time: ", timenum);
    }

    //tapos_block_num 
    // @abi action
    void examtapnum(){
        uint64_t tapos_num;
        tapos_num = tapos_block_num();
        print("ref block num: ",tapos_num);
    }

    //tapos_block_prefix
    // @abi action
    void examtappre(){
        uint64_t tapos_prefix;
        tapos_prefix = tapos_block_prefix();
        print("ref block id: ",tapos_prefix);
    }

    //graphene_assert 
    // @abi action
    void examassert(){
        uint64_t number=1;
        graphene_assert(number == 1, "wrong!");
    }

    //graphene_assert_message 
    // @abi action
    void examassmsg(){
        uint64_t number=1;
        std::string msg = "wrong!!!";
        graphene_assert_message(number == 1, msg.c_str(),msg.length()); 
    }

    //print
    // @abi action
    void examprint(){
        print("example example example!!!");
    }
};

GRAPHENE_ABI(example,(examcurr)(examgetast)(examgetamo)(examdepo)(examwith)(examgetbl)(examsha25)(examsha512)(examripemd)(examverify)(examgetnum)(examgetid)(examidnum)(examgettime)(examgettrx)(examgetacid)(examgetname)(examassid)(examreadtrx)(examtrxsize)(exampira)(examtapnum)(examtappre)(examassert)(examassmsg)(examprint))
