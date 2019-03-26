#include <graphene/query_txid/query_txid_plugin.hpp>
#include <fc/io/fstream.hpp>
#include <graphene/chain/transaction_entry_object.hpp>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <fc/signals.hpp>

namespace graphene{ namespace query_txid{

namespace detail
{
    using namespace leveldb;
    class query_txid_plugin_impl
    {   
    public:
        query_txid_plugin_impl(query_txid_plugin& _plugin):_self(_plugin){}
        ~query_txid_plugin_impl(){
        }

        void collect_txid_index(const signed_block &b);

        graphene::chain::database& database()
        {
            return _self.database();
        }
        void init();
        static optional<trx_entry_object> query_trx_by_id(std::string txid);

    private:
        query_txid_plugin&                  _self;
        uint64_t                            limit_batch = 10;   //limit of leveldb batch

        fc::signal<void()>                  sig_db_write;
        fc::signal<void(const uint64_t)>    sig_remove;

        static DB*                          leveldb;
        std::string                         db_path = "/tmp/lvd.db";
        void consume_block();                                   //Consume block
        void remove_trx_index(const uint64_t trx_entry_id);     //Remove trx_index in db
    };
    DB* query_txid_plugin_impl::leveldb = nullptr;
    void query_txid_plugin_impl::init(){     
        //Create leveldb
        Options options;
        options.create_if_missing = true;
        Status s = DB::Open(options,db_path,&leveldb);

        // Respond to the sig_db_write signale
        sig_db_write.connect( [&](){ consume_block(); });
        sig_remove.connect( [&](const uint64_t trx_entry_id){ remove_trx_index(trx_entry_id); });
    }
    optional<trx_entry_object> query_txid_plugin_impl::query_trx_by_id(std::string txid)
    {
        std::string value;
        
        leveldb::Status s = leveldb->Get(leveldb::ReadOptions(), txid, &value);
        if(!s.ok()) return optional<trx_entry_object>();
        std::vector<char> data(value.begin(), value.end());
        auto result = fc::raw::unpack<trx_entry_object>(data);
        return result;
        /*
        leveldb::Iterator* it = leveldb->NewIterator(leveldb::ReadOptions());
        for (it->SeekToFirst(); it->Valid(); it->Next()) {

            trx_entry_object obj = fc::raw::unpack<trx_entry_object>(it->value().data(),it->value().size());
            //ilog("key:${key} -- value:${value}",("key",it->key().ToString())("value",it->value().ToString()));
        }
        assert(it->status().ok());  // Check for any errors found during the scan
        delete it;*/
        
    }
    void query_txid_plugin_impl::collect_txid_index(const signed_block &b){
        graphene::chain::database& db = database();
        for( auto idx = 0; idx < b.transactions.size(); idx++ ){
            db.create<trx_entry_object>( [&]( trx_entry_object& obj )
            {
                obj.txid          = b.transactions[idx].id();
                obj.block_num     = b.block_num();
                obj.trx_in_block  = idx;
            }); 
        }
        sig_db_write();         
    }
    void query_txid_plugin_impl::consume_block(){
        //ilog("consum consume consume consume");
        graphene::chain::database& db = database();
        const auto& dpo = db.get_dynamic_global_properties();
        uint64_t irr_num = dpo.last_irreversible_block_num;
        //判断是否满足写入到db的条件（配置文件中配置了写入的条数）
        const auto& trx_idx = db.get_index_type<trx_entry_index>().indices();
        const auto& trx_bn_idx = trx_idx.get<by_blocknum>();
        auto itor_begin = trx_idx.begin();
        if(itor_begin == trx_idx.end()) return;
        auto itor_end   = trx_bn_idx.lower_bound(irr_num);
        auto number = (--itor_end)->id.instance() - itor_begin->id.instance();
        //ilog("trx begin is, ${num}",("num",itor_begin->id.instance()));
        //ilog("trx end is, ${num}",("num",itor_end->id.instance()));
        //ilog("trx in db num is, ${num}",("num",number));
        auto backupnum = number;
        while(number > limit_batch){
            // 从队列中取出前n项元素，插入到leveldb中，使用同步的方式，原子操作，保证插入成功或者失败
            leveldb::WriteBatch batch;
            auto itor_backup = itor_begin;
            for(auto idx = 0; idx<limit_batch ; idx++){
                auto serialize = fc::raw::pack(*itor_begin);
                //ilog("buffer:${buffer}",("buffer",serialize.data()));
                std::string txid(itor_begin->txid);
                batch.Put(txid,{serialize.data(),serialize.size()});
                itor_begin++;
                if(itor_begin->id.instance() == itor_end->id.instance()) break;
            }
            leveldb::WriteOptions write_options;
            write_options.sync = true;
            Status s = leveldb->Write(write_options,&batch);
            if (!s.ok()){
                itor_begin = itor_backup;
                break;
            }
            number -= limit_batch;
        }
        if(backupnum > limit_batch)
            sig_remove(itor_begin->id.instance());
    }
    void query_txid_plugin_impl::remove_trx_index(const uint64_t trx_entry_id)
    {
        //ilog("remove remove remove remove ${num}",("num",trx_entry_id));
        graphene::chain::database &db = database();
        const auto &trx_idx = db.get_index_type<trx_entry_index>().indices();
        //ilog("remove,${trx_ent_id},bengin: ${begin},end: ${end}",("trx_ent_id",trx_entry_id)("begin",trx_idx.begin()->id.instance())("end",trx_idx.rbegin()->id.instance()));
        for (auto itor = trx_idx.begin(); itor != trx_idx.end();) {
            auto backup_itr = itor;
            ++itor;
            if (itor->id.instance() < trx_entry_id) {
                db.remove(*backup_itr);
            } else {
                break;
            }
        }
    }
}


// -----------------------------------query_txid_plugin --------------------------------------

query_txid_plugin::query_txid_plugin():
    my(new detail::query_txid_plugin_impl(*this)){
}

query_txid_plugin::~query_txid_plugin(){
    
}

std::string query_txid_plugin::plugin_name()const {
    return "query_txid";
}

void query_txid_plugin::plugin_set_program_options(
    boost::program_options::options_description& cli,
            boost::program_options::options_description& cfg){

}

void query_txid_plugin::plugin_initialize(const boost::program_options::variables_map& options){
    ilog("test leveldb plugin!!!!");
    // Add the index of the trx_entry_index object table to the database
    database().add_index< primary_index< trx_entry_index > >();
    // Respond to the apply_block signal
    database().applied_block.connect( [&]( const signed_block& b){ my->collect_txid_index(b); } );
    // Initialize the plugin instance
    my->init();
}

void query_txid_plugin::plugin_startup(){

}

optional<trx_entry_object> query_txid_plugin::query_trx_by_id(std::string txid){
    return detail::query_txid_plugin_impl::query_trx_by_id(txid);
}

}}