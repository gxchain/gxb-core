#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/dispatcher.hpp>
#include <vector>

using namespace graphene;

class cross_chain : public contract
{
    public:
       cross_chain(uint64_t account_id)
                  :contract(account_id)
                  ,records_table(_self,_self)
                  ,Eth_confirm_table(_self,_self)
                  ,Eth_withdraw_table(_self,_self)
                  {}
    
    // @abi action
    // @abi payable
    void deposit(std::string target, std::string addr)
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        graphene_assert(asset_id == 1, "Only support GXC ");
        graphene_assert(asset_amount >= min_deposit, "Must greater than minnumber ");
        contract_asset amount{asset_amount, asset_id};
        uint64_t id_number = records_table.available_primary_key();
        auto coin_kind = find(cross_list.begin(),cross_list.end(),target);
        graphene_assert(coin_kind != cross_list.end(), "invalid chain name");
        uint64_t sender = get_trx_sender();
        records_table.emplace(sender,[&](auto &o){
            o.id = id_number;
            o.sender = sender;
            o.asset_id = asset_id;
            o.amount = asset_amount;
            o.coinkind = target;
            o.addr = addr;
            o.state = 0; 
        });
    }

    //@abi action 
    void withdraw(std::string to_account, contract_asset amount, std::string from_target, std::string txid, std::string from_account)
    {
        int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
        uint64_t sender = get_trx_sender();
        auto coin_kind = find(cross_list.begin(),cross_list.end(),from_target);
        graphene_assert(amount.asset_id == 1, "Only support GXC ");
        graphene_assert(amount.amount >= min_withdraw , "Must greater than minnumber ");
        graphene_assert(coin_kind != cross_list.end(), "invalid chain name");
        graphene_assert(sender == adminAccount, "You have no authority");
        graphene_assert(account_id >= 0, "invalid account_name to_account");
        graphene_assert(amount.amount > 0, "invalid amount");
        if(from_target == "ETH"){
            auto txid_uint = graphenelib::string_to_name(txid.c_str());
            auto txid_iterator = Eth_withdraw_table.find(txid_uint);
            graphene_assert(txid_iterator == Eth_withdraw_table.end(), "The txid is existed, be honest");
            auto id_number = Eth_withdraw_table.available_primary_key();
            Eth_withdraw_table.emplace(sender, [&](auto &o){
                o.txid =txid_uint;
                o.id = id_number;
            });
            auto begin_iterator = Eth_withdraw_table.begin();
            if(id_number - (*begin_iterator).id > 100){
                Eth_withdraw_table.erase(begin_iterator);
            }
            auto contract_id = current_receiver();
            auto contract_balance = get_balance(contract_id,amount.asset_id);
            graphene_assert( contract_balance > amount.amount, "balance not enough");
            withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
        }
    }

    //@abi action 
    void confirm(uint64_t order_id, std::string target, std::string addr, contract_asset amount, std::string txid)
    {
        uint64_t sender = get_trx_sender();
        graphene_assert(sender == adminAccount, "You have no authority");
        auto idx = records_table.find(order_id);
        graphene_assert(idx != records_table.end(), "There is no that order_id");
        graphene_assert((*idx).coinkind == target ,"Unmatched chain name");
        if(target == "ETH"){
            auto txid_uint = graphenelib::string_to_name(txid.c_str());
            auto txid_iterator = Eth_confirm_table.find(txid_uint);
            graphene_assert(txid_iterator == Eth_confirm_table.end(), "The txid is existed, be honest");
            auto id_number = Eth_confirm_table.available_primary_key();
            Eth_confirm_table.emplace(sender, [&](auto &o){
                o.txid = txid_uint;
                o.id = id_number;
            });
            auto begin_iterator = Eth_confirm_table.begin();
            if(id_number - (*begin_iterator).id > 100){
                Eth_confirm_table.erase(begin_iterator);
            }
            records_table.modify(idx, sender ,[&](auto& o){
            o.state =1;
            });
            records_table.erase(idx);
        }
    }

    //@abi action 
    void adjust(uint64_t deposit_min, uint64_t withdraw_min){
        uint64_t sender = get_trx_sender();
        graphene_assert(sender == adminAccount, "You have no authority");
        min_deposit = deposit_min;
        min_withdraw = withdraw_min;
    }

    private:
       const uint64_t adminAccount = 4707;
       std::vector<std::string> cross_list = {"ETH"};
       uint64_t min_deposit = 50000;
       uint64_t min_withdraw = 50000;
       //@abi table txidTable i64
       struct txidTable{
           uint64_t txid;
           uint64_t id;

           uint64_t primary_key() const {return txid;} 
           GRAPHENE_SERIALIZE(txidTable, (txid)(id))
       };
        typedef multi_index<N(txidTable), txidTable> txidTable_index;

       //@abi table record i64
       struct record{
           uint64_t id;
           uint64_t sender;
           uint64_t asset_id;
           int64_t amount;
           std::string coinkind;
           std::string addr;
           uint64_t state; 

           uint64_t primary_key() const { return id; }
           uint64_t by_sender() const { return sender; }

           GRAPHENE_SERIALIZE(record, (id)(sender)(asset_id)(amount)(coinkind)(addr)(state))
       };

        typedef multi_index<N(record), record,  
            indexed_by<N(sender), const_mem_fun<record, uint64_t, &record::by_sender>>> record_index; 

        record_index records_table; 
        txidTable_index Eth_confirm_table;
        txidTable_index Eth_withdraw_table;
};

GRAPHENE_ABI(cross_chain, (deposit)(withdraw)(confirm)(adjust))