    /*
     * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
     *
     * The MIT License
     *
     * Permission is hereby granted, free of charge, to any person obtaining a copy
     * of this software and associated documentation files (the "Software"), to deal
     * in the Software without restriction, including without limitation the rights
     * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     * copies of the Software, and to permit persons to whom the Software is
     * furnished to do so, subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be included in
     * all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
     * THE SOFTWARE.
     */
    #include <algorithm>
    #include <cctype>
    #include <iomanip>
    #include <iostream>
    #include <iterator>
    #include <sstream>
    #include <string>
    #include <list>

    #include <boost/version.hpp>
    #include <boost/lexical_cast.hpp>
    #include <boost/algorithm/string/replace.hpp>

    #include <boost/range/adaptor/map.hpp>
    #include <boost/range/algorithm_ext/erase.hpp>
    #include <boost/range/algorithm/unique.hpp>
    #include <boost/range/algorithm/sort.hpp>

    #include <boost/multi_index_container.hpp>
    #include <boost/multi_index/ordered_index.hpp>
    #include <boost/multi_index/mem_fun.hpp>
    #include <boost/multi_index/member.hpp>
    #include <boost/multi_index/random_access_index.hpp>
    #include <boost/multi_index/tag.hpp>
    #include <boost/multi_index/sequenced_index.hpp>
    #include <boost/multi_index/hashed_index.hpp>

    #include <fc/git_revision.hpp>
    #include <fc/io/fstream.hpp>
    #include <fc/io/json.hpp>
    #include <fc/io/stdio.hpp>
    #include <fc/network/http/websocket.hpp>
    #include <fc/rpc/cli.hpp>
    #include <fc/rpc/websocket_api.hpp>
    #include <fc/crypto/aes.hpp>
    #include <fc/crypto/hex.hpp>
    #include <fc/thread/mutex.hpp>
    #include <fc/thread/scoped_lock.hpp>

    #include <graphene/app/api.hpp>
    #include <graphene/app/network_broadcast_api.hpp>
    #include <graphene/chain/asset_object.hpp>
    #include <graphene/chain/protocol/fee_schedule.hpp>
    #include <graphene/utilities/git_revision.hpp>
    #include <graphene/utilities/key_conversion.hpp>
    #include <graphene/utilities/words.hpp>
    #include <graphene/wallet/wallet.hpp>
    #include <graphene/wallet/api_documentation.hpp>
    #include <graphene/wallet/reflect_util.hpp>
    #include <graphene/debug_witness/debug_api.hpp>
    #include <graphene/chain/wast_to_wasm.hpp>
    #include <graphene/chain/abi_serializer.hpp>
    #include <graphene/chain/protocol/name.hpp>
    #include <fc/smart_ref_impl.hpp>

    #ifndef WIN32
    # include <sys/types.h>
    # include <sys/stat.h>
    #endif

    #define BRAIN_KEY_WORD_COUNT 16
    #define OWNER "owner"
    #define ACTIVE "active"
    #define ALL "all"

    namespace graphene { namespace wallet {

    namespace detail {

    struct operation_result_printer
    {
    public:
       operation_result_printer( const wallet_api_impl& w )
          : _wallet(w) {}
       const wallet_api_impl& _wallet;
       typedef std::string result_type;

       std::string operator()(const void_result& x) const;
       std::string operator()(const object_id_type& oid);
       std::string operator()(const asset& a);
       std::string operator()(const contract_receipt& r);
    };

    // BLOCK  TRX  OP  VOP
    struct operation_printer
    {
    private:
       std::ostream& out;
       const wallet_api_impl& wallet;
       operation_result result;

       std::string fee(const asset& a) const;

    public:
       operation_printer( std::ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
          : out(out),
            wallet(wallet),
            result(r)
       {}
       typedef std::string result_type;

       template<typename T>
       std::string operator()(const T& op)const;

       std::string operator()(const transfer_operation& op)const;
       std::string operator()(const transfer_from_blind_operation& op)const;
       std::string operator()(const transfer_to_blind_operation& op)const;
       std::string operator()(const account_create_operation& op)const;
       std::string operator()(const account_update_operation& op)const;
       std::string operator()(const asset_create_operation& op)const;
    };

    template<class T>
    optional<T> maybe_id( const string& name_or_id )
    {
       if( std::isdigit( name_or_id.front() ) )
       {
          try
          {
             return fc::variant(name_or_id).as<T>();
          }
          catch (const fc::exception&)
          {
          }
       }
       return optional<T>();
    }

    string address_to_shorthash( const address& addr )
    {
       uint32_t x = addr.addr._hash[0];
       static const char hd[] = "0123456789abcdef";
       string result;

       result += hd[(x >> 0x1c) & 0x0f];
       result += hd[(x >> 0x18) & 0x0f];
       result += hd[(x >> 0x14) & 0x0f];
       result += hd[(x >> 0x10) & 0x0f];
       result += hd[(x >> 0x0c) & 0x0f];
       result += hd[(x >> 0x08) & 0x0f];
       result += hd[(x >> 0x04) & 0x0f];
       result += hd[(x        ) & 0x0f];

       return result;
    }

    fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                             int sequence_number )
    {
       std::string sequence_string = std::to_string(sequence_number);
       fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
       fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
       return derived_key;
    }

    string normalize_brain_key( string s )
    {
       size_t i = 0, n = s.length();
       std::string result;
       char c;
       result.reserve( n );

       bool preceded_by_whitespace = false;
       bool non_empty = false;
       while( i < n )
       {
          c = s[i++];
          switch( c )
          {
          case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
             preceded_by_whitespace = true;
             continue;

          case 'a': c = 'A'; break;
          case 'b': c = 'B'; break;
          case 'c': c = 'C'; break;
          case 'd': c = 'D'; break;
          case 'e': c = 'E'; break;
          case 'f': c = 'F'; break;
          case 'g': c = 'G'; break;
          case 'h': c = 'H'; break;
          case 'i': c = 'I'; break;
          case 'j': c = 'J'; break;
          case 'k': c = 'K'; break;
          case 'l': c = 'L'; break;
          case 'm': c = 'M'; break;
          case 'n': c = 'N'; break;
          case 'o': c = 'O'; break;
          case 'p': c = 'P'; break;
          case 'q': c = 'Q'; break;
          case 'r': c = 'R'; break;
          case 's': c = 'S'; break;
          case 't': c = 'T'; break;
          case 'u': c = 'U'; break;
          case 'v': c = 'V'; break;
          case 'w': c = 'W'; break;
          case 'x': c = 'X'; break;
          case 'y': c = 'Y'; break;
          case 'z': c = 'Z'; break;

          default:
             break;
          }
          if( preceded_by_whitespace && non_empty )
             result.push_back(' ');
          result.push_back(c);
          preceded_by_whitespace = false;
          non_empty = true;
       }
       return result;
    }

    struct op_prototype_visitor
    {
       typedef void result_type;

       int t = 0;
       flat_map< std::string, operation >& name2op;

       op_prototype_visitor(
          int _t,
          flat_map< std::string, operation >& _prototype_ops
          ):t(_t), name2op(_prototype_ops) {}

       template<typename Type>
       result_type operator()( const Type& op )const
       {
          string name = fc::get_typename<Type>::name();
          size_t p = name.rfind(':');
          if( p != string::npos )
             name = name.substr( p+1 );
          name2op[ name ] = Type();
       }
    };

    class wallet_api_impl
    {
    public:
       api_documentation method_documentation;
    private:
       void claim_registered_account(const account_object& account)
       {
          auto it = _wallet.pending_account_registrations.find( account.name );
          FC_ASSERT( it != _wallet.pending_account_registrations.end() );
          for (const std::string& wif_key : it->second)
             if( !import_key( account.name, wif_key ) )
             {
                // somebody else beat our pending registration, there is
                //    nothing we can do except log it and move on
                elog( "account ${name} registered by someone else first!",
                      ("name", account.name) );
                // might as well remove it from pending regs,
                //    because there is now no way this registration
                //    can become valid (even in the extremely rare
                //    possibility of migrating to a fork where the
                //    name is available, the user can always
                //    manually re-register)
             }
          _wallet.pending_account_registrations.erase( it );
       }

       // after a witness registration succeeds, this saves the private key in the wallet permanently
       //
       void claim_registered_witness(const std::string& witness_name)
       {
          auto iter = _wallet.pending_witness_registrations.find(witness_name);
          FC_ASSERT(iter != _wallet.pending_witness_registrations.end());
          std::string wif_key = iter->second;

          // get the list key id this key is registered with in the chain
          fc::optional<fc::ecc::private_key> witness_private_key = wif_to_key(wif_key);
          FC_ASSERT(witness_private_key);

          auto pub_key = witness_private_key->get_public_key();
          _keys[pub_key] = wif_key;
          _wallet.pending_witness_registrations.erase(iter);
       }

       fc::mutex _resync_mutex;
       void resync()
       {
          fc::scoped_lock<fc::mutex> lock(_resync_mutex);
          // this method is used to update wallet_data annotations
          //   e.g. wallet has been restarted and was not notified
          //   of events while it was down
          //
          // everything that is done "incremental style" when a push
          //   notification is received, should also be done here
          //   "batch style" by querying the blockchain

          if( !_wallet.pending_account_registrations.empty() )
          {
             // make a vector of the account names pending registration
             std::vector<string> pending_account_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_account_registrations));

             // look those up on the blockchain
             std::vector<fc::optional<graphene::chain::account_object >>
                   pending_account_objects = _remote_db->lookup_account_names( pending_account_names );

             // if any of them exist, claim them
             for( const fc::optional<graphene::chain::account_object>& optional_account : pending_account_objects )
                if( optional_account )
                   claim_registered_account(*optional_account);
          }

          if (!_wallet.pending_witness_registrations.empty())
          {
             // make a vector of the owner accounts for witnesses pending registration
             std::vector<string> pending_witness_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_witness_registrations));

             // look up the owners on the blockchain
             std::vector<fc::optional<graphene::chain::account_object>> owner_account_objects = _remote_db->lookup_account_names(pending_witness_names);

             // if any of them have registered witnesses, claim them
             for( const fc::optional<graphene::chain::account_object>& optional_account : owner_account_objects )
                if (optional_account)
                {
                   fc::optional<witness_object> witness_obj = _remote_db->get_witness_by_account(optional_account->id);
                   if (witness_obj)
                      claim_registered_witness(optional_account->name);
                }
          }
       }

       void enable_umask_protection()
       {
    #ifdef __unix__
          _old_umask = umask( S_IRWXG | S_IRWXO );
    #endif
       }

       void disable_umask_protection()
       {
    #ifdef __unix__
          umask( _old_umask );
    #endif
       }

       void init_prototype_ops()
       {
          operation op;
          for( int t=0; t<op.count(); t++ )
          {
             op.set_which( t );
             op.visit( op_prototype_visitor(t, _prototype_ops) );
          }
          return;
       }

       map<transaction_handle_type, signed_transaction> _builder_transactions;

       // if the user executes the same command twice in quick succession,
       // we might generate the same transaction id, and cause the second
       // transaction to be rejected.  This can be avoided by altering the
       // second transaction slightly (bumping up the expiration time by
       // a second).  Keep track of recent transaction ids we've generated
       // so we can know if we need to do this
       struct recently_generated_transaction_record
       {
          fc::time_point_sec generation_time;
          graphene::chain::transaction_id_type transaction_id;
       };
       struct timestamp_index{};
       typedef boost::multi_index_container<recently_generated_transaction_record,
                                            boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<recently_generated_transaction_record,
                                                                                                                                        graphene::chain::transaction_id_type,
                                                                                                                                        &recently_generated_transaction_record::transaction_id>,
                                                                                                             std::hash<graphene::chain::transaction_id_type> >,
                                                                           boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                                  boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time> > > > recently_generated_transaction_set_type;
       recently_generated_transaction_set_type _recently_generated_transactions;

    public:
       wallet_api& self;
       wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
          : self(s),
            _chain_id(initial_data.chain_id),
            _remote_api(rapi),
            _remote_db(rapi->database()),
            _remote_net_broadcast(rapi->network_broadcast()),
            _remote_hist(rapi->history())
       {
          chain_id_type remote_chain_id = _remote_db->get_chain_id();
          if( remote_chain_id != _chain_id )
          {
             FC_THROW( "Remote server gave us an unexpected chain_id",
                ("remote_chain_id", remote_chain_id)
                ("chain_id", _chain_id) );
          }
          init_prototype_ops();

          _remote_db->set_block_applied_callback( [this](const variant& block_id )
          {
             on_block_applied( block_id );
          } );

          _wallet.chain_id = _chain_id;
          _wallet.ws_server = initial_data.ws_server;
          _wallet.ws_user = initial_data.ws_user;
          _wallet.ws_password = initial_data.ws_password;
       }
       virtual ~wallet_api_impl()
       {
          try
          {
             _remote_db->cancel_all_subscriptions();
          }
          catch (const fc::exception& e)
          {
             // Right now the wallet_api has no way of knowing if the connection to the
             // witness has already disconnected (via the witness node exiting first).
             // If it has exited, cancel_all_subscriptsions() will throw and there's
             // nothing we can do about it.
             // dlog("Caught exception ${e} while canceling database subscriptions", ("e", e));
          }
       }

       void encrypt_keys()
       {
          if( !is_locked() )
          {
             plain_keys data;
             data.keys = _keys;
             data.checksum = _checksum;
             auto plain_txt = fc::raw::pack(data);
             _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
          }
       }

       void on_block_applied( const variant& block_id )
       {
          fc::async([this]{resync();}, "Resync after block");
       }

       bool copy_wallet_file( string destination_filename )
       {
          fc::path src_path = get_wallet_filename();
          if( !fc::exists( src_path ) )
             return false;
          fc::path dest_path = destination_filename + _wallet_filename_extension;
          int suffix = 0;
          while( fc::exists(dest_path) )
          {
             ++suffix;
             dest_path = destination_filename + "-" + fc::to_string( suffix ) + _wallet_filename_extension;
          }
          wlog( "backing up wallet ${src} to ${dest}",
                ("src", src_path)
                ("dest", dest_path) );

          fc::path dest_parent = fc::absolute(dest_path).parent_path();
          try
          {
             enable_umask_protection();
             if( !fc::exists( dest_parent ) )
                fc::create_directories( dest_parent );
             fc::copy( src_path, dest_path );
             disable_umask_protection();
          }
          catch(...)
          {
             disable_umask_protection();
             throw;
          }
          return true;
       }

       bool is_locked()const
       {
          return _checksum == fc::sha512();
       }

       template<typename T>
       T get_object(object_id<T::space_id, T::type_id, T> id)const
       {
          auto ob = _remote_db->get_objects({id}).front();
          return ob.template as<T>();
       }

       /**
        * @brief set_operation_fees
        * @param tx
        * @param s
        */
       void set_operation_fees(signed_transaction& tx, const fee_schedule& s, fc::optional<asset_object> fee_asset = fc::optional<asset_object>())
       {
           for( auto& op : tx.operations )  {
               if (fee_asset.valid()) {
                   s.set_fee(op, fee_asset->options.core_exchange_rate);
               }
               else {
                   s.set_fee(op);
               }
           }
       }

       variant info() const
       {
          auto chain_props = get_chain_properties();
          auto global_props = get_global_properties();
          auto dynamic_props = get_dynamic_global_properties();
          fc::mutable_variant_object result;
          result["head_block_num"] = dynamic_props.head_block_number;
          result["head_block_id"] = dynamic_props.head_block_id;
          result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                              time_point_sec(time_point::now()),
                                                                              " old");
          result["next_maintenance_time"] = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
          result["chain_id"] = chain_props.chain_id;
          result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
          result["active_witnesses"] = global_props.active_witnesses;
          result["active_committee_members"] = global_props.active_committee_members;
          return result;
       }

       variant_object about() const
       {
          string client_version( graphene::utilities::git_revision_description );
          const size_t pos = client_version.find( '/' );
          if( pos != string::npos && client_version.size() > pos )
             client_version = client_version.substr( pos + 1 );

          fc::mutable_variant_object result;
          //result["blockchain_name"]        = BLOCKCHAIN_NAME;
          //result["blockchain_description"] = BTS_BLOCKCHAIN_DESCRIPTION;
          result["client_version"]           = client_version;
          result["graphene_revision"]        = graphene::utilities::git_revision_sha;
          result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
          result["fc_revision"]              = fc::git_revision_sha;
          result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
          result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
          result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
          result["openssl_version"]          = OPENSSL_VERSION_TEXT;

          std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
    #if defined(__APPLE__)
          std::string os = "osx";
    #elif defined(__linux__)
          std::string os = "linux";
    #elif defined(_MSC_VER)
          std::string os = "win32";
    #else
          std::string os = "other";
    #endif
          result["build"] = os + " " + bitness;

          return result;
       }

       chain_property_object get_chain_properties() const
       {
          return _remote_db->get_chain_properties();
       }
       global_property_object get_global_properties() const
       {
          return _remote_db->get_global_properties();
       }

       data_transaction_commission_percent_t get_commission_percent() const
       {
         return _remote_db->get_commission_percent();
       }

       dynamic_global_property_object get_dynamic_global_properties() const
       {
          return _remote_db->get_dynamic_global_properties();
       }
       account_object get_account(account_id_type id) const
       {
          if( _wallet.my_accounts.get<by_id>().count(id) )
             return *_wallet.my_accounts.get<by_id>().find(id);
          auto rec = _remote_db->get_accounts({id}).front();
          FC_ASSERT(rec);
          return *rec;
       }

       account_object get_account(string account_name_or_id) const
       {
           FC_ASSERT(account_name_or_id.size() > 0);

           if (auto id = maybe_id<account_id_type>(account_name_or_id)) {
               // It's an ID
               return get_account(*id);
           } else {
               auto rec = _remote_db->lookup_account_names({account_name_or_id}).front();
               FC_ASSERT(rec && rec->name == account_name_or_id);
               return *rec;
           }
       }

       account_id_type get_account_id(string account_name_or_id) const
       {
          return get_account(account_name_or_id).get_id();
       }
       optional<asset_object> find_asset(asset_id_type id)const
       {
          auto rec = _remote_db->get_assets({id}).front();
          return rec;
       }
       optional<asset_object> find_asset(string asset_symbol_or_id)const
       {
           FC_ASSERT(asset_symbol_or_id.size() > 0);

           if (auto id = maybe_id<asset_id_type>(asset_symbol_or_id)) {
               // It's an ID
               return find_asset(*id);
           } else {
               // It's a symbol
               auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
               if (!rec) return optional<asset_object>();
               if (rec->symbol != asset_symbol_or_id) {
                   return optional<asset_object>();
               }
               return rec;
           }
       }
       asset_object get_asset(asset_id_type id)const
       {
          auto opt = find_asset(id);
          FC_ASSERT(opt);
          return *opt;
       }
       asset_object get_asset(string asset_symbol_or_id)const
       {
          auto opt = find_asset(asset_symbol_or_id);
          FC_ASSERT(opt);
          return *opt;
       }

       asset_id_type get_asset_id(string asset_symbol_or_id) const
       {
          FC_ASSERT( asset_symbol_or_id.size() > 0 );
          vector<optional<asset_object>> opt_asset;
          if( std::isdigit( asset_symbol_or_id.front() ) )
             return fc::variant(asset_symbol_or_id).as<asset_id_type>();
          opt_asset = _remote_db->lookup_asset_symbols( {asset_symbol_or_id} );
          FC_ASSERT( (opt_asset.size() > 0) && (opt_asset[0].valid()) );
          return opt_asset[0]->id;
       }

       string                            get_wallet_filename() const
       {
          return _wallet_filename;
       }

       fc::ecc::private_key              get_private_key(const public_key_type& id)const
       {
          auto it = _keys.find(id);
          FC_ASSERT( it != _keys.end() );

          fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
          FC_ASSERT( privkey );
          return *privkey;
       }

       fc::ecc::private_key get_private_key_for_account(const account_object& account)const
       {
          vector<public_key_type> active_keys = account.active.get_keys();
          if (active_keys.size() != 1)
             FC_THROW("Expecting a simple authority with one active key");
          return get_private_key(active_keys.front());
       }

       // imports the private key into the wallet, and associate it in some way (?) with the
       // given account name.
       // @returns true if the key matches a current active/owner/memo key for the named
       //          account, false otherwise (but it is stored either way)
       bool import_key(string account_name_or_id, string wif_key)
       {
          fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
          if (!optional_private_key)
             FC_THROW("Invalid private key");
          graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

          account_object account = get_account( account_name_or_id );

          // make a list of all current public keys for the named account
          flat_set<public_key_type> all_keys_for_account;
          std::vector<public_key_type> active_keys = account.active.get_keys();
          std::vector<public_key_type> owner_keys = account.owner.get_keys();
          std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
          std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
          all_keys_for_account.insert(account.options.memo_key);

          _keys[wif_pub_key] = wif_key;

          _wallet.update_account(account);

          _wallet.extra_keys[account.id].insert(wif_pub_key);

          return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
       }

       vector< signed_transaction > import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast );

       bool load_wallet_file(string wallet_filename = "")
       {
          // TODO:  Merge imported wallet with existing wallet,
          //        instead of replacing it
          if( wallet_filename == "" )
             wallet_filename = _wallet_filename;

          if( ! fc::exists( wallet_filename ) )
             return false;

          _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >();
          if( _wallet.chain_id != _chain_id )
             FC_THROW( "Wallet chain ID does not match",
                ("wallet.chain_id", _wallet.chain_id)
                ("chain_id", _chain_id) );

          size_t account_pagination = 100;
          vector< account_id_type > account_ids_to_send;
          size_t n = _wallet.my_accounts.size();
          account_ids_to_send.reserve( std::min( account_pagination, n ) );
          auto it = _wallet.my_accounts.begin();

          for( size_t start=0; start<n; start+=account_pagination )
          {
             size_t end = std::min( start+account_pagination, n );
             assert( end > start );
             account_ids_to_send.clear();
             std::vector< account_object > old_accounts;
             for( size_t i=start; i<end; i++ )
             {
                assert( it != _wallet.my_accounts.end() );
                old_accounts.push_back( *it );
                account_ids_to_send.push_back( old_accounts.back().id );
                ++it;
             }
             std::vector< optional< account_object > > accounts = _remote_db->get_accounts(account_ids_to_send);
             // server response should be same length as request
             FC_ASSERT( accounts.size() == account_ids_to_send.size() );
             size_t i = 0;
             for( const optional< account_object >& acct : accounts )
             {
                account_object& old_acct = old_accounts[i];
                if( !acct.valid() )
                {
                   elog( "Could not find account ${id} : \"${name}\" does not exist on the chain!", ("id", old_acct.id)("name", old_acct.name) );
                   i++;
                   continue;
                }
                // this check makes sure the server didn't send results
                // in a different order, or accounts we didn't request
                FC_ASSERT( acct->id == old_acct.id );
                if( fc::json::to_string(*acct) != fc::json::to_string(old_acct) )
                {
                   wlog( "Account ${id} : \"${name}\" updated on chain", ("id", acct->id)("name", acct->name) );
                }
                _wallet.update_account( *acct );
                i++;
             }
          }

          return true;
       }
       void save_wallet_file(string wallet_filename = "")
       {
          //
          // Serialize in memory, then save to disk
          //
          // This approach lessens the risk of a partially written wallet
          // if exceptions are thrown in serialization
          //

          encrypt_keys();

          if( wallet_filename == "" )
             wallet_filename = _wallet_filename;

          wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

          string data = fc::json::to_pretty_string( _wallet );
          try
          {
             enable_umask_protection();
             //
             // Parentheses on the following declaration fails to compile,
             // due to the Most Vexing Parse.  Thanks, C++
             //
             // http://en.wikipedia.org/wiki/Most_vexing_parse
             //
             fc::ofstream outfile{ fc::path( wallet_filename ) };
             outfile.write( data.c_str(), data.length() );
             outfile.flush();
             outfile.close();
             disable_umask_protection();
          }
          catch(...)
          {
             disable_umask_protection();
             throw;
          }
       }

       transaction_handle_type begin_builder_transaction()
       {
          int trx_handle = _builder_transactions.empty()? 0
                                                        : (--_builder_transactions.end())->first + 1;
          _builder_transactions[trx_handle];
          return trx_handle;
       }
       void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
       {
          FC_ASSERT(_builder_transactions.count(transaction_handle));
          _builder_transactions[transaction_handle].operations.emplace_back(op);
       }
       void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                     uint32_t operation_index,
                                                     const operation& new_op)
       {
          FC_ASSERT(_builder_transactions.count(handle));
          signed_transaction& trx = _builder_transactions[handle];
          FC_ASSERT( operation_index < trx.operations.size());
          trx.operations[operation_index] = new_op;
       }
       asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL)
       {
          FC_ASSERT(_builder_transactions.count(handle));

          auto fee_asset_obj = get_asset(fee_asset);
          asset total_fee = fee_asset_obj.amount(0);

          auto gprops = _remote_db->get_global_properties().parameters;
          if( fee_asset_obj.get_id() != asset_id_type() )
          {
             for( auto& op : _builder_transactions[handle].operations )
                total_fee += gprops.current_fees->set_fee( op, fee_asset_obj.options.core_exchange_rate );

             FC_ASSERT((total_fee * fee_asset_obj.options.core_exchange_rate).amount <=
                       get_object<asset_dynamic_data_object>(fee_asset_obj.dynamic_asset_data_id).fee_pool,
                       "Cannot pay fees in ${asset}, as this asset's fee pool is insufficiently funded.",
                       ("asset", fee_asset_obj.symbol));
          } else {
             for( auto& op : _builder_transactions[handle].operations )
                total_fee += gprops.current_fees->set_fee( op );
          }

          return total_fee;
       }
       transaction preview_builder_transaction(transaction_handle_type handle)
       {
          FC_ASSERT(_builder_transactions.count(handle));
          return _builder_transactions[handle];
       }
       signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true)
       {
          FC_ASSERT(_builder_transactions.count(transaction_handle));

          return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
       }

       pair<transaction_id_type,signed_transaction> broadcast_transaction(signed_transaction tx)
       {
           try {
               _remote_net_broadcast->broadcast_transaction(tx);
           }
           catch (const fc::exception& e) {
               elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()));
               throw;
           }
           return std::make_pair(tx.id(),tx);
       }

       signed_transaction propose_builder_transaction(
          transaction_handle_type handle,
          time_point_sec expiration = time_point::now() + fc::minutes(1),
          uint32_t review_period_seconds = 0, bool broadcast = true)
       {
          FC_ASSERT(_builder_transactions.count(handle));
          proposal_create_operation op;
          op.expiration_time = expiration;
          signed_transaction& trx = _builder_transactions[handle];
          std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                         [](const operation& op) -> op_wrapper { return op; });
          if( review_period_seconds )
             op.review_period_seconds = review_period_seconds;
          trx.operations = {op};
          _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

          return trx = sign_transaction(trx, broadcast);
       }

       signed_transaction propose_builder_transaction2(
          transaction_handle_type handle,
          string account_name_or_id,
          time_point_sec expiration = time_point::now() + fc::minutes(1),
          uint32_t review_period_seconds = 0, bool broadcast = true)
       {
          FC_ASSERT(_builder_transactions.count(handle));
          proposal_create_operation op;
          op.fee_paying_account = get_account(account_name_or_id).get_id();
          op.expiration_time = expiration;
          signed_transaction& trx = _builder_transactions[handle];
          std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                         [](const operation& op) -> op_wrapper { return op; });
          if( review_period_seconds )
             op.review_period_seconds = review_period_seconds;
          trx.operations = {op};
          _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

          return trx = sign_transaction(trx, broadcast);
       }

       void remove_builder_transaction(transaction_handle_type handle)
       {
          _builder_transactions.erase(handle);
       }

       variants get_table_objects(string contract, string table)
       { try {
             account_object contract_obj = get_account(contract);

             const auto& tables = contract_obj.abi.tables;
             auto iter = std::find_if(tables.begin(), tables.end(),
                     [&](const table_def& t) { return t.name == table; });

             if (iter != tables.end()) {
                 return _remote_db->get_table_objects(contract_obj.id.number, contract_obj.id.number, name(table));
             } else {
                 GRAPHENE_ASSERT(false, table_not_found_exception, "No table found for ${contract}", ("contract", contract));
             }
             return variants();
       } FC_CAPTURE_AND_RETHROW((contract)(table)) }

       variant get_contract_tables(string contract)
       { try {
             account_object contract_obj = get_account(contract);

             fc::variants result;
             const auto &tables = contract_obj.abi.tables;
             result.reserve(tables.size());

             std::transform(tables.begin(), tables.end(), std::back_inserter(result),
                     [](const table_def &t_def) -> fc::variant {
                         return name(t_def.name).to_string();
                     });

             return result;
       } FC_CAPTURE_AND_RETHROW((contract)) }

       signed_transaction deploy_contract(string name,
                                          string account,
                                          string vm_type,
                                          string vm_version,
                                          string contract_dir,
                                          string fee_asset_symbol,
                                          bool broadcast = false)
       { try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_name(name));
           fc::optional<asset_object> fee_asset_obj = get_asset(fee_asset_symbol);

           std::vector<uint8_t> wasm;
           variant abi_def_data;

           auto load_contract = [&]() {
               fc::path cpath(contract_dir);
               if (cpath.filename().generic_string() == ".") cpath = cpath.parent_path();

               fc::path wast_path = cpath / (cpath.filename().generic_string() + ".wast");
               fc::path wasm_path = cpath / (cpath.filename().generic_string() + ".wasm");
               fc::path abi_path = cpath / (cpath.filename().generic_string() + ".abi");

               bool wast_exist = fc::exists(wast_path);
               bool wasm_exist = fc::exists(wasm_path);
               bool abi_exist = fc::exists(abi_path);

               FC_ASSERT(abi_exist && (wast_exist || wasm_exist), "need abi and wast/wasm file");

               abi_def_data = fc::json::from_file(abi_path);

               std::string wast;
               std::string wasm_string;
               fc::read_file_contents(wasm_path, wasm_string);
               const string binary_wasm_header("\x00\x61\x73\x6d", 4);
               if (wasm_string.compare(0, 4, binary_wasm_header) == 0) {
                   for (auto it = wasm_string.begin(); it != wasm_string.end(); ++it) { //TODO
                       wasm.push_back(*it);
                   }
               } else {
                   fc::read_file_contents(wast_path, wast);
                   FC_ASSERT(!wast.empty(), "wasm and wast file both invalid");
                   wasm = graphene::chain::wast_to_wasm(wast);
               }
           };

           load_contract();

           account_object creator_account_object = this->get_account(account);
           account_id_type creator_account_id = creator_account_object.id;

           contract_deploy_operation op;
           op.name = name;
           op.account = creator_account_id;
           op.vm_type = vm_type;
           op.vm_version = vm_version;
           op.code = bytes(wasm.begin(), wasm.end());
           op.abi = abi_def_data.as<abi_def>();

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (name)(account)(vm_type)(vm_version)(contract_dir)(fee_asset_symbol)(broadcast)) }

       signed_transaction call_contract(string account,
                                        string contract,
                                        optional<asset> amount,
                                        string method,
                                        string args,
                                        string fee_asset_symbol,
                                        bool broadcast = false)
       { try {
             FC_ASSERT(!self.is_locked());
             fc::optional<asset_object> fee_asset_obj = get_asset(fee_asset_symbol);

             account_object caller = get_account(account);
             account_object contract_obj = get_account(contract);

             contract_call_operation contract_call_op;
             contract_call_op.account = caller.id;
             contract_call_op.contract_id = contract_obj.id;
             if (amount.valid()) {
                 contract_call_op.amount = amount;
             }
             contract_call_op.method_name = string_to_name(method.c_str());
             fc::variant action_args_var = fc::json::from_string(args, fc::json::relaxed_parser);

             abi_serializer abis(contract_obj.abi, fc::milliseconds(1000000));
             auto action_type = abis.get_action_type(method);
             GRAPHENE_ASSERT(!action_type.empty(), action_validate_exception, "Unknown action ${action} in contract ${contract}", ("action", method)("contract", contract));
             const fc::time_point deadline = fc::time_point::now() + fc::milliseconds(1000000);
             contract_call_op.data = abis.variant_to_binary(action_type, action_args_var, 0, deadline, fc::milliseconds(1000000));

             signed_transaction tx;
             tx.operations.push_back(contract_call_op);
             set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
             tx.validate();

             return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (account)(contract)(amount)(method)(args)(fee_asset_symbol)(broadcast)) }

       signed_transaction register_account(string name,
                                           public_key_type owner,
                                           public_key_type active,
                                           string  registrar_account,
                                           string  referrer_account,
                                           uint32_t referrer_percent,
                                           bool broadcast = false)
       { try {
          FC_ASSERT( !self.is_locked() );
          FC_ASSERT( is_valid_name(name) );
          account_create_operation account_create_op;

          // #449 referrer_percent is on 0-100 scale, if user has larger
          // number it means their script is using GRAPHENE_100_PERCENT scale
          // instead of 0-100 scale.
          FC_ASSERT( referrer_percent <= 100 );
          // TODO:  process when pay_from_account is ID

          account_object registrar_account_object =
                this->get_account( registrar_account );
          FC_ASSERT( registrar_account_object.is_lifetime_member() );

          account_id_type registrar_account_id = registrar_account_object.id;

          account_object referrer_account_object =
                this->get_account( referrer_account );
          account_create_op.referrer = referrer_account_object.id;
          account_create_op.referrer_percent = uint16_t( referrer_percent * GRAPHENE_1_PERCENT );

          account_create_op.registrar = registrar_account_id;
          account_create_op.name = name;
          account_create_op.owner = authority(1, owner, 1);
          account_create_op.active = authority(1, active, 1);
          account_create_op.options.memo_key = active;

          signed_transaction tx;

          tx.operations.push_back( account_create_op );

          auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
          set_operation_fees( tx, current_fees );

          vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

          auto dyn_props = get_dynamic_global_properties();
          tx.set_reference_block( dyn_props.head_block_id );
          tx.set_expiration( dyn_props.time + fc::seconds(30) );
          tx.validate();

          for( public_key_type& key : paying_keys )
          {
             auto it = _keys.find(key);
             if( it != _keys.end() )
             {
                fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
                if( !privkey.valid() )
                {
                   FC_ASSERT( false, "Malformed private key in _keys" );
                }
                tx.sign( *privkey, _chain_id );
             }
          }

          if( broadcast )
             _remote_net_broadcast->broadcast_transaction( tx );
          return tx;
       } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(registrar_account)(referrer_account)(referrer_percent)(broadcast) ) }


       signed_transaction register_account2(string name,
                                           public_key_type owner,
                                           public_key_type active,
                                           public_key_type memo,
                                           string  registrar_account,
                                           string  referrer_account,
                                           uint32_t referrer_percent,
                                           string asset_symbol,
                                           bool broadcast = false)
       { try {
          FC_ASSERT( !self.is_locked() );
          FC_ASSERT( is_valid_name(name) );
          account_create_operation account_create_op;

          // #449 referrer_percent is on 0-100 scale, if user has larger
          // number it means their script is using GRAPHENE_100_PERCENT scale
          // instead of 0-100 scale.
          FC_ASSERT( referrer_percent <= 100 );
          // TODO:  process when pay_from_account is ID

          account_object registrar_account_object =
                this->get_account( registrar_account );
          FC_ASSERT( registrar_account_object.is_lifetime_member() );

          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

          account_id_type registrar_account_id = registrar_account_object.id;

          account_object referrer_account_object =
                this->get_account( referrer_account );
          account_create_op.referrer = referrer_account_object.id;
          account_create_op.referrer_percent = uint16_t( referrer_percent * GRAPHENE_1_PERCENT );

          account_create_op.registrar = registrar_account_id;
          account_create_op.name = name;
          account_create_op.owner = authority(1, owner, 1);
          account_create_op.active = authority(1, active, 1);
          account_create_op.options.memo_key = memo;

          signed_transaction tx;

          tx.operations.push_back( account_create_op );

          auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
          set_operation_fees( tx, current_fees, asset_obj );

          vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

          auto dyn_props = get_dynamic_global_properties();
          tx.set_reference_block( dyn_props.head_block_id );
          tx.set_expiration( dyn_props.time + fc::seconds(30) );
          tx.validate();

          for( public_key_type& key : paying_keys )
          {
             auto it = _keys.find(key);
             if( it != _keys.end() )
             {
                fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
                if( !privkey.valid() )
                {
                   FC_ASSERT( false, "Malformed private key in _keys" );
                }
                tx.sign( *privkey, _chain_id );
             }
          }

          if( broadcast )
             _remote_net_broadcast->broadcast_transaction( tx );
          return tx;
       } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(registrar_account)(referrer_account)(referrer_percent)(broadcast) ) }


       signed_transaction upgrade_account(string name, string asset_symbol, bool broadcast)
       { try {
          FC_ASSERT( !self.is_locked() );
          account_object account_obj = get_account(name);
          FC_ASSERT( !account_obj.is_lifetime_member() );

          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

          signed_transaction tx;
          account_upgrade_operation op;
          op.account_to_upgrade = account_obj.get_id();
          op.upgrade_to_lifetime_member = true;
          tx.operations = {op};
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (name) ) }

       signed_transaction upgrade_data_transaction_member(string name, string auth_referrer, bool broadcast)
       { try {
          FC_ASSERT( !self.is_locked() );
          account_object account_obj = get_account(name);
          const account_object auth_referrer_account = get_account(auth_referrer);
          FC_ASSERT(!account_obj.is_data_transaction_member());

          account_upgrade_data_transaction_member_operation op;
          op.account_to_upgrade = account_obj.get_id();
          op.upgrade_to_data_transaction_member = true;

          const chain_parameters& current_params = get_global_properties().parameters;
          proposal_create_operation prop_op;
          prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
          prop_op.fee_paying_account = auth_referrer_account.get_id();
          prop_op.proposed_ops.emplace_back(op);
          current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

          signed_transaction tx;
          tx.operations.push_back(prop_op);
          set_operation_fees(tx, current_params.current_fees);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (name) ) }

       signed_transaction upgrade_account_merchant(string name,string auth_referrer, bool broadcast)
       { try {
          FC_ASSERT( !self.is_locked() );
          account_object account_obj = get_account(name);
          const account_object auth_referrer_account = get_account(auth_referrer);
          FC_ASSERT( !account_obj.is_merchant_member() );

          account_upgrade_merchant_operation op;
          op.account_to_upgrade = account_obj.get_id();
          op.upgrade_to_merchant_member = true;
          op.auth_referrer = auth_referrer_account.get_id();

          operation_ext_version_t ext;
          ext.version = operation_version_one;
          op.extensions.insert(ext);

          const chain_parameters& current_params = get_global_properties().parameters;
          proposal_create_operation prop_op;
          prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
          prop_op.fee_paying_account = auth_referrer_account.get_id();
          prop_op.proposed_ops.emplace_back(op);
          current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

          signed_transaction tx;
          tx.operations.push_back(prop_op);
          set_operation_fees(tx, current_params.current_fees);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (name) ) }

       signed_transaction second_hand_datasource_copyright_clear(const string& proposing_account, const string& datasource, object_id_type product_id, bool broadcast)
       {
           FC_ASSERT(!self.is_locked());
           account_object datasource_account_obj = get_account(datasource);
           FC_ASSERT(datasource_account_obj.is_datasource_member());

           datasource_copyright_clear_operation op;
           op.datasource_account = datasource_account_obj.id;
           op.product_id = product_id;

           const chain_parameters& current_params = get_global_properties().parameters;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
           prop_op.fee_paying_account = get_account(proposing_account).id;
           prop_op.proposed_ops.emplace_back(op);
           current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }


       signed_transaction upgrade_account_datasource(string name,string auth_referrer, bool broadcast)
       { try {
          FC_ASSERT( !self.is_locked() );
          account_object account_obj = get_account(name);
          const account_object auth_referrer_account = get_account(auth_referrer);
          FC_ASSERT( account_obj.is_merchant_member() );
          FC_ASSERT( !account_obj.is_datasource_member() );

          account_upgrade_datasource_operation op;
          op.account_to_upgrade = account_obj.get_id();
          op.upgrade_to_datasource_member = true;
          op.auth_referrer = auth_referrer_account.get_id();

          operation_ext_version_t ext;
          ext.version = operation_version_one;
          op.extensions.insert(ext);

          const chain_parameters& current_params = get_global_properties().parameters;
          proposal_create_operation prop_op;
          prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
          prop_op.fee_paying_account = auth_referrer_account.get_id();
          prop_op.proposed_ops.emplace_back(op);
          current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

          signed_transaction tx;
          tx.operations.push_back(prop_op);
          set_operation_fees(tx, current_params.current_fees);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (name) ) }

       // This function generates derived keys starting with index 0 and keeps incrementing
       // the index until it finds a key that isn't registered in the block chain.  To be
       // safer, it continues checking for a few more keys to make sure there wasn't a short gap
       // caused by a failed registration or the like.
       int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
       {
          int first_unused_index = 0;
          int number_of_consecutive_unused_keys = 0;
          for (int key_index = 0; ; ++key_index)
          {
             fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
             graphene::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
             if( _keys.find(derived_public_key) == _keys.end() )
             {
                if (number_of_consecutive_unused_keys)
                {
                   ++number_of_consecutive_unused_keys;
                   if (number_of_consecutive_unused_keys > 5)
                      return first_unused_index;
                }
                else
                {
                   first_unused_index = key_index;
                   number_of_consecutive_unused_keys = 1;
                }
             }
             else
             {
                // key_index is used
                first_unused_index = 0;
                number_of_consecutive_unused_keys = 0;
             }
          }
       }

       signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                          string account_name,
                                                          string registrar_account,
                                                          string referrer_account,
                                                          bool broadcast = false,
                                                          bool save_wallet = true)
       { try {
             int active_key_index = find_first_unused_derived_key_index(owner_privkey);
             fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

             int memo_key_index = find_first_unused_derived_key_index(active_privkey);
             fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

             graphene::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
             graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
             graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

             account_create_operation account_create_op;

             // TODO:  process when pay_from_account is ID

             account_object registrar_account_object = get_account( registrar_account );

             account_id_type registrar_account_id = registrar_account_object.id;

             account_object referrer_account_object = get_account( referrer_account );
             account_create_op.referrer = referrer_account_object.id;
             account_create_op.referrer_percent = referrer_account_object.referrer_rewards_percentage;

             account_create_op.registrar = registrar_account_id;
             account_create_op.name = account_name;
             account_create_op.owner = authority(1, owner_pubkey, 1);
             account_create_op.active = authority(1, active_pubkey, 1);
             account_create_op.options.memo_key = memo_pubkey;

             // current_fee_schedule()
             // find_account(pay_from_account)

             // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

             signed_transaction tx;

             tx.operations.push_back( account_create_op );

             set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

             vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

             auto dyn_props = get_dynamic_global_properties();
             tx.set_reference_block( dyn_props.head_block_id );
             tx.set_expiration( dyn_props.time + fc::seconds(30) );
             tx.validate();

             for( public_key_type& key : paying_keys )
             {
                auto it = _keys.find(key);
                if( it != _keys.end() )
                {
                   fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
                   FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
                   tx.sign( *privkey, _chain_id );
                }
             }

             // we do not insert owner_privkey here because
             //    it is intended to only be used for key recovery
             _wallet.pending_account_registrations[account_name].push_back(key_to_wif( active_privkey ));
             _wallet.pending_account_registrations[account_name].push_back(key_to_wif( memo_privkey ));
             if( save_wallet )
                save_wallet_file();
             if( broadcast )
                _remote_net_broadcast->broadcast_transaction( tx );
             return tx;
       } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account)(broadcast) ) }

       signed_transaction create_account_with_brain_key(string brain_key,
                                                        string account_name,
                                                        string registrar_account,
                                                        string referrer_account,
                                                        bool broadcast = false,
                                                        bool save_wallet = true)
       { try {
          FC_ASSERT( !self.is_locked() );
          string normalized_brain_key = normalize_brain_key( brain_key );
          // TODO:  scan blockchain for accounts that exist with same brain key
          fc::ecc::private_key owner_privkey = derive_private_key( normalized_brain_key, 0 );
          return create_account_with_private_key(owner_privkey, account_name, registrar_account, referrer_account, broadcast, save_wallet);
       } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account) ) }


       signed_transaction create_data_market_category(string category_name,uint8_t data_market_type,uint32_t order_num,string issuer,bool broadcast /* = false */){

           FC_ASSERT( !self.is_locked() );
           const account_object issuer_account = get_account( issuer );
           FC_ASSERT(category_name!="", "category_name cannot be empty!");
           FC_ASSERT(data_market_type==1 || data_market_type==2, "data_market_type must 1 or 2");

           data_market_category_create_operation create_op;
           create_op.issuer = issuer_account.id;
           create_op.category_name = category_name;
           create_op.data_market_type =  data_market_type;
           create_op.order_num = order_num;
           create_op.create_date_time = fc::time_point_sec(fc::time_point::now());
           signed_transaction tx;
           tx.operations.push_back( create_op );
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

           tx.validate();

           return sign_transaction( tx, broadcast );
       }

       signed_transaction propose_data_market_category_update(const string &proposing_account,string category_id,string new_category_name,uint32_t new_order_num,uint8_t new_status,bool broadcast)
       {
           FC_ASSERT( !self.is_locked() );
           fc::optional<data_market_category_id_type> data_market_category_id = maybe_id<data_market_category_id_type>(category_id);
           FC_ASSERT(0!= (&(*data_market_category_id)));
           FC_ASSERT(new_category_name!="", "category_name cannot be empty!");
           FC_ASSERT((0==new_status || 1==new_status ), "new_status cannot out of {0,1}");

           data_market_category_update_operation update_op;
           update_op.new_category_name = new_category_name;
           update_op.new_order_num =  new_order_num;
           update_op.data_market_category = *data_market_category_id;
           update_op.new_status = new_status;

           const chain_parameters& current_params = get_global_properties().parameters;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
           prop_op.fee_paying_account = get_account(proposing_account).id;
           prop_op.proposed_ops.emplace_back( update_op );
           current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction create_free_data_product(string product_name,string brief_desc,string datasource_account,string category_id,double price,string icon, vector<schema_context_object> schema_contexts,string parent_id,string issuer,bool recommend,bool broadcast){
           FC_ASSERT( !self.is_locked() );
           account_object issuer_account = get_account( issuer );
           account_object datasource_account_object = get_account( datasource_account );
           data_market_category_object  data_market_category = this->get_data_market_category(category_id);
           FC_ASSERT(product_name!="", "product_name cannot be empty!");

           free_data_product_create_operation create_op;
           create_op.product_name =  product_name;
           create_op.brief_desc = brief_desc;
           create_op.datasource = datasource_account_object.id;
           create_op.category_id = data_market_category.id;
           create_op.price = price * GRAPHENE_BLOCKCHAIN_PRECISION;
           create_op.icon = icon;
           create_op.schema_contexts = schema_contexts;
           create_op.create_date_time = fc::time_point_sec(fc::time_point::now());
           if(recommend){
               create_op.recommend_expiration_date_time = time_point_sec::maximum();
           }else{
               create_op.recommend_expiration_date_time = time_point_sec::min();
           }
           if(0!= (&parent_id) && !parent_id.empty()){
               free_data_product_object parent_free_data_product =  this->get_free_data_product(parent_id);
               if(0!= (&parent_free_data_product)){
                   create_op.parent_id = parent_id;
               }
           }

           create_op.issuer = issuer_account.id;
           signed_transaction tx;
           tx.operations.push_back( create_op );
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction( tx, broadcast );

       }
       signed_transaction propose_free_data_product_update(const string &proposing_account,string free_data_product_id,string new_product_name,string new_brief_desc,string new_datasource_account,string new_category_id,double new_price,string new_icon,vector<schema_context_object> new_schema_contexts,string new_parent_id,uint8_t new_status,bool new_recommend,bool broadcast)
       {
           FC_ASSERT( !self.is_locked() );

           free_data_product_object free_data_product =  this->get_free_data_product(free_data_product_id);
           account_object new_datasource_account_object = get_account(new_datasource_account);
           data_market_category_object  data_market_category = this->get_data_market_category(new_category_id);

           FC_ASSERT(new_product_name!="", "new_product_name cannot be empty!");
           FC_ASSERT((0==new_status || 1==new_status || 2==new_status), "new_status cannot out of {0,1,2}");

           // get original schema_contexts
           vector<schema_context_object> schema_contexts = free_data_product.schema_contexts;
           vector<schema_context_object> contexts_tmp;
           for (auto &item : new_schema_contexts) {
               if (find(schema_contexts.begin(), schema_contexts.end(), item) == schema_contexts.end()) {
                   contexts_tmp.push_back(item);
               }
           }
           // append new nonduplicate schema_context
           schema_contexts.insert(schema_contexts.end(), contexts_tmp.begin(), contexts_tmp.end());

           free_data_product_update_operation update_op;
           update_op.free_data_product = free_data_product.id;
           update_op.new_product_name = new_product_name;
           update_op.new_brief_desc = new_brief_desc;
           update_op.new_datasource = new_datasource_account_object.id;
           update_op.new_category_id = data_market_category.id;
           update_op.new_price = new_price * GRAPHENE_BLOCKCHAIN_PRECISION;
           update_op.new_icon = new_icon;
           update_op.new_schema_contexts = schema_contexts;
           if(0!= (&new_parent_id) && !new_parent_id.empty()){
               free_data_product_object parent_free_data_product =  this->get_free_data_product(new_parent_id);
               if(0!= (&parent_free_data_product)){
                   update_op.new_parent_id = new_parent_id;// setString
               }
           }
           update_op.new_status = new_status;
           if(new_recommend){
               update_op.new_recommend_expiration_date_time = time_point_sec::maximum();
           }else{
               update_op.new_recommend_expiration_date_time = time_point_sec::min();
           }

           const chain_parameters& current_params = get_global_properties().parameters;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
           // prop_op.review_period_seconds = current_params.committee_proposal_review_period;
           prop_op.fee_paying_account = get_account(proposing_account).id;
           prop_op.proposed_ops.emplace_back( update_op );
           current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction create_league_data_product(string product_name,string brief_desc,string category_id,double refer_price,string icon,vector<schema_context_object> schema_contexts,string issuer, uint64_t pocs_threshold, bool broadcast){

           FC_ASSERT( !self.is_locked() );
           account_object issuer_account = get_account( issuer );
           data_market_category_object  data_market_category = this->get_data_market_category(category_id);
           FC_ASSERT(product_name!="", "product_name cannot be empty!");

           league_data_product_create_operation create_op;
           create_op.product_name =  product_name;
           create_op.brief_desc = brief_desc;
           create_op.category_id = data_market_category.id;
           create_op.refer_price = refer_price * GRAPHENE_BLOCKCHAIN_PRECISION;
           create_op.icon = icon;
           create_op.schema_contexts = schema_contexts;
           create_op.issuer = issuer_account.id;
           create_op.create_date_time = fc::time_point_sec(fc::time_point::now());
           pocs_threshold_league_data_product_t ext;
           ext.pocs_threshold = pocs_threshold;
           create_op.extensions.insert(ext);

           signed_transaction tx;
           tx.operations.push_back( create_op );
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction( tx, broadcast );

       }

       signed_transaction propose_league_data_product_update(const string &proposing_account,string league_data_product_id,string new_product_name,string new_brief_desc,string new_category_id,double new_refer_price,string new_icon, vector<schema_context_object> new_schema_contexts,uint8_t new_status,uint64_t new_pocs_threshold,bool broadcast)
       {
           FC_ASSERT( !self.is_locked() );

           league_data_product_object league_data_product =  this->get_league_data_product(league_data_product_id);
           data_market_category_object  data_market_category = this->get_data_market_category(new_category_id);

           FC_ASSERT(new_product_name!="", "new_product_name cannot be empty!");
           FC_ASSERT((0==new_status || 1==new_status || 2==new_status), "new_status cannot out of {0,1,2}");

           // get original schema_contexts
           vector<schema_context_object> schema_contexts = league_data_product.schema_contexts;
           vector<schema_context_object> contexts_tmp;
           for (auto &item : new_schema_contexts) {
               if (find(schema_contexts.begin(), schema_contexts.end(), item) == schema_contexts.end()) {
                   contexts_tmp.push_back(item);
               }
           }
           // append new nonduplicate schema_context
           schema_contexts.insert(schema_contexts.end(), contexts_tmp.begin(), contexts_tmp.end());

           league_data_product_update_operation update_op;
           update_op.league_data_product = league_data_product.id;
           update_op.new_product_name = new_product_name;
           update_op.new_brief_desc = new_brief_desc;
           update_op.new_category_id = data_market_category.id;
           update_op.new_refer_price = new_refer_price * GRAPHENE_BLOCKCHAIN_PRECISION;
           update_op.new_icon = new_icon;
           update_op.new_schema_contexts = schema_contexts;
           update_op.new_status = new_status;
           pocs_threshold_league_data_product_t ext;
           ext.pocs_threshold = new_pocs_threshold;
           update_op.extensions.insert(ext);

           const chain_parameters& current_params = get_global_properties().parameters;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
           prop_op.fee_paying_account = get_account(proposing_account).id;
           prop_op.proposed_ops.emplace_back( update_op );
           current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction create_league(string league_name, string brief_desc, vector<account_id_type> members, vector <league_data_product_id_type> data_products, vector <uint64_t> prices,string category_id, string icon, string issuer, vector<uint64_t> pocs_thresholds, vector<uint64_t> fee_bases, vector<uint64_t> pocs_weights, bool recommend, bool broadcast) {
           FC_ASSERT( !self.is_locked() );

           account_object issuer_account = get_account( issuer );
           data_market_category_object  data_market_category = this->get_data_market_category(category_id);
           FC_ASSERT(league_name!="", "league_name cannot be empty!");
           FC_ASSERT(data_products.size() == prices.size(), "data_products size must equal to prices size");

           for (auto &price : prices) {
               price = price * GRAPHENE_BLOCKCHAIN_PRECISION;
           }

           league_create_operation create_op;
           create_op.league_name =  league_name;
           create_op.brief_desc = brief_desc;
           create_op.data_products = data_products;
           create_op.prices = prices;
           create_op.category_id = data_market_category.id;
           create_op.icon = icon;
           create_op.issuer = issuer_account.id;
           create_op.create_date_time = fc::time_point_sec(fc::time_point::now());
           if(recommend){
               create_op.recommend_expiration_date_time = time_point_sec::maximum();
           }else{
               create_op.recommend_expiration_date_time = time_point_sec::min();
           }
           create_op.members = members;
           pocs_threshold_league_t ext;
           ext.pocs_thresholds = pocs_thresholds;
           ext.fee_bases = fee_bases;
           ext.product_pocs_weights = pocs_weights;
           create_op.extensions.insert(ext);

           signed_transaction tx;
           tx.operations.push_back( create_op );
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction( tx, broadcast );
       }

       signed_transaction data_transaction_create(string request_id, object_id_type product_id, fc::optional<league_id_type> league_id, string version, string params, string requester, bool broadcast) {
           FC_ASSERT(!self.is_locked());

           account_object requester_account = get_account(requester);
           FC_ASSERT(0!= (&requester_account));

           data_transaction_create_operation create_op;
           create_op.request_id = request_id;
           create_op.product_id = product_id;
           if (league_id.valid()) {
               create_op.league_id = league_id;
           }
           else {
               create_op.league_id = fc::optional<league_id_type>();
           }
           create_op.version = version;
           create_op.params = params;
           create_op.requester = requester_account.id;
           create_op.create_date_time = fc::time_point_sec(fc::time_point::now());

           signed_transaction tx;
           tx.operations.push_back(create_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction(tx, broadcast);
       }

       signed_transaction pay_data_transaction(string from, string to, asset amount, string request_id, bool broadcast)
       {
           FC_ASSERT( !self.is_locked() );
           account_object from_account = get_account(from);
           account_object to_account = get_account(to);
           account_id_type from_id = from_account.id;
           account_id_type to_id = get_account_id(to);

           pay_data_transaction_operation pay_op;
           pay_op.from = from_id;
           pay_op.to = to_id;
           pay_op.amount = amount;
           pay_op.request_id = request_id;

           signed_transaction tx;
           tx.operations.push_back(pay_op);
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction data_transaction_datasource_validate_error(string request_id, string datasource, bool broadcast)
       {
           FC_ASSERT(!self.is_locked());
           account_object datasource_account = get_account(datasource);

           FC_ASSERT(0 != (&datasource_account));

           data_transaction_datasource_validate_error_operation op;
           op.request_id = request_id;
           op.datasource = datasource_account.id;

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction(tx, broadcast);
       }

       signed_transaction data_transaction_complain_datasource(string request_id, account_id_type requester, account_id_type datasource,
        string merchant_copyright_hash, string datasource_copyright_hash, bool broadcast)
       {
           FC_ASSERT(!self.is_locked());
           account_object requester_account = get_account(requester);
           account_object datasource_account = get_account(datasource);
           FC_ASSERT(0 != (&requester_account));
           FC_ASSERT(0 != (&datasource_account));

           data_transaction_complain_operation complain_op;
           complain_op.request_id = request_id;
           complain_op.requester = requester;
           complain_op.datasource = datasource;
           complain_op.merchant_copyright_hash = merchant_copyright_hash;
           complain_op.datasource_copyright_hash = datasource_copyright_hash;
           complain_op.create_date_time = fc::time_point_sec(fc::time_point::now());

           signed_transaction tx;
           tx.operations.push_back(complain_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction(tx, broadcast);
       }

       signed_transaction data_transaction_datasource_upload(string request_id, string requester, string datasource, fc::optional<string> copyright_hash, bool broadcast)
       {
           FC_ASSERT(!self.is_locked());
           account_object requester_account = get_account(requester);
           account_object datasource_account = get_account(datasource);

           FC_ASSERT(0 != (&requester_account));
           FC_ASSERT(0 != (&datasource_account));

           data_transaction_datasource_upload_operation update_op;
           update_op.request_id = request_id;
           update_op.datasource = datasource_account.id;
           update_op.requester = requester_account.id;

           operation_ext_copyright_hash_t ext;
           if (copyright_hash.valid()) {
               ext.copyright_hash = copyright_hash;
               update_op.extensions.insert(ext);
           }

           signed_transaction tx;
           tx.operations.push_back(update_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction(tx, broadcast);
       }

       signed_transaction data_transaction_update(string request_id, uint8_t new_status, string new_requester, fc::optional<string> memo, bool broadcast) {
           FC_ASSERT(!self.is_locked());

           account_object requester_account = get_account(new_requester);

           FC_ASSERT(0 != (&requester_account));
           FC_ASSERT(((data_transaction_status_confirmed == new_status) || (data_transaction_status_user_rejected == new_status)), "new_status cannot out of {1,99}");

           data_transaction_update_operation update_op;
           update_op.new_status = new_status;
           update_op.request_id = request_id;
           update_op.new_requester = requester_account.id;
           if (memo.valid()) {
               update_op.new_memo = memo;
           }
           else {
               update_op.new_memo = fc::optional<string>();
           }

           signed_transaction tx;
           tx.operations.push_back(update_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
           tx.validate();
           return sign_transaction(tx, broadcast);
       }

       data_market_category_object get_data_market_category(string category_id){
           fc::optional<data_market_category_id_type> data_market_category_id = maybe_id<data_market_category_id_type>(category_id);
           if (data_market_category_id)
           {
              std::vector<data_market_category_id_type> ids_to_get;
              ids_to_get.push_back(*data_market_category_id);
              std::vector<fc::optional<data_market_category_object>> data_market_category_objects = _remote_db->get_data_market_categories(ids_to_get);
              if (data_market_category_objects.front())
                 return *data_market_category_objects.front();
              FC_THROW("No data_market_category is registered for id ${id}", ("id", category_id));
           }else{
               FC_THROW("No data_market_category is registered for id ${id}", ("id", category_id));
           }
       }

       free_data_product_object get_free_data_product(string product_id){
           fc::optional<free_data_product_id_type> free_data_product_id = maybe_id<free_data_product_id_type>(product_id);
           if (free_data_product_id)
           {
              std::vector<free_data_product_id_type> ids_to_get;
              ids_to_get.push_back(*free_data_product_id);
              std::vector<fc::optional<free_data_product_object>> free_data_product_objects = _remote_db->get_free_data_products(ids_to_get);
              if (free_data_product_objects.front())
                 return *free_data_product_objects.front();
              FC_THROW("No free_data_product is registered for id ${id}", ("id", product_id));
           }else{
               FC_THROW("No free_data_product is registered for id ${id}", ("id", product_id));
           }
       }

       league_data_product_object get_league_data_product(string product_id){
           fc::optional<league_data_product_id_type> league_data_product_id = maybe_id<league_data_product_id_type>(product_id);
           if (league_data_product_id)
           {
              std::vector<league_data_product_id_type> ids_to_get;
              ids_to_get.push_back(*league_data_product_id);
              std::vector<fc::optional<league_data_product_object>> league_data_product_objects = _remote_db->get_league_data_products(ids_to_get);
              if (league_data_product_objects.front())
                 return *league_data_product_objects.front();
              FC_THROW("No league_data_product is registered for id ${id}", ("id", product_id));
           }else{
               FC_THROW("No league_data_product is registered for id ${id}", ("id", product_id));
           }
        }

       league_object get_league(string league_id) {
           fc::optional<league_id_type> league_data_id = maybe_id<league_id_type>(league_id);
           if (league_data_id)
           {
              std::vector<league_id_type> ids_to_get;
              ids_to_get.push_back(*league_data_id);
              std::vector<fc::optional<league_object>> league_objects = _remote_db->get_leagues(ids_to_get);
              if (league_objects.front())
                 return *league_objects.front();
              FC_THROW("No league is registered for id ${id}", ("id", league_id));
           }else{
               FC_THROW("No league is registered for id ${id}", ("id", league_id));
           }
       }

       vector<data_market_category_object> list_data_market_categories(uint32_t market_type){

           return  _remote_db->list_data_market_categories(market_type);
       }
       free_data_product_search_results_object list_free_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const{

           return _remote_db->list_free_data_products(data_market_category_id,offset,limit,order_by,keyword,show_all);
       }
       league_data_product_search_results_object list_league_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const{

           return _remote_db->list_league_data_products(data_market_category_id,offset,limit,order_by,keyword,show_all);
       }
       league_search_results_object list_leagues(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const {
           return _remote_db->list_leagues(data_market_category_id,offset,limit,order_by,keyword,show_all);
       }

       data_transaction_search_results_object list_data_transactions_by_requester(string requester, uint32_t limit) const {
           return _remote_db->list_data_transactions_by_requester(requester, limit);
       }

       map<account_id_type, uint64_t> list_second_hand_datasources(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint32_t limit) const {
           return _remote_db->list_second_hand_datasources(start_date_time, end_date_time, limit);
       }

       uint32_t list_total_second_hand_transaction_counts_by_datasource(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, const string& datasource_account) const{
           fc::optional<account_id_type> acct_id = maybe_id<account_id_type>(datasource_account);
           if (!acct_id) acct_id = get_account(datasource_account).id;
           return _remote_db->list_total_second_hand_transaction_counts_by_datasource(start_date_time, end_date_time, *acct_id);
       }

       optional<data_transaction_object> get_data_transaction_by_request_id(string request_id) const {
           return _remote_db->get_data_transaction_by_request_id(request_id);
       }

       void get_tps()
       {
           const uint16_t delta_block = 5;
           while (true) {
               uint32_t head_block_num = get_dynamic_global_properties().head_block_number;
               // get total_trx
               uint32_t total_trx = 0;
               time_point_sec start_time, end_time;
               for (int i = 0; i < delta_block; ++i) {
                   auto block = _remote_db->get_block(head_block_num - i);
                   total_trx += block->transactions.size();
                   if (0 == i) {
                       end_time = block->timestamp;
                   }
                   if (delta_block - 1 == i){
                       start_time = block->timestamp;
                   }
               }

               // tps = total_trx / delta_time
               auto tps = total_trx / (end_time - start_time).to_seconds();
               ilog("tps: ${tps},  time: ${t}", ("t", end_time)("tps", tps));

               // sleep interval
               uint8_t block_interval = get_global_properties().parameters.block_interval;
               fc::usleep(fc::seconds(block_interval));
           }
       }

       signed_transaction create_asset(string issuer,
                                       string symbol,
                                       uint8_t precision,
                                       asset_options common,
                                       string fee_asset_symbol,
                                       bool broadcast = false)
       { try {
          account_object issuer_account = get_account( issuer );
          FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

          fc::optional<asset_object> fee_asset_obj = get_asset(fee_asset_symbol);
          FC_ASSERT(fee_asset_obj, "Could not find asset matching ${asset}", ("asset", fee_asset_symbol));

          asset_create_operation create_op;
          create_op.issuer = issuer_account.id;
          create_op.symbol = symbol;
          create_op.precision = precision;
          create_op.common_options = common;

          signed_transaction tx;
          tx.operations.push_back(create_op);
          set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((issuer)(symbol)(precision)(common)(fee_asset_symbol)(broadcast)) }

       signed_transaction update_asset(string symbol,
                                       optional<string> new_issuer,
                                       asset_options new_options,
                                       string fee_asset_symbol,
                                       bool broadcast /* = false */)
       { try {
          fc::optional<asset_object> asset_to_update = find_asset(symbol);
          FC_ASSERT(asset_to_update, "No asset with that symbol exists!");

          fc::optional<asset_object> fee_asset_obj = find_asset(fee_asset_symbol);
          FC_ASSERT(fee_asset_obj, "Could not find asset matching ${asset}", ("asset", fee_asset_symbol));

          optional<account_id_type> new_issuer_account_id;
          if (new_issuer) {
            account_object new_issuer_account = get_account(*new_issuer);
            new_issuer_account_id = new_issuer_account.id;
          }

          asset_update_operation update_op;
          update_op.issuer = asset_to_update->issuer;
          if (asset_to_update->issuer == GRAPHENE_NULL_ACCOUNT && asset_to_update->get_id() == asset_id_type()) {
              update_op.issuer = account_id_type();
          }
          update_op.asset_to_update = asset_to_update->id;
          update_op.new_issuer = new_issuer_account_id;
          update_op.new_options = new_options;

          signed_transaction tx;
          tx.operations.push_back(update_op);
          set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((symbol)(new_issuer)(new_options)(fee_asset_symbol)(broadcast)) }

       signed_transaction update_bitasset(string symbol,
                                          bitasset_options new_options,
                                          bool broadcast /* = false */)
       { try {
          optional<asset_object> asset_to_update = find_asset(symbol);
          if (!asset_to_update)
            FC_THROW("No asset with that symbol exists!");

          asset_update_bitasset_operation update_op;
          update_op.issuer = asset_to_update->issuer;
          update_op.asset_to_update = asset_to_update->id;
          update_op.new_options = new_options;

          signed_transaction tx;
          tx.operations.push_back( update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (symbol)(new_options)(broadcast) ) }

       signed_transaction update_asset_feed_producers(string symbol,
                                                      flat_set<string> new_feed_producers,
                                                      bool broadcast /* = false */)
       { try {
          optional<asset_object> asset_to_update = find_asset(symbol);
          if (!asset_to_update)
            FC_THROW("No asset with that symbol exists!");

          asset_update_feed_producers_operation update_op;
          update_op.issuer = asset_to_update->issuer;
          update_op.asset_to_update = asset_to_update->id;
          update_op.new_feed_producers.reserve(new_feed_producers.size());
          std::transform(new_feed_producers.begin(), new_feed_producers.end(),
                         std::inserter(update_op.new_feed_producers, update_op.new_feed_producers.end()),
                         [this](const std::string& account_name_or_id){ return get_account_id(account_name_or_id); });

          signed_transaction tx;
          tx.operations.push_back( update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (symbol)(new_feed_producers)(broadcast) ) }

       signed_transaction publish_asset_feed(string publishing_account,
                                             string symbol,
                                             price_feed feed,
                                             bool broadcast /* = false */)
       { try {
          optional<asset_object> asset_to_update = find_asset(symbol);
          if (!asset_to_update)
            FC_THROW("No asset with that symbol exists!");

          asset_publish_feed_operation publish_op;
          publish_op.publisher = get_account_id(publishing_account);
          publish_op.asset_id = asset_to_update->id;
          publish_op.feed = feed;

          signed_transaction tx;
          tx.operations.push_back( publish_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (publishing_account)(symbol)(feed)(broadcast) ) }

       signed_transaction fund_asset_fee_pool(string from,
                                              string symbol,
                                              string amount,
                                              bool broadcast /* = false */)
       { try {
          account_object from_account = get_account(from);
          optional<asset_object> asset_to_fund = find_asset(symbol);
          if (!asset_to_fund)
            FC_THROW("No asset with that symbol exists!");
          asset_object core_asset = get_asset(asset_id_type());

          asset_fund_fee_pool_operation fund_op;
          fund_op.from_account = from_account.id;
          fund_op.asset_id = asset_to_fund->id;
          fund_op.amount = core_asset.amount_from_string(amount).amount;

          signed_transaction tx;
          tx.operations.push_back( fund_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (from)(symbol)(amount)(broadcast) ) }

       signed_transaction reserve_asset(string from,
                                     string amount,
                                     string symbol,
                                     bool broadcast /* = false */)
       { try {
          account_object from_account = get_account(from);
          optional<asset_object> asset_to_reserve = find_asset(symbol);
          if (!asset_to_reserve)
            FC_THROW("No asset with that symbol exists!");

          asset_reserve_operation reserve_op;
          reserve_op.payer = from_account.id;
          reserve_op.amount_to_reserve = asset_to_reserve->amount_from_string(amount);

          signed_transaction tx;
          tx.operations.push_back( reserve_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(broadcast) ) }

       signed_transaction global_settle_asset(string symbol,
                                              price settle_price,
                                              bool broadcast /* = false */)
       { try {
          optional<asset_object> asset_to_settle = find_asset(symbol);
          if (!asset_to_settle)
            FC_THROW("No asset with that symbol exists!");

          asset_global_settle_operation settle_op;
          settle_op.issuer = asset_to_settle->issuer;
          settle_op.asset_to_settle = asset_to_settle->id;
          settle_op.settle_price = settle_price;

          signed_transaction tx;
          tx.operations.push_back( settle_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (symbol)(settle_price)(broadcast) ) }

       signed_transaction settle_asset(string account_to_settle,
                                       string amount_to_settle,
                                       string symbol,
                                       bool broadcast /* = false */)
       { try {
          optional<asset_object> asset_to_settle = find_asset(symbol);
          if (!asset_to_settle)
            FC_THROW("No asset with that symbol exists!");

          asset_settle_operation settle_op;
          settle_op.account = get_account_id(account_to_settle);
          settle_op.amount = asset_to_settle->amount_from_string(amount_to_settle);

          signed_transaction tx;
          tx.operations.push_back( settle_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (account_to_settle)(amount_to_settle)(symbol)(broadcast) ) }

       signed_transaction whitelist_account(string authorizing_account,
                                            string account_to_list,
                                            account_whitelist_operation::account_listing new_listing_status,
                                            bool broadcast /* = false */)
       { try {
          account_whitelist_operation whitelist_op;
          whitelist_op.authorizing_account = get_account_id(authorizing_account);
          whitelist_op.account_to_list = get_account_id(account_to_list);
          whitelist_op.new_listing = new_listing_status;

          signed_transaction tx;
          tx.operations.push_back( whitelist_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (authorizing_account)(account_to_list)(new_listing_status)(broadcast) ) }

       signed_transaction create_committee_member(string owner_account, string url,
                                          bool broadcast /* = false */)
       { try {

          committee_member_create_operation committee_member_create_op;
          committee_member_create_op.committee_member_account = get_account_id(owner_account);
          committee_member_create_op.url = url;
          if (_remote_db->get_committee_member_by_account(committee_member_create_op.committee_member_account))
             FC_THROW("Account ${owner_account} is already a committee_member", ("owner_account", owner_account));

          signed_transaction tx;
          tx.operations.push_back( committee_member_create_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

       witness_object get_witness(string owner_account)
       {
          try
          {
             fc::optional<witness_id_type> witness_id = maybe_id<witness_id_type>(owner_account);
             if (witness_id)
             {
                std::vector<witness_id_type> ids_to_get;
                ids_to_get.push_back(*witness_id);
                std::vector<fc::optional<witness_object>> witness_objects = _remote_db->get_witnesses(ids_to_get);
                if (witness_objects.front())
                   return *witness_objects.front();
                FC_THROW("No witness is registered for id ${id}", ("id", owner_account));
             }
             else
             {
                // then maybe it's the owner account
                try
                {
                   account_id_type owner_account_id = get_account_id(owner_account);
                   fc::optional<witness_object> witness = _remote_db->get_witness_by_account(owner_account_id);
                   if (witness)
                      return *witness;
                   else
                      FC_THROW("No witness is registered for account ${account}", ("account", owner_account));
                }
                catch (const fc::exception&)
                {
                   FC_THROW("No account or witness named ${account}", ("account", owner_account));
                }
             }
          }
          FC_CAPTURE_AND_RETHROW( (owner_account) )
       }

       committee_member_object get_committee_member(string owner_account)
       {
          try
          {
             fc::optional<committee_member_id_type> committee_member_id = maybe_id<committee_member_id_type>(owner_account);
             if (committee_member_id)
             {
                std::vector<committee_member_id_type> ids_to_get;
                ids_to_get.push_back(*committee_member_id);
                std::vector<fc::optional<committee_member_object>> committee_member_objects = _remote_db->get_committee_members(ids_to_get);
                if (committee_member_objects.front())
                   return *committee_member_objects.front();
                FC_THROW("No committee_member is registered for id ${id}", ("id", owner_account));
             }
             else
             {
                // then maybe it's the owner account
                try
                {
                   account_id_type owner_account_id = get_account_id(owner_account);
                   fc::optional<committee_member_object> committee_member = _remote_db->get_committee_member_by_account(owner_account_id);
                   if (committee_member)
                      return *committee_member;
                   else
                      FC_THROW("No committee_member is registered for account ${account}", ("account", owner_account));
                }
                catch (const fc::exception&)
                {
                   FC_THROW("No account or committee_member named ${account}", ("account", owner_account));
                }
             }
          }
          FC_CAPTURE_AND_RETHROW( (owner_account) )
       }

       signed_transaction create_witness(string owner_account,
                                         string url,
                                         string asset_symbol,
                                         bool broadcast /* = false */)
       { try {
          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
          account_object witness_account = get_account(owner_account);
          fc::ecc::private_key active_private_key = get_private_key_for_account(witness_account);
          int witness_key_index = find_first_unused_derived_key_index(active_private_key);
          fc::ecc::private_key witness_private_key = derive_private_key(key_to_wif(active_private_key), witness_key_index);
          graphene::chain::public_key_type witness_public_key = witness_private_key.get_public_key();

          witness_create_operation witness_create_op;
          witness_create_op.witness_account = witness_account.id;
          witness_create_op.block_signing_key = witness_public_key;
          witness_create_op.url = url;

          if (_remote_db->get_witness_by_account(witness_create_op.witness_account))
             FC_THROW("Account ${owner_account} is already a witness", ("owner_account", owner_account));

          signed_transaction tx;
          tx.operations.push_back( witness_create_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
          tx.validate();

          _wallet.pending_witness_registrations[owner_account] = key_to_wif(witness_private_key);

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

       signed_transaction update_witness(string witness_name,
                                         string url,
                                         string block_signing_key,
                                         string asset_symbol,
                                         bool broadcast /* = false */)
       { try {
          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
          witness_object witness = get_witness(witness_name);
          account_object witness_account = get_account( witness.witness_account );
          fc::ecc::private_key active_private_key = get_private_key_for_account(witness_account);

          witness_update_operation witness_update_op;
          witness_update_op.witness = witness.id;
          witness_update_op.witness_account = witness_account.id;
          if( url != "" )
             witness_update_op.new_url = url;
          if( block_signing_key != "" )
             witness_update_op.new_signing_key = public_key_type( block_signing_key );

          signed_transaction tx;
          tx.operations.push_back( witness_update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (witness_name)(url)(block_signing_key)(broadcast) ) }

       template<typename WorkerInit>
       static WorkerInit _create_worker_initializer( const variant& worker_settings )
       {
          WorkerInit result;
          from_variant( worker_settings, result );
          return result;
       }

       signed_transaction create_worker(
          string owner_account,
          time_point_sec work_begin_date,
          time_point_sec work_end_date,
          share_type daily_pay,
          string name,
          string url,
          variant worker_settings,
          bool broadcast
          )
       {
          worker_initializer init;
          std::string wtype = worker_settings["type"].get_string();

          // TODO:  Use introspection to do this dispatch
          if( wtype == "burn" )
             init = _create_worker_initializer< burn_worker_initializer >( worker_settings );
          else if( wtype == "refund" )
             init = _create_worker_initializer< refund_worker_initializer >( worker_settings );
          else if( wtype == "vesting" )
             init = _create_worker_initializer< vesting_balance_worker_initializer >( worker_settings );
          else
          {
             FC_ASSERT( false, "unknown worker[\"type\"] value" );
          }

          worker_create_operation op;
          op.owner = get_account( owner_account ).id;
          op.work_begin_date = work_begin_date;
          op.work_end_date = work_end_date;
          op.daily_pay = daily_pay;
          op.name = name;
          op.url = url;
          op.initializer = init;

          signed_transaction tx;
          tx.operations.push_back( op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
          tx.validate();

          return sign_transaction( tx, broadcast );
       }

       signed_transaction update_worker_votes(
          string account,
          worker_vote_delta delta,
          bool broadcast
          )
       {
          account_object acct = get_account( account );
          account_update_operation op;

          // you could probably use a faster algorithm for this, but flat_set is fast enough :)
          flat_set< worker_id_type > merged;
          merged.reserve( delta.vote_for.size() + delta.vote_against.size() + delta.vote_abstain.size() );
          for( const worker_id_type& wid : delta.vote_for )
          {
             bool inserted = merged.insert( wid ).second;
             FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
          }
          for( const worker_id_type& wid : delta.vote_against )
          {
             bool inserted = merged.insert( wid ).second;
             FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
          }
          for( const worker_id_type& wid : delta.vote_abstain )
          {
             bool inserted = merged.insert( wid ).second;
             FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
          }

          // should be enforced by FC_ASSERT's above
          assert( merged.size() == delta.vote_for.size() + delta.vote_against.size() + delta.vote_abstain.size() );

          vector< object_id_type > query_ids;
          for( const worker_id_type& wid : merged )
             query_ids.push_back( wid );

          flat_set<vote_id_type> new_votes( acct.options.votes );

          fc::variants objects = _remote_db->get_objects( query_ids );
          for( const variant& obj : objects )
          {
             worker_object wo;
             from_variant( obj, wo );
             new_votes.erase( wo.vote_for );
             new_votes.erase( wo.vote_against );
             if( delta.vote_for.find( wo.id ) != delta.vote_for.end() )
                new_votes.insert( wo.vote_for );
             else if( delta.vote_against.find( wo.id ) != delta.vote_against.end() )
                new_votes.insert( wo.vote_against );
             else
                assert( delta.vote_abstain.find( wo.id ) != delta.vote_abstain.end() );
          }

          account_update_operation update_op;
          update_op.account = acct.id;
          update_op.new_options = acct.options;
          update_op.new_options->votes = new_votes;

          signed_transaction tx;
          tx.operations.push_back( update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
          tx.validate();

          return sign_transaction( tx, broadcast );
       }

       vector< vesting_balance_object_with_info > get_vesting_balances( string account_name )
       { try {
          fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>( account_name );
          std::vector<vesting_balance_object_with_info> result;
          fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

          if( vbid )
          {
             result.emplace_back( get_object<vesting_balance_object>(*vbid), now );
             return result;
          }

          // try casting to avoid a round-trip if we were given an account ID
          fc::optional<account_id_type> acct_id = maybe_id<account_id_type>( account_name );
          if( !acct_id )
             acct_id = get_account( account_name ).id;

          vector< vesting_balance_object > vbos = _remote_db->get_vesting_balances( *acct_id );
          if( vbos.size() == 0 )
             return result;

          for( const vesting_balance_object& vbo : vbos )
             result.emplace_back( vbo, now );

          return result;
       } FC_CAPTURE_AND_RETHROW( (account_name) )
       }

       signed_transaction withdraw_vesting(
          string witness_name,
          string amount,
          string asset_symbol,
          bool broadcast = false )
       { try {
          asset_object asset_obj = get_asset( asset_symbol );
          fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(witness_name);
          if( !vbid )
          {
             witness_object wit = get_witness( witness_name );
             FC_ASSERT( wit.pay_vb );
             vbid = wit.pay_vb;
          }

          vesting_balance_object vbo = get_object< vesting_balance_object >( *vbid );
          vesting_balance_withdraw_operation vesting_balance_withdraw_op;

          vesting_balance_withdraw_op.vesting_balance = *vbid;
          vesting_balance_withdraw_op.owner = vbo.owner;
          vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

          signed_transaction tx;
          tx.operations.push_back( vesting_balance_withdraw_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (witness_name)(amount) )
       }

       signed_transaction update_account_multisig_keys(string account, string type, uint32_t weight_threshold, vector<public_key_type> key_auths, vector<weight_type> key_weights, string fee_symbol, bool broadcast)
       { try {
             FC_ASSERT(OWNER == type || ACTIVE == type || ALL == type);
             FC_ASSERT(key_auths.size() == key_weights.size());
             account_object updating_account = get_account(account);

             fc::optional<asset_object> asset_obj = get_asset(fee_symbol);
             FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", fee_symbol));

             auto auth = authority();
             auth.weight_threshold = weight_threshold;
             for (uint8_t i = 0; i < key_auths.size(); ++i) {
                 auth.add_authority(key_auths.at(i), key_weights.at(i));
             }

             account_update_operation op;
             op.account = updating_account.id;
             op.owner = updating_account.owner;
             op.active = updating_account.active;
             if (ACTIVE == type) {
                 op.active = auth;
             } else if (OWNER == type) {
                 op.owner = auth;
             } else if (ALL == type) {
                 op.active = auth;
                 op.owner = auth;
             }

             signed_transaction tx;
             tx.operations.push_back(op);
             set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
             tx.validate();

             return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((account)(type)(weight_threshold)(key_auths)(key_weights)(fee_symbol)(broadcast)) }

       signed_transaction update_account_multisig(string account, string type, uint32_t weight_threshold, vector<string> account_auths, vector<weight_type> account_weights, string fee_symbol, bool broadcast)
       { try {
             FC_ASSERT(OWNER == type || ACTIVE == type || ALL == type);
             FC_ASSERT(account_auths.size() == account_weights.size());
             account_object updating_account = get_account(account);

             fc::optional<asset_object> asset_obj = get_asset(fee_symbol);
             FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", fee_symbol));

             vector<account_id_type> account_ids;
             account_ids.reserve(account_auths.size());
             std::transform(account_auths.begin(), account_auths.end(),
                           std::inserter(account_ids, account_ids.end()),
                           [this](const std::string& account_name_or_id){ return get_account_id(account_name_or_id); });
             auto auth = authority();
             auth.weight_threshold = weight_threshold;
             for (uint8_t i = 0; i < account_ids.size(); ++i) {
                 auth.add_authority(account_ids.at(i), account_weights.at(i));
             }

             account_update_operation op;
             op.account = updating_account.id;
             op.owner = updating_account.owner;
             op.active = updating_account.active;
             if (ACTIVE == type) {
                 op.active = auth;
             } else if (OWNER == type) {
                 op.owner = auth;
             } else if (ALL == type) {
                 op.active = auth;
                 op.owner = auth;
             }

             signed_transaction tx;
             tx.operations.push_back(op);
             set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
             tx.validate();

             return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((account)(type)(weight_threshold)(account_auths)(account_weights)(fee_symbol)(broadcast)) }

       signed_transaction vote_for_committee_member(string voting_account,
                                            string committee_member,
                                            bool approve,
                                            bool broadcast /* = false */)
       { try {
          account_object voting_account_object = get_account(voting_account);
          account_id_type committee_member_owner_account_id = get_account_id(committee_member);
          fc::optional<committee_member_object> committee_member_obj = _remote_db->get_committee_member_by_account(committee_member_owner_account_id);
          if (!committee_member_obj)
             FC_THROW("Account ${committee_member} is not registered as a committee_member", ("committee_member", committee_member));
          if (approve)
          {
             auto insert_result = voting_account_object.options.votes.insert(committee_member_obj->vote_id);
             if (!insert_result.second)
                FC_THROW("Account ${account} was already voting for committee_member ${committee_member}", ("account", voting_account)("committee_member", committee_member));
          }
          else
          {
             unsigned votes_removed = voting_account_object.options.votes.erase(committee_member_obj->vote_id);
             if (!votes_removed)
                FC_THROW("Account ${account} is already not voting for committee_member ${committee_member}", ("account", voting_account)("committee_member", committee_member));
          }
          account_update_operation account_update_op;
          account_update_op.account = voting_account_object.id;
          account_update_op.new_options = voting_account_object.options;

          signed_transaction tx;
          tx.operations.push_back( account_update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (voting_account)(committee_member)(approve)(broadcast) ) }

       signed_transaction vote_for_witness(string voting_account,
                                            string witness,
                                            bool approve,
                                            bool broadcast /* = false */)
       { try {
          account_object voting_account_object = get_account(voting_account);
          account_id_type witness_owner_account_id = get_account_id(witness);
          fc::optional<witness_object> witness_obj = _remote_db->get_witness_by_account(witness_owner_account_id);
          if (!witness_obj)
             FC_THROW("Account ${witness} is not registered as a witness", ("witness", witness));
          if (approve)
          {
             auto insert_result = voting_account_object.options.votes.insert(witness_obj->vote_id);
             if (!insert_result.second)
                FC_THROW("Account ${account} was already voting for witness ${witness}", ("account", voting_account)("witness", witness));
          }
          else
          {
             unsigned votes_removed = voting_account_object.options.votes.erase(witness_obj->vote_id);
             if (!votes_removed)
                FC_THROW("Account ${account} is already not voting for witness ${witness}", ("account", voting_account)("witness", witness));
          }
          account_update_operation account_update_op;
          account_update_op.account = voting_account_object.id;
          account_update_op.new_options = voting_account_object.options;

          signed_transaction tx;
          tx.operations.push_back( account_update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (voting_account)(witness)(approve)(broadcast) ) }

       signed_transaction set_voting_proxy(string account_to_modify,
                                           optional<string> voting_account,
                                           bool broadcast /* = false */)
       { try {
          account_object account_object_to_modify = get_account(account_to_modify);
          if (voting_account)
          {
             account_id_type new_voting_account_id = get_account_id(*voting_account);
             if (account_object_to_modify.options.voting_account == new_voting_account_id)
                FC_THROW("Voting proxy for ${account} is already set to ${voter}", ("account", account_to_modify)("voter", *voting_account));
             account_object_to_modify.options.voting_account = new_voting_account_id;
          }
          else
          {
             if (account_object_to_modify.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT)
                FC_THROW("Account ${account} is already voting for itself", ("account", account_to_modify));
             account_object_to_modify.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
          }

          account_update_operation account_update_op;
          account_update_op.account = account_object_to_modify.id;
          account_update_op.new_options = account_object_to_modify.options;

          signed_transaction tx;
          tx.operations.push_back( account_update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(broadcast) ) }

       signed_transaction set_desired_witness_and_committee_member_count(string account_to_modify,
                                                                 uint16_t desired_number_of_witnesses,
                                                                 uint16_t desired_number_of_committee_members,
                                                                 bool broadcast /* = false */)
       { try {
          account_object account_object_to_modify = get_account(account_to_modify);

          if (account_object_to_modify.options.num_witness == desired_number_of_witnesses &&
              account_object_to_modify.options.num_committee == desired_number_of_committee_members)
             FC_THROW("Account ${account} is already voting for ${witnesses} witnesses and ${committee_members} committee_members",
                      ("account", account_to_modify)("witnesses", desired_number_of_witnesses)("committee_members",desired_number_of_witnesses));
          account_object_to_modify.options.num_witness = desired_number_of_witnesses;
          account_object_to_modify.options.num_committee = desired_number_of_committee_members;

          account_update_operation account_update_op;
          account_update_op.account = account_object_to_modify.id;
          account_update_op.new_options = account_object_to_modify.options;

          signed_transaction tx;
          tx.operations.push_back( account_update_op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
          tx.validate();

          return sign_transaction( tx, broadcast );
       } FC_CAPTURE_AND_RETHROW( (account_to_modify)(desired_number_of_witnesses)(desired_number_of_committee_members)(broadcast) ) }

       signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
       {
           set<public_key_type> pks = _remote_db->get_potential_signatures(tx);
           flat_set<public_key_type> owned_keys;
           owned_keys.reserve(pks.size());
           std::copy_if( pks.begin(), pks.end(), std::inserter(owned_keys, owned_keys.end()),
                   [this](const public_key_type& pk){ return _keys.find(pk) != _keys.end(); } );
           tx.signatures.clear();
           set<public_key_type> approving_key_set = _remote_db->get_required_signatures(tx, owned_keys);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block( dyn_props.head_block_id );

           // first, some bookkeeping, expire old items from _recently_generated_transactions
           // since transactions include the head block id, we just need the index for keeping transactions unique
           // when there are multiple transactions in the same block.  choose a time period that should be at
           // least one block long, even in the worst case.  2 minutes ought to be plenty.
           fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(2));
           auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
           auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
           _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);

           uint32_t expiration_time_offset = 0;
           for (;;)
           {
               tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
               tx.signatures.clear();

               for (const public_key_type &key : approving_key_set)
                   tx.sign(get_private_key(key), _chain_id);

               graphene::chain::transaction_id_type this_transaction_id = tx.id();
               auto iter = _recently_generated_transactions.find(this_transaction_id);
               if (iter == _recently_generated_transactions.end())
               {
                   // we haven't generated this transaction before, the usual case
                   recently_generated_transaction_record this_transaction_record;
                   this_transaction_record.generation_time = dyn_props.time;
                   this_transaction_record.transaction_id = this_transaction_id;
                   _recently_generated_transactions.insert(this_transaction_record);
                   break;
               }

               // else we've generated a dupe, increment expiration time and re-sign it
               ++expiration_time_offset;
           }

           if (broadcast)
           {
               try
               {
                   _remote_net_broadcast->broadcast_transaction(tx);
               }
               catch (const fc::exception& e)
               {
                   elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
                   throw;
               }
           }

           return tx;
       }

       memo_data sign_memo(string from, string to, string memo)
       {
           FC_ASSERT(!self.is_locked());

           memo_data md = memo_data();

           // get account memo key, if that fails, try a pubkey
           try {
               account_object from_account = get_account(from);
               md.from = from_account.options.memo_key;
           } catch (const fc::exception& e) {
               md.from = self.get_public_key(from);
           }
           // same as above, for destination key
           try {
               account_object to_account = get_account(to);
               md.to = to_account.options.memo_key;
           } catch (const fc::exception& e) {
               md.to = self.get_public_key(to);
           }

           md.set_message(get_private_key(md.from), md.to, memo);
           return md;
       }

       string read_memo(const memo_data& md)
       {
           FC_ASSERT(!is_locked());
           std::string clear_text;

           const memo_data *memo = &md;

           try {
               FC_ASSERT(_keys.count(memo->to) || _keys.count(memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", memo->to)("from", memo->from));
               if (_keys.count(memo->to)) {
                   auto my_key = wif_to_key(_keys.at(memo->to));
                   FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                   clear_text = memo->get_message(*my_key, memo->from);
               } else {
                   auto my_key = wif_to_key(_keys.at(memo->from));
                   FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                   clear_text = memo->get_message(*my_key, memo->to);
               }
           } catch (const fc::exception& e) {
               elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
           }

           return clear_text;
       }

       signed_transaction transfer(string from, string to, string amount,
                                   string asset_symbol, string memo, bool broadcast = false)
       { try {
          FC_ASSERT( !self.is_locked() );
          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

          account_object from_account = get_account(from);
          account_object to_account = get_account(to);
          account_id_type from_id = from_account.id;
          account_id_type to_id = get_account_id(to);

          transfer_operation xfer_op;

          xfer_op.from = from_id;
          xfer_op.to = to_id;
          xfer_op.amount = asset_obj->amount_from_string(amount);

          if( memo.size() )
             {
                xfer_op.memo = memo_data();
                xfer_op.memo->from = from_account.options.memo_key;
                xfer_op.memo->to = to_account.options.memo_key;
                xfer_op.memo->set_message(get_private_key(from_account.options.memo_key),
                                          to_account.options.memo_key, memo);
             }

          signed_transaction tx;
          tx.operations.push_back(xfer_op);
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

       pair<graphene::chain::transaction_id_type,signed_transaction> transfer3(string from,
                                                                               string to,
                                                                               string amount,
                                                                               string asset_symbol,
                                                                               string memo,
                                                                               string fee_asset_symbol,
                                                                               bool broadcast)
       { try {
          FC_ASSERT(!self.is_locked());
          fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
          FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
          fc::optional<asset_object> fee_asset_obj = get_asset(fee_asset_symbol);

          account_object from_account = get_account(from);
          account_object to_account = get_account(to);
          account_id_type from_id = from_account.id;
          account_id_type to_id = get_account_id(to);

          transfer_operation xfer_op;
          xfer_op.from = from_id;
          xfer_op.to = to_id;
          xfer_op.amount = asset_obj->amount_from_string(amount);

          if (memo.size()) {
              xfer_op.memo = memo_data();
              xfer_op.memo->from = from_account.options.memo_key;
              xfer_op.memo->to = to_account.options.memo_key;
              xfer_op.memo->set_message(get_private_key(from_account.options.memo_key),
                                        to_account.options.memo_key, memo);
             }

          signed_transaction tx;
          tx.operations.push_back(xfer_op);
          set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
          tx.validate();

          auto trx = sign_transaction(tx, broadcast);
          return std::make_pair(trx.id(), trx);
       } FC_CAPTURE_AND_RETHROW((from)(to)(amount)(asset_symbol)(memo)(fee_asset_symbol)(broadcast)) }

       signed_transaction issue_asset(string to_account,
                                       string amount,
                                       string symbol,
                                       string memo,
                                       string fee_asset_symbol,
                                       bool broadcast = false)
       { try {
          auto asset_obj = get_asset(symbol);

          fc::optional<asset_object> fee_asset_obj = get_asset(fee_asset_symbol);
          FC_ASSERT(fee_asset_obj, "Could not find asset matching ${asset}", ("asset", fee_asset_symbol));

          account_object to = get_account(to_account);
          account_object issuer = get_account(asset_obj.issuer);

          asset_issue_operation issue_op;
          issue_op.issuer           = asset_obj.issuer;
          issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
          issue_op.issue_to_account = to.id;

          if (memo.size()) {
              issue_op.memo = memo_data();
              issue_op.memo->from = issuer.options.memo_key;
              issue_op.memo->to = to.options.memo_key;
              issue_op.memo->set_message(get_private_key(issuer.options.memo_key),
                                         to.options.memo_key, memo);
          }

          signed_transaction tx;
          tx.operations.push_back(issue_op);
          set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
          tx.validate();

          return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((to_account)(amount)(symbol)(memo)(fee_asset_symbol)(broadcast)) }

       std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
       {
          std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
          m["help"] = [](variant result, const fc::variants& a)
          {
             return result.get_string();
          };

          m["gethelp"] = [](variant result, const fc::variants& a)
          {
             return result.get_string();
          };

          m["get_account_history"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<vector<operation_detail>>();
             std::stringstream ss;
             for( operation_detail& d : r )
             {
                operation_history_object& i = d.op;
                auto b = _remote_db->get_block_header(i.block_num);
                FC_ASSERT(b);
                ss << b->timestamp.to_iso_string() << " ";
                i.op.visit(operation_printer(ss, *this, i.result));
                ss << " \n";
             }

             return ss.str();
          };

          m["get_account_history_by_operations"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<account_history_operation_detail>();
             std::stringstream ss;
             ss << "total_without_operations : ";
             ss << r.total_without_operations;
             ss << " \n";
             for( operation_detail_ex& d : r.details )
             {
                operation_history_object& i = d.op;
                auto b = _remote_db->get_block_header(i.block_num);
                FC_ASSERT(b);
                ss << b->timestamp.to_iso_string() << " ";
                i.op.visit(operation_printer(ss, *this, i.result));
                ss << " transaction_id : ";
                ss << d.transaction_id.str();
                ss << " \n";
             }

             return ss.str();
          };

          m["get_irreversible_account_history"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<irreversible_account_history_detail>();
             std::stringstream ss;
             ss << "next_start_sequence : " << r.next_start_sequence << "\n";
             ss << "result_count : " << r.result_count << "\n";
             for (operation_detail_ex &d : r.details)
             {
                 operation_history_object &i = d.op;
                 auto b = _remote_db->get_block_header(i.block_num);
                 FC_ASSERT(b);
                 ss << b->timestamp.to_iso_string() << " ";
                 i.op.visit(operation_printer(ss, *this, i.result));
                 ss << " transaction_id : " << d.transaction_id.str() << "\n";
             }

             return ss.str();
          };
          m["get_relative_account_history"] = [this](variant result, const fc::variants& a)
          {
              auto r = result.as<vector<operation_detail>>();
              std::stringstream ss;

              for( operation_detail& d : r )
              {
                  operation_history_object& i = d.op;
                  auto b = _remote_db->get_block_header(i.block_num);
                  FC_ASSERT(b);
                  ss << b->timestamp.to_iso_string() << " ";
                  i.op.visit(operation_printer(ss, *this, i.result));
                  ss << " \n";
              }

              return ss.str();
          };

          m["list_account_balances"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<vector<asset>>();
             vector<asset_object> asset_recs;
             std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
                return get_asset(a.asset_id);
             });

             std::stringstream ss;
             for( unsigned i = 0; i < asset_recs.size(); ++i )
                ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

             return ss.str();
          };

          m["list_account_lock_balances"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<vector<asset>>();
             vector<asset_object> asset_recs;
             std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
                return get_asset(a.asset_id);
             });

             std::stringstream ss;
             for( unsigned i = 0; i < asset_recs.size(); ++i )
                ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

             return ss.str();
          };

          m["get_blind_balances"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<vector<asset>>();
             vector<asset_object> asset_recs;
             std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
                return get_asset(a.asset_id);
             });

             std::stringstream ss;
             for( unsigned i = 0; i < asset_recs.size(); ++i )
                ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

             return ss.str();
          };
          m["transfer_to_blind"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<blind_confirmation>();
             std::stringstream ss;
             r.trx.operations[0].visit( operation_printer( ss, *this, operation_result() ) );
             ss << "\n";




             for( const auto& out : r.outputs )
             {
                asset_object a = get_asset( out.decrypted_memo.amount.asset_id );
                ss << a.amount_to_pretty_string( out.decrypted_memo.amount ) << " to  " << out.label << "\n\t  receipt: " << out.confirmation_receipt <<"\n\n";
             }
             return ss.str();
          };
          m["blind_transfer"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<blind_confirmation>();
             std::stringstream ss;
             r.trx.operations[0].visit( operation_printer( ss, *this, operation_result() ) );
             ss << "\n";
             for( const auto& out : r.outputs )
             {
                asset_object a = get_asset( out.decrypted_memo.amount.asset_id );
                ss << a.amount_to_pretty_string( out.decrypted_memo.amount ) << " to  " << out.label << "\n\t  receipt: " << out.confirmation_receipt <<"\n\n";
             }
             return ss.str();
          };
          m["receive_blind_transfer"] = [this](variant result, const fc::variants& a)
          {
             auto r = result.as<blind_receipt>();
             std::stringstream ss;
             asset_object as = get_asset( r.amount.asset_id );
             ss << as.amount_to_pretty_string( r.amount ) << "  " << r.from_label << "  =>  " << r.to_label  << "  " << r.memo <<"\n";
             return ss.str();
          };
          m["blind_history"] = [this](variant result, const fc::variants& a)
          {
             auto records = result.as<vector<blind_receipt>>();
             std::stringstream ss;
             ss << "WHEN         "
                << "  " << "AMOUNT"  << "  " << "FROM" << "  =>  " << "TO" << "  " << "MEMO" <<"\n";
             ss << "====================================================================================\n";
             for( auto& r : records )
             {
                asset_object as = get_asset( r.amount.asset_id );
                ss << fc::get_approximate_relative_time_string( r.date )
                   << "  " << as.amount_to_pretty_string( r.amount ) << "  " << r.from_label << "  =>  " << r.to_label  << "  " << r.memo <<"\n";
             }
             return ss.str();
          };
          m["get_order_book"] = [](variant result, const fc::variants& a)
          {
             auto orders = result.as<order_book>();
             auto bids = orders.bids;
             auto asks = orders.asks;
             std::stringstream ss;
             std::stringstream sum_stream;
             sum_stream << "Sum(" << orders.base << ')';
             double bid_sum = 0;
             double ask_sum = 0;
             const int spacing = 20;

             auto prettify_num = [&]( double n )
             {
                //ss << n;
                if (abs( round( n ) - n ) < 0.00000000001 )
                {
                   //ss << setiosflags( !ios::fixed ) << (int) n;     // doesn't compile on Linux with gcc
                   ss << (int) n;
                }
                else if (n - floor(n) < 0.000001)
                {
                   ss << setiosflags( ios::fixed ) << setprecision(10) << n;
                }
                else
                {
                   ss << setiosflags( ios::fixed ) << setprecision(6) << n;
                }
             };

             ss << setprecision( 8 ) << setiosflags( ios::fixed ) << setiosflags( ios::left );

             ss << ' ' << setw( (spacing * 4) + 6 ) << "BUY ORDERS" << "SELL ORDERS\n"
                << ' ' << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
                << orders.base << ' ' << setw( spacing ) << sum_stream.str()
                << "   " << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
                << orders.base << ' ' << setw( spacing ) << sum_stream.str()
                << "\n====================================================================================="
                << "|=====================================================================================\n";

             for (int i = 0; i < bids.size() || i < asks.size() ; i++)
             {
                if ( i < bids.size() )
                {
                    bid_sum += bids[i].base;
                    ss << ' ' << setw( spacing );
                    prettify_num( bids[i].price );
                    ss << ' ' << setw( spacing );
                    prettify_num( bids[i].quote );
                    ss << ' ' << setw( spacing );
                    prettify_num( bids[i].base );
                    ss << ' ' << setw( spacing );
                    prettify_num( bid_sum );
                    ss << ' ';
                }
                else
                {
                    ss << setw( (spacing * 4) + 5 ) << ' ';
                }

                ss << '|';

                if ( i < asks.size() )
                {
                   ask_sum += asks[i].base;
                   ss << ' ' << setw( spacing );
                   prettify_num( asks[i].price );
                   ss << ' ' << setw( spacing );
                   prettify_num( asks[i].quote );
                   ss << ' ' << setw( spacing );
                   prettify_num( asks[i].base );
                   ss << ' ' << setw( spacing );
                   prettify_num( ask_sum );
                }

                ss << '\n';
             }

             ss << endl
                << "Buy Total:  " << bid_sum << ' ' << orders.base << endl
                << "Sell Total: " << ask_sum << ' ' << orders.base << endl;

             return ss.str();
          };

          return m;
       }

       signed_transaction propose_league_update(
          const string& proposing_account,
          string league_id,
          string new_league_name,
          string new_brief_desc,
          vector<account_id_type> new_members,
          vector <league_data_product_id_type> new_data_products,
          vector <uint64_t> new_prices,
          string new_category_id,
          string new_icon,
          uint8_t new_status,
          vector<uint64_t> new_pocs_thresholds,
          vector<uint64_t> new_fee_bases,
          vector<uint64_t> new_pocs_weights,
          bool new_recommend,
          bool broadcast
          )
       {
           // construct league_update_operation
           league_object league = this->get_league(league_id);
           data_market_category_object  data_market_category = this->get_data_market_category(new_category_id);

           FC_ASSERT(0 != (&league));
           FC_ASSERT(0 != (&data_market_category));
           FC_ASSERT(new_league_name != "", "new_league_name cannot be empty");
           FC_ASSERT((0==new_status || 1==new_status || 2==new_status), "new_status cannot out of {0,1,2}");
           FC_ASSERT(new_data_products.size() == new_prices.size(), "new_data_products size must equal to new_prices size");

           for (auto &price : new_prices) {
               price = price * GRAPHENE_BLOCKCHAIN_PRECISION;
           }

           league_update_operation update_op;
           update_op.league = league.id;
           update_op.new_league_name = new_league_name;
           update_op.new_brief_desc = new_brief_desc;
           update_op.new_data_products = new_data_products;
           update_op.new_prices = new_prices;
           update_op.new_status = new_status;
           update_op.new_category_id = data_market_category.id;
           update_op.new_icon = new_icon;
           if(new_recommend){
               update_op.new_recommend_expiration_date_time = time_point_sec::maximum();
           }else{
               update_op.new_recommend_expiration_date_time = time_point_sec::min();
           }
           update_op.new_members = new_members;
           pocs_threshold_league_t ext;
           ext.pocs_thresholds = new_pocs_thresholds;
           ext.fee_bases = new_fee_bases;
           ext.product_pocs_weights = new_pocs_weights;
           update_op.extensions.insert(ext);

           const chain_parameters& current_params = get_global_properties().parameters;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
           // prop_op.review_period_seconds = current_params.committee_proposal_review_period;
           prop_op.fee_paying_account = get_account(proposing_account).id;
           prop_op.proposed_ops.emplace_back( update_op );
           current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction propose_parameter_change(
          const string& proposing_account,
          fc::time_point_sec expiration_time,
          const variant_object& changed_values,
          bool broadcast = false)
       {
          FC_ASSERT( !changed_values.contains("current_fees") );

          const chain_parameters& current_params = get_global_properties().parameters;
          chain_parameters new_params = current_params;
          fc::reflector<chain_parameters>::visit(
             fc::from_variant_visitor<chain_parameters>( changed_values, new_params )
             );

          committee_member_update_global_parameters_operation update_op;
          update_op.new_parameters = new_params;

          proposal_create_operation prop_op;

          prop_op.expiration_time = expiration_time;
          prop_op.review_period_seconds = current_params.committee_proposal_review_period;
          prop_op.fee_paying_account = get_account(proposing_account).id;

          prop_op.proposed_ops.emplace_back( update_op );
          current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

          signed_transaction tx;
          tx.operations.push_back(prop_op);
          set_operation_fees(tx, current_params.current_fees);
          tx.validate();

          return sign_transaction(tx, broadcast);
       }

       signed_transaction propose_gpo_extensions_change(
          const string& proposing_account,
          fc::time_point_sec expiration_time,
          const variant_object& changed_extensions,
          bool broadcast = false)
       {
           FC_ASSERT( !changed_extensions.contains("current_fees") );
           //only change extensions
           FC_ASSERT(changed_extensions.contains("extensions"));

           const chain_parameters& current_params = get_global_properties().parameters;
           chain_parameters new_params = current_params;
           fc::reflector<chain_parameters>::visit(
              fc::from_variant_visitor<chain_parameters>( changed_extensions, new_params )
              );
           extensions_type extension_tmp = new_params.extensions;
           for (auto& ext : current_params.extensions) {
              auto iter = std::find_if(extension_tmp.begin(), extension_tmp.end(), [&](future_extensions f_ext) -> bool{
                 return f_ext.which() == ext.which();
              });
              if (iter == extension_tmp.end()){
                 extension_tmp.insert(ext);
              }
           }
           new_params.extensions = extension_tmp;
           committee_member_update_global_parameters_operation update_op;
           update_op.new_parameters = new_params;

           proposal_create_operation prop_op;

           prop_op.expiration_time = expiration_time;
           prop_op.review_period_seconds = current_params.committee_proposal_review_period;
           prop_op.fee_paying_account = get_account(proposing_account).id;

           prop_op.proposed_ops.emplace_back( update_op );
           current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

           signed_transaction tx;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       }

       signed_transaction propose_fee_change(
          const string& proposing_account,
          fc::time_point_sec expiration_time,
          const variant_object& changed_fees,
          bool broadcast = false)
       {
          const chain_parameters& current_params = get_global_properties().parameters;
          const fee_schedule_type& current_fees = *(current_params.current_fees);

          flat_map< int, fee_parameters > fee_map;
          fee_map.reserve( current_fees.parameters.size() );
          for( const fee_parameters& op_fee : current_fees.parameters )
             fee_map[ op_fee.which() ] = op_fee;
          uint32_t scale = current_fees.scale;

          for( const auto& item : changed_fees )
          {
             const string& key = item.key();
             if( key == "scale" )
             {
                int64_t _scale = item.value().as_int64();
                FC_ASSERT( _scale >= 0 );
                FC_ASSERT( _scale <= std::numeric_limits<uint32_t>::max() );
                scale = uint32_t( _scale );
                continue;
             }
             // is key a number?
             auto is_numeric = [&]() -> bool
             {
                size_t n = key.size();
                for( size_t i=0; i<n; i++ )
                {
                   if( !isdigit( key[i] ) )
                      return false;
                }
                return true;
             };

             int which;
             if( is_numeric() )
                which = std::stoi( key );
             else
             {
                const auto& n2w = _operation_which_map.name_to_which;
                auto it = n2w.find( key );
                FC_ASSERT( it != n2w.end(), "unknown operation" );
                which = it->second;
             }

             fee_parameters fp = from_which_variant< fee_parameters >( which, item.value() );
             fee_map[ which ] = fp;
          }

          fee_schedule_type new_fees;

          for( const std::pair< int, fee_parameters >& item : fee_map )
             new_fees.parameters.insert( item.second );
          new_fees.scale = scale;

          chain_parameters new_params = current_params;
          new_params.current_fees = new_fees;

          committee_member_update_global_parameters_operation update_op;
          update_op.new_parameters = new_params;

          proposal_create_operation prop_op;

          prop_op.expiration_time = expiration_time;
          prop_op.review_period_seconds = current_params.committee_proposal_review_period;
          prop_op.fee_paying_account = get_account(proposing_account).id;

          prop_op.proposed_ops.emplace_back( update_op );
          current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

          signed_transaction tx;
          tx.operations.push_back(prop_op);
          set_operation_fees(tx, current_params.current_fees);
          tx.validate();

          return sign_transaction(tx, broadcast);
       }

       signed_transaction propose_deprecate_datasource(
        string propose_account,
        string datasource_account,
        bool broadcast = false)
        {try {
            FC_ASSERT( !self.is_locked() );
            account_object account_datasource_obj = get_account(datasource_account);
            const account_object propose_account_obj = get_account(propose_account);
            FC_ASSERT( account_datasource_obj.is_datasource_member() );
            FC_ASSERT( propose_account_obj.is_lifetime_member() );

            account_upgrade_datasource_operation op;
            op.account_to_upgrade = account_datasource_obj.get_id();
            op.upgrade_to_datasource_member = false;
            op.auth_referrer = propose_account_obj.get_id();

            operation_ext_version_t ext;
            ext.version = operation_version_one;
            op.extensions.insert(ext);

            const chain_parameters& current_params = get_global_properties().parameters;
            proposal_create_operation prop_op;
            prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
            prop_op.fee_paying_account = propose_account_obj.get_id();
            prop_op.proposed_ops.emplace_back(op);
            current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

            signed_transaction tx;
            tx.operations.push_back(prop_op);
            set_operation_fees(tx, current_params.current_fees);
            tx.validate();

            return sign_transaction(tx, broadcast);
        } FC_CAPTURE_AND_RETHROW( (datasource_account) ) }

        signed_transaction propose_deprecate_merchant(
            string propose_account,
            string merchant_account,
            bool broadcast = false)
        {try {
            FC_ASSERT( !self.is_locked() );
            account_object account_merchant_obj = get_account(merchant_account);
            const account_object propose_account_obj = get_account(propose_account);
            FC_ASSERT( propose_account_obj.is_lifetime_member() );
            FC_ASSERT( account_merchant_obj.is_merchant_member() );
            FC_ASSERT( account_merchant_obj.is_datasource_member() );

            account_upgrade_merchant_operation op;
            op.account_to_upgrade = account_merchant_obj.get_id();
            op.upgrade_to_merchant_member = false;
            op.auth_referrer = propose_account_obj.get_id();

            operation_ext_version_t ext;
            ext.version = operation_version_one;
            op.extensions.insert(ext);

            const chain_parameters& current_params = get_global_properties().parameters;
            proposal_create_operation prop_op;
            prop_op.expiration_time = fc::time_point::now() + std::min(fc::seconds(current_params.maximum_proposal_lifetime / 2), fc::days(1));
            prop_op.fee_paying_account = propose_account_obj.get_id();
            if (account_merchant_obj.is_datasource_member()){
                account_upgrade_datasource_operation opEx;
                opEx.account_to_upgrade = account_merchant_obj.get_id();
                opEx.upgrade_to_datasource_member = false;
                opEx.auth_referrer = propose_account_obj.get_id();

                operation_ext_version_t extEx;
                extEx.version = operation_version_one;
                opEx.extensions.insert(extEx);
                prop_op.proposed_ops.emplace_back(opEx);
                current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );
            }
            prop_op.proposed_ops.emplace_back(op);
            current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

            signed_transaction tx;
            tx.operations.push_back(prop_op);
            set_operation_fees(tx, current_params.current_fees);
            tx.validate();

            return sign_transaction(tx, broadcast);
        } FC_CAPTURE_AND_RETHROW( (merchant_account) ) }

       signed_transaction approve_proposal(
          const string& fee_paying_account,
          const string& proposal_id,
          const approval_delta& delta,
          bool broadcast = false)
       {
          proposal_update_operation update_op;

          update_op.fee_paying_account = get_account(fee_paying_account).id;
          update_op.proposal = fc::variant(proposal_id).as<proposal_id_type>();
          // make sure the proposal exists
          get_object( update_op.proposal );

          for( const std::string& name : delta.active_approvals_to_add )
             update_op.active_approvals_to_add.insert( get_account( name ).id );
          for( const std::string& name : delta.active_approvals_to_remove )
             update_op.active_approvals_to_remove.insert( get_account( name ).id );
          for( const std::string& name : delta.owner_approvals_to_add )
             update_op.owner_approvals_to_add.insert( get_account( name ).id );
          for( const std::string& name : delta.owner_approvals_to_remove )
             update_op.owner_approvals_to_remove.insert( get_account( name ).id );
          for( const std::string& k : delta.key_approvals_to_add )
             update_op.key_approvals_to_add.insert( public_key_type( k ) );
          for( const std::string& k : delta.key_approvals_to_remove )
             update_op.key_approvals_to_remove.insert( public_key_type( k ) );

          signed_transaction tx;
          tx.operations.push_back(update_op);
          set_operation_fees(tx, get_global_properties().parameters.current_fees);
          tx.validate();
          return sign_transaction(tx, broadcast);
       }

       void dbg_make_uia(string creator, string symbol)
       {
          asset_options opts;
          opts.flags &= ~(white_list | disable_force_settle | global_settle);
          opts.issuer_permissions = opts.flags;
          opts.core_exchange_rate = price(asset(1), asset(1,asset_id_type(1)));
          create_asset(get_account(creator).name, symbol, 2, opts, GRAPHENE_SYMBOL, true);
       }

       void dbg_push_blocks( const std::string& src_filename, uint32_t count )
       {
          use_debug_api();
          (*_remote_debug)->debug_push_blocks( src_filename, count );
          (*_remote_debug)->debug_stream_json_objects_flush();
       }

       void dbg_generate_blocks( const std::string& debug_wif_key, uint32_t count )
       {
          use_debug_api();
          (*_remote_debug)->debug_generate_blocks( debug_wif_key, count );
          (*_remote_debug)->debug_stream_json_objects_flush();
       }

       void dbg_stream_json_objects( const std::string& filename )
       {
          use_debug_api();
          (*_remote_debug)->debug_stream_json_objects( filename );
          (*_remote_debug)->debug_stream_json_objects_flush();
       }

       void dbg_update_object( const fc::variant_object& update )
       {
          use_debug_api();
          (*_remote_debug)->debug_update_object( update );
          (*_remote_debug)->debug_stream_json_objects_flush();
       }

       void use_network_node_api()
       {
          if( _remote_net_node )
             return;
          try
          {
             _remote_net_node = _remote_api->network_node();
          }
          catch( const fc::exception& e )
          {
             std::cerr << "\nCouldn't get network node API.  You probably are not configured\n"
             "to access the network API on the witness_node you are\n"
             "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
             "\n";
             throw(e);
          }
       }

       void use_debug_api()
       {
          if( _remote_debug )
             return;
          try
          {
            _remote_debug = _remote_api->debug();
          }
          catch( const fc::exception& e )
          {
             std::cerr << "\nCouldn't get debug node API.  You probably are not configured\n"
             "to access the debug API on the node you are connecting to.\n"
             "\n"
             "To fix this problem:\n"
             "- Please ensure you are running debug_node, not witness_node.\n"
             "- Please follow the instructions in README.md to set up an apiaccess file.\n"
             "\n";
          }
       }

       void network_add_nodes( const vector<string>& nodes )
       {
          use_network_node_api();
          for( const string& node_address : nodes )
          {
             (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
          }
       }

       vector< variant > network_get_connected_peers()
       {
          use_network_node_api();
          const auto peers = (*_remote_net_node)->get_connected_peers();
          vector< variant > result;
          result.reserve( peers.size() );
          for( const auto& peer : peers )
          {
             variant v;
             fc::to_variant( peer, v );
             result.push_back( v );
          }
          return result;
       }

       void flood_create_account(string prefix, uint32_t number_of_accounts)
       { try {
               const account_object &master = *_wallet.my_accounts.get<by_name>().lower_bound("import");

               fc::time_point start = fc::time_point::now();
               for (int i = 0; i < number_of_accounts; ++i) {
                   std::ostringstream brain_key;
                   brain_key << "brain key for account " << prefix << i;
                   signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i), master.name, master.name, /* broadcast = */ true, /* save wallet = */ false);
               }
               fc::time_point end = fc::time_point::now();
               ilog("Created ${n} accounts in ${time} milliseconds",
                    ("n", number_of_accounts)("time", (end - start).count() / 1000));
           } catch (...) {
               throw;
           }
       }

       void flood_transfer(string from_account, string account_prefix, uint32_t number_of_accounts, uint32_t number_of_loop)
       {
           fc::time_point start = fc::time_point::now();
           for (int i = 0; i < number_of_loop; ++i) {
               for (int j = 0; j < number_of_accounts; ++j) {
                   try {
                       signed_transaction trx = transfer(from_account, account_prefix + fc::to_string(j), "1", GRAPHENE_SYMBOL, "", true);
                       trx = transfer(from_account, account_prefix + fc::to_string(i), "1", GRAPHENE_SYMBOL, "", true);
                   } catch (...) {
                   }
               }
           }
           fc::time_point end = fc::time_point::now();
           ilog("Transferred to ${n} accounts in ${time} milliseconds, loop ${l}",
                   ("n", number_of_accounts * 2)("time", (end - start).count() / 1000)("l", number_of_loop));
       }

       bool verify_transaction_signature(const signed_transaction& trx, public_key_type pub_key)
       {
           return trx.validate_signee(pub_key, _chain_id);
       }

       bool verify_proxy_transfer_signature(const proxy_transfer_params& param, public_key_type pub_key)
       {
           return param.verify_proxy_transfer_signature(pub_key);
       }

       signed_transaction custom(string account, uint16_t id, string data, string fee_symbol, bool broadcast)
       { try {
           FC_ASSERT(!self.is_locked());
           account_object payer = get_account(account);
           fc::optional<asset_object> fee_asset_obj = get_asset(fee_symbol);

           custom_operation op;
           op.payer = payer.id;
           op.id = id;
           if (data.size()) {
               std::copy(data.begin(), data.end(), std::back_inserter(op.data));
           }

           string raw_data(op.data.begin(), op.data.end());
           idump((raw_data));

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, fee_asset_obj);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((account)(id)(data)(fee_symbol)(broadcast)) }

       void transfer_test(account_id_type from_account, account_id_type to_account, uint32_t times)
       {
            FC_ASSERT( !self.is_locked() );
            fc::optional<asset_object> asset_obj = get_asset("GXC");
            FC_ASSERT(asset_obj, "Could not find asset GXC");

            asset amount;
            amount.asset_id = asset_obj->id;
            amount.amount = 100000;
            fc::optional<account_object> from_account_obj = get_account(from_account);
            FC_ASSERT(from_account_obj, "Could not find account_object ${from_account}", ("from_account", from_account));

            for(int i = 0; i < times; ++i) {
                transfer_operation xfer_op;
                xfer_op.from = from_account;
                xfer_op.to = to_account;
                xfer_op.amount = amount;
                signed_transaction tx;
                tx.operations.push_back(xfer_op);
                set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, asset_obj);
                tx.validate();

                vector<public_key_type> paying_keys = from_account_obj->active.get_keys();
                auto dyn_props = get_dynamic_global_properties();
                tx.set_expiration( dyn_props.time + fc::seconds(300 + i) );
                for( public_key_type& key : paying_keys )
                {
                    auto it = _keys.find(key);
                    if( it != _keys.end() )
                    {
                        fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
                        FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
                        tx.sign( *privkey, _chain_id );
                    }
                }
                try
                {
                    _remote_net_broadcast->broadcast_transaction( tx );
                }
                catch (const fc::exception& e)
                {
                    elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
                    throw;
                }
            }
       }

       fc::sha256 get_hash(const string& value)
       {
           return fc::sha256::hash(value);
       }

      signature_type sign_string(string wif_key, const string &raw_string)
      {
          fc::optional<fc::ecc::private_key> privkey = wif_to_key(wif_key);
          FC_ASSERT(privkey.valid(), "Malformed private key in _keys");

          digest_type::encoder enc;
          fc::raw::pack(enc, raw_string);
          return privkey->sign_compact(enc.result());
      }

       void flood_network(string prefix, uint32_t number_of_transactions)
       { try {
               const u_int16_t loop_num = 1000;

               const account_object &master = *_wallet.my_accounts.get<by_name>().lower_bound("import");
               int number_of_accounts = number_of_transactions / loop_num;

               fc::time_point start = fc::time_point::now();
               for (int i = 0; i < number_of_accounts; ++i) {
                   std::ostringstream brain_key;
                   brain_key << "brain key for account " << prefix << i;
                   signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i), master.name, master.name, /* broadcast = */ true, /* save wallet = */ false);
               }
               fc::time_point end = fc::time_point::now();
               ilog("Created ${n} accounts in ${time} milliseconds",
                    ("n", number_of_accounts)("time", (end - start).count() / 1000));
               start = fc::time_point::now();
               for (int i = 0; i < loop_num; ++i) {
                   for (int j = 0; j < number_of_accounts; ++j) {
                       signed_transaction trx = transfer(master.name, prefix + fc::to_string(j), "2", GRAPHENE_SYMBOL, "", true);
                       trx = transfer(master.name, prefix + fc::to_string(i), "1", GRAPHENE_SYMBOL, "", true);
                   }
               }
               end = fc::time_point::now();
               ilog("Transferred to ${n} accounts in ${time} milliseconds, loop ${l}",
                    ("n", number_of_accounts * 2)("time", (end - start).count() / 1000)("l", loop_num));
           } catch (...) {
               throw;
           }
       }

       operation get_prototype_operation( string operation_name )
       {
          auto it = _prototype_ops.find( operation_name );
          if( it == _prototype_ops.end() )
             FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
          return it->second;
       }

       string                  _wallet_filename;
       wallet_data             _wallet;

       map<public_key_type,string> _keys;
       fc::sha512                  _checksum;

       chain_id_type           _chain_id;
       fc::api<login_api>      _remote_api;
       fc::api<database_api>   _remote_db;
       fc::api<network_broadcast_api>   _remote_net_broadcast;
       fc::api<history_api>    _remote_hist;
       optional< fc::api<network_node_api> > _remote_net_node;
       optional< fc::api<graphene::debug_witness::debug_api> > _remote_debug;

       flat_map<string, operation> _prototype_ops;

       static_variant_map _operation_which_map = create_static_variant_map< operation >();

    #ifdef __unix__
       unsigned int _old_umask;
    #endif
       const string _wallet_filename_extension = ".wallet";
    };

    std::string operation_printer::fee(const asset& a)const {
       out << "   (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
       return "";
    }

    template<typename T>
    std::string operation_printer::operator()(const T& op)const
    {
       //balance_accumulator acc;
       //op.get_balance_delta( acc, result );
       auto a = wallet.get_asset( op.fee.asset_id );
       auto payer = wallet.get_account( op.fee_payer() );

       string op_name = fc::get_typename<T>::name();
       if( op_name.find_last_of(':') != string::npos )
          op_name.erase(0, op_name.find_last_of(':')+1);
       out << op_name <<" ";
      // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
       out << payer.name << " fee: " << a.amount_to_pretty_string( op.fee );
       operation_result_printer rprinter(wallet);
       std::string str_result = result.visit(rprinter);
       if( str_result != "" )
       {
          out << "   result: " << str_result;
       }
       return "";
    }
    std::string operation_printer::operator()(const transfer_from_blind_operation& op)const
    {
       auto a = wallet.get_asset( op.fee.asset_id );
       auto receiver = wallet.get_account( op.to );

       out <<  receiver.name
       << " received " << a.amount_to_pretty_string( op.amount ) << " from blinded balance";
       return "";
    }
    std::string operation_printer::operator()(const transfer_to_blind_operation& op)const
    {
       auto fa = wallet.get_asset( op.fee.asset_id );
       auto a = wallet.get_asset( op.amount.asset_id );
       auto sender = wallet.get_account( op.from );

       out <<  sender.name
       << " sent " << a.amount_to_pretty_string( op.amount ) << " to " << op.outputs.size() << " blinded balance" << (op.outputs.size()>1?"s":"")
       << " fee: " << fa.amount_to_pretty_string( op.fee );
       return "";
    }
    string operation_printer::operator()(const transfer_operation& op) const
    {
       out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
           << " from " << wallet.get_account(op.from).name << " to " << wallet.get_account(op.to).name;
       std::string memo;
       if( op.memo )
       {
          if( wallet.is_locked() )
          {
             out << " -- Unlock wallet to see memo.";
          } else {
             try {
                FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
                if( wallet._keys.count(op.memo->to) ) {
                   auto my_key = wif_to_key(wallet._keys.at(op.memo->to));
                   FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                   memo = op.memo->get_message(*my_key, op.memo->from);
                   out << " -- Memo: " << memo;
                } else {
                   auto my_key = wif_to_key(wallet._keys.at(op.memo->from));
                   FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                   memo = op.memo->get_message(*my_key, op.memo->to);
                   out << " -- Memo: " << memo;
                }
             } catch (const fc::exception& e) {
                out << " -- could not decrypt memo";
             }
          }
       }
       fee(op.fee);
       return memo;
    }

    std::string operation_printer::operator()(const account_create_operation& op) const
    {
       out << "Create Account '" << op.name << "'";
       return fee(op.fee);
    }

    std::string operation_printer::operator()(const account_update_operation& op) const
    {
       out << "Update Account '" << wallet.get_account(op.account).name << "'";
       return fee(op.fee);
    }

    std::string operation_printer::operator()(const asset_create_operation& op) const
    {
       out << "Create ";
       if( op.bitasset_opts.valid() )
          out << "BitAsset ";
       else
          out << "User-Issue Asset ";
       out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
       return fee(op.fee);
    }

    std::string operation_result_printer::operator()(const void_result& x) const
    {
       return "";
    }

    std::string operation_result_printer::operator()(const object_id_type& oid)
    {
       return std::string(oid);
    }

    std::string operation_result_printer::operator()(const asset& a)
    {
       return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
    }
    
    std::string operation_result_printer::operator()(const contract_receipt& r)
    {
       return std::string(r);
    }

    }}}

    namespace graphene { namespace wallet {
       vector<brain_key_info> utility::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys)
       {
          // Safety-check
          FC_ASSERT( number_of_desired_keys >= 1 );

          // Create as many derived owner keys as requested
          vector<brain_key_info> results;
          brain_key = graphene::wallet::detail::normalize_brain_key(brain_key);
          for (int i = 0; i < number_of_desired_keys; ++i) {
            fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key( brain_key, i );

            brain_key_info result;
            result.brain_priv_key = brain_key;
            result.wif_priv_key = key_to_wif( priv_key );
            result.pub_key = priv_key.get_public_key();

            results.push_back(result);
          }

          return results;
       }

       brain_key_info utility::suggest_brain_key()
       {
           brain_key_info result;
           // create a private key for secure entropy
           fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
           fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
           fc::bigint entropy1(sha_entropy1.data(), sha_entropy1.data_size());
           fc::bigint entropy2(sha_entropy2.data(), sha_entropy2.data_size());
           fc::bigint entropy(entropy1);
           entropy <<= 8 * sha_entropy1.data_size();
           entropy += entropy2;
           string brain_key = "";

           for (int i = 0; i < BRAIN_KEY_WORD_COUNT; i++) {
               fc::bigint choice = entropy % graphene::words::word_list_size;
               entropy /= graphene::words::word_list_size;
               if (i > 0)
                   brain_key += " ";
               brain_key += graphene::words::word_list[choice.to_int64()];
           }

           brain_key = detail::normalize_brain_key(brain_key);
           fc::ecc::private_key priv_key = detail::derive_private_key(brain_key, 0);
           result.brain_priv_key = brain_key;
           result.wif_priv_key = key_to_wif(priv_key);
           result.pub_key = priv_key.get_public_key();
           return result;
       }
    }}

    namespace graphene { namespace wallet {

    wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
       : my(new detail::wallet_api_impl(*this, initial_data, rapi))
    {
    }

    wallet_api::~wallet_api()
    {
    }

    bool wallet_api::copy_wallet_file(string destination_filename)
    {
       return my->copy_wallet_file(destination_filename);
    }

    optional<signed_block_with_info> wallet_api::get_block(uint32_t num)const
    {
       return my->_remote_db->get_block(num);
    }

    optional<signed_block_with_info> wallet_api::get_block_by_id(block_id_type block_id)const
    {
       return my->_remote_db->get_block_by_id(block_id);
    }

    uint64_t wallet_api::get_account_count() const
    {
       return my->_remote_db->get_account_count();
    }

    uint64_t wallet_api::get_asset_count() const
    {
       return my->_remote_db->get_asset_count();
    }

    uint64_t wallet_api::get_data_transaction_product_costs(fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_product_costs(start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_total_count(fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_total_count(start, end);
    }

    map<account_id_type, uint64_t> wallet_api::list_data_transaction_complain_requesters(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const
    {
        return my->_remote_db->list_data_transaction_complain_requesters(start_date_time, end_date_time, limit);
    }

    map<account_id_type, uint64_t> wallet_api::list_data_transaction_complain_datasources(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint8_t limit) const
    {
        return my->_remote_db->list_data_transaction_complain_datasources(start_date_time, end_date_time, limit);
    }

    uint64_t wallet_api::get_merchants_total_count() const
    {
        return my->_remote_db->get_merchants_total_count();
    }

    uint64_t wallet_api::get_data_transaction_commission(fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_commission(start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_pay_fee(fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_pay_fee(start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_product_costs_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_product_costs_by_requester(requester, start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_total_count_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
    {
        return my->_remote_db->get_data_transaction_total_count_by_requester(requester, start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_pay_fees_by_requester(string requester, fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_pay_fees_by_requester(requester, start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_product_costs_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_product_costs_by_product_id(product_id, start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    uint64_t wallet_api::get_data_transaction_total_count_by_product_id(string product_id, fc::time_point_sec start, fc::time_point_sec end) const
    {
       return my->_remote_db->get_data_transaction_total_count_by_product_id(product_id, start, end) / GRAPHENE_BLOCKCHAIN_PRECISION;
    }

    vector<account_object> wallet_api::list_my_accounts()
    {
       return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
    }

    map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
    {
       return my->_remote_db->lookup_accounts(lowerbound, limit);
    }

    vector<asset> wallet_api::list_account_balances(const string& id)
    {
       if( auto real_id = detail::maybe_id<account_id_type>(id) )
          return my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
       return my->_remote_db->get_account_balances(get_account(id).id, flat_set<asset_id_type>());
    }

    vector<asset> wallet_api::list_account_lock_balances(const string& account_id_or_name)
    {
       if( auto real_id = detail::maybe_id<account_id_type>(account_id_or_name) ) {
          return my->_remote_db->get_account_lock_balances(*real_id, flat_set<asset_id_type>());
       }
       return my->_remote_db->get_account_lock_balances(get_account(account_id_or_name).id, flat_set<asset_id_type>());
    }

    vector<asset_object> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
    {
       return my->_remote_db->list_assets( lowerbound, limit );
    }

    vector<operation_detail> wallet_api::get_account_history(string name, int limit)const
    {
       vector<operation_detail> result;
       auto account_id = get_account(name).get_id();

       while( limit > 0 )
       {
          operation_history_id_type start;
          if( result.size() )
          {
             start = result.back().op.id;
             start = start + 1;
          }


          vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, operation_history_id_type(), std::min(100,limit), start);
          for( auto& o : current ) {
             std::stringstream ss;
             auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
             result.push_back( operation_detail{ memo, ss.str(), o } );
          }
          if( current.size() < std::min(100,limit) )
             break;
          limit -= current.size();
       }

       return result;
    }

    account_history_operation_detail wallet_api::get_account_history_by_operations(string account_name_or_id, vector<uint32_t> operation_indexs, uint32_t start, int limit)const
    {
        FC_ASSERT(limit <= 100 );
        vector<operation_detail_ex> detail_exs;
        account_history_operation_detail result;
        uint32_t total = 0;
        auto account_id = get_account(account_name_or_id).get_id();

        history_operation_detail current = my->_remote_hist->get_account_history_by_operations(account_id, operation_indexs, start, std::min(100,limit));
        total = current.total_without_operations;
        for (auto &operation_history_obj : current.operation_history_objs) {
            std::stringstream ss;
            transaction_id_type transaction_id;
            auto memo = operation_history_obj.op.visit(detail::operation_printer(ss, *my, operation_history_obj.result));
            optional<signed_block_with_info> block = get_block(operation_history_obj.block_num);
            if (block.valid()){
                if (operation_history_obj.trx_in_block < block->transaction_ids.size()){
                    transaction_id = block->transaction_ids[operation_history_obj.trx_in_block];
                }
            }
            detail_exs.push_back(operation_detail_ex{memo, ss.str(), operation_history_obj, transaction_id});
        }
        result.details = detail_exs;
        result.total_without_operations = total;
        return result;
    }

    irreversible_account_history_detail wallet_api::get_irreversible_account_history(string name, vector<uint32_t> operation_types, uint32_t start, int limit) const
    {
        irreversible_account_history_detail result;
        auto account_id = get_account(name).get_id();

        const auto& account = my->get_account(account_id);
        const auto& stats = my->get_object(account.statistics);

        // sequence of account_transaction_history_object start from 1
        start = start == 0 ? 1 : start;

        if (start <= stats.removed_ops) {
            start = stats.removed_ops;
        }
        result.next_start_sequence = start;

        // get irreversible block_num
        uint32_t irreversible_block_num = get_dynamic_global_properties().last_irreversible_block_num;

        // get operation history
        uint32_t counter = start;
        while (limit > 0 && counter <= stats.total_ops) {
            uint32_t min_limit = std::min<uint32_t> (100, limit);
            auto current = my->_remote_hist->get_account_history_by_operations(account_id, operation_types, start, min_limit);
            uint16_t skip_count = 0;
            for (auto& obj : current.operation_history_objs) {
                // only keep irreversible operation history
                if (obj.block_num > irreversible_block_num) {
                    ++skip_count;
                    continue;
                }

                std::stringstream ss;
                auto memo = obj.op.visit(detail::operation_printer(ss, *my, obj.result));

                transaction_id_type transaction_id;
                auto block = get_block(obj.block_num);
                if (block.valid() && obj.trx_in_block < block->transaction_ids.size()) {
                    transaction_id = block->transaction_ids[obj.trx_in_block];
                }
                result.details.push_back(operation_detail_ex{memo, ss.str(), obj, transaction_id});
            }

            start += (current.total_without_operations > 0 ? current.total_without_operations : min_limit) - skip_count;
            limit -= current.operation_history_objs.size() - skip_count;
            counter += min_limit;

            result.result_count += current.operation_history_objs.size() - skip_count;
            result.next_start_sequence = start;

            // current block_num > irreversible_block_num, break
            if (skip_count > 0) {
                break;
            }
        }
        return result;
    }

    vector<operation_detail> wallet_api::get_relative_account_history(string name, uint32_t stop, int limit, uint32_t start)const
    {
        FC_ASSERT( start >= 0 || limit <= 100 );

        vector<operation_detail> result;
        auto account_id = get_account(name).get_id();

        while( limit > 0 )
        {
            vector <operation_history_object> current = my->_remote_hist->get_relative_account_history(account_id, stop, std::min<uint32_t>(100, limit), start);
            for (auto &o : current) {
                std::stringstream ss;
                auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
                result.push_back(operation_detail{memo, ss.str(), o});
            }
            if (current.size() < std::min<uint32_t>(100, limit))
                break;
            limit -= current.size();
            start -= 100;
            if( start == 0 ) break;
        }
        return result;
    }

    vector<bucket_object> wallet_api::get_market_history( string symbol1, string symbol2, uint32_t bucket , fc::time_point_sec start, fc::time_point_sec end )const
    {
       return my->_remote_hist->get_market_history( get_asset_id(symbol1), get_asset_id(symbol2), bucket, start, end );
    }

    vector<limit_order_object> wallet_api::get_limit_orders(string a, string b, uint32_t limit)const
    {
       return my->_remote_db->get_limit_orders(get_asset(a).id, get_asset(b).id, limit);
    }

    vector<call_order_object> wallet_api::get_call_orders(string a, uint32_t limit)const
    {
       return my->_remote_db->get_call_orders(get_asset(a).id, limit);
    }

    vector<force_settlement_object> wallet_api::get_settle_orders(string a, uint32_t limit)const
    {
       return my->_remote_db->get_settle_orders(get_asset(a).id, limit);
    }

    brain_key_info wallet_api::suggest_brain_key()const
    {
        return graphene::wallet::utility::suggest_brain_key();
    }

    vector<brain_key_info> wallet_api::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys) const
    {
       return graphene::wallet::utility::derive_owner_keys_from_brain_key(brain_key, number_of_desired_keys);
    }

    bool wallet_api::is_public_key_registered(string public_key) const
    {
       bool is_known = my->_remote_db->is_public_key_registered(public_key);
       return is_known;
    }

    bool wallet_api::is_account_registered(string name) const
    {
       bool is_known = my->_remote_db->is_account_registered(name);
       return is_known;
    }

    memo_data wallet_api::sign_memo(string from, string to, string memo)
    {
        FC_ASSERT(!is_locked());
        return my->sign_memo(from, to, memo);
    }

    string wallet_api::read_memo(const memo_data& memo)
    {
        FC_ASSERT(!is_locked());
        return my->read_memo(memo);
    }

    string wallet_api::serialize_proxy_transfer_params(proxy_transfer_params param) const
    {
       return fc::to_hex(fc::raw::pack(param));
    }

    string wallet_api::serialize_transaction( signed_transaction tx )const
    {
       return fc::to_hex(fc::raw::pack(tx));
    }

    variant wallet_api::get_object( object_id_type id ) const
    {
       return my->_remote_db->get_objects({id});
    }

    signed_transaction wallet_api::create_data_market_category(string category_name,uint8_t data_market_type,uint32_t order_num,string issuer,bool broadcast /* = false */)
    {
        return my->create_data_market_category(category_name,data_market_type,order_num,issuer,broadcast);
    }

    signed_transaction wallet_api::propose_data_market_category_update(const string &proposing_account,string category_id,string new_category_name,uint32_t new_order_num,uint8_t new_status,bool broadcast)
    {
        return my->propose_data_market_category_update(proposing_account, category_id, new_category_name, new_order_num, new_status, broadcast);
    }

    signed_transaction wallet_api::create_free_data_product(string product_name,string brief_desc,string datasource_account,string category_id,double price,string icon,vector<schema_context_object> schema_contexts,string parent_id,string issuer,bool recommend,bool broadcast)
    {
        return my->create_free_data_product(product_name,brief_desc,datasource_account,category_id,price,icon,schema_contexts,parent_id,issuer,recommend,broadcast);
    }

    signed_transaction wallet_api::propose_free_data_product_update(const string &proposing_account,string free_data_product_id,string new_product_name,string new_brief_desc,string new_datasource_account,string new_category_id,double new_price,string new_icon,vector<schema_context_object> new_schema_contexts,string new_parent_id,uint8_t new_status,bool new_recommend,bool broadcast)
    {
        return my->propose_free_data_product_update(proposing_account,free_data_product_id,new_product_name,new_brief_desc,new_datasource_account,new_category_id,new_price,new_icon,new_schema_contexts,new_parent_id,new_status,new_recommend,broadcast);
    }

    signed_transaction wallet_api::create_league_data_product(string product_name,string brief_desc,string category_id,double refer_price,string icon,vector<schema_context_object> schema_contexts,string issuer,uint64_t pocs_threshold, bool broadcast)
    {
        return my->create_league_data_product(product_name,brief_desc,category_id,refer_price,icon,schema_contexts,issuer,pocs_threshold,broadcast);
    }

    signed_transaction wallet_api::propose_league_data_product_update(const string &proposing_account,string league_data_product_id,string new_product_name,string new_brief_desc,string new_category_id,double new_refer_price,string new_icon,vector<schema_context_object> new_schema_contexts,uint8_t new_status,uint64_t new_pocs_threshold,bool broadcast)
    {
        return my->propose_league_data_product_update(proposing_account,league_data_product_id,new_product_name,new_brief_desc,new_category_id,new_refer_price,new_icon,new_schema_contexts,new_status,new_pocs_threshold,broadcast);
    }

    signed_transaction wallet_api::create_league(string league_name, string brief_desc, vector<account_id_type> members, vector <league_data_product_id_type> data_products, vector <uint64_t> prices, string category_id, string icon, string issuer, vector<uint64_t> pocs_thresholds, vector<uint64_t> fee_bases, vector<uint64_t> pocs_weights, bool recommend, bool broadcast)
    {
        return my->create_league(league_name, brief_desc, members, data_products, prices,category_id, icon, issuer, pocs_thresholds, fee_bases, pocs_weights, recommend, broadcast);
    }

    signed_transaction wallet_api::data_transaction_create(string request_id, object_id_type product_id, fc::optional<league_id_type> league_id, string version, string params, string requester, bool broadcast)
    {
        return my->data_transaction_create(request_id, product_id, league_id, version, params, requester, broadcast);
    }

    signed_transaction wallet_api::pay_data_transaction(string from, string to, asset amount, string request_id, bool broadcast = false)
    {
        return my->pay_data_transaction(from, to, amount, request_id, broadcast);
    }

    signed_transaction wallet_api::data_transaction_datasource_validate_error(string request_id, string datasource, bool broadcast)
    {
        return my->data_transaction_datasource_validate_error(request_id, datasource, broadcast);
    }

    signed_transaction wallet_api::data_transaction_complain_datasource(string request_id, account_id_type requester, account_id_type datasource,
        string merchant_copyright_hash, string datasource_copyright_hash, bool broadcast)
    {
        return my->data_transaction_complain_datasource(request_id, requester,datasource, merchant_copyright_hash, datasource_copyright_hash, broadcast);
    }

    signed_transaction wallet_api::data_transaction_datasource_upload(string request_id, string requester, string datasource, fc::optional<string> copyright_hash, bool broadcast)
    {
        return my->data_transaction_datasource_upload(request_id, requester, datasource, copyright_hash, broadcast);
    }

    signed_transaction wallet_api::data_transaction_update(string request_id, uint8_t new_status, string new_requester, fc::optional<string> memo, bool broadcast)
    {
        return my->data_transaction_update(request_id, new_status, new_requester, memo, broadcast);
    }

    data_market_category_object wallet_api::get_data_market_category(string category_id)
    {
        return my->get_data_market_category(category_id);
    }

    free_data_product_object wallet_api::get_free_data_product(string product_id)
    {
        return my->get_free_data_product(product_id);
    }

    league_data_product_object wallet_api::get_league_data_product(string product_id)
    {
        return my->get_league_data_product(product_id);
    }

    league_object wallet_api::get_league(string league_id)
    {
        return my->get_league(league_id);
    }

    vector<data_market_category_object> wallet_api::list_data_market_categories(uint32_t market_type)
    {
        return my->list_data_market_categories(market_type);
    }

    free_data_product_search_results_object wallet_api::list_free_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const
    {
        return my->list_free_data_products(data_market_category_id,offset,limit,order_by,keyword,show_all);
    }

    league_data_product_search_results_object wallet_api::list_league_data_products(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const
    {
        return my->list_league_data_products(data_market_category_id,offset,limit,order_by,keyword,show_all);
    }

    league_search_results_object wallet_api::list_leagues(string data_market_category_id,uint32_t offset,uint32_t limit,string order_by,string keyword,bool show_all) const
    {
        return my->list_leagues(data_market_category_id,offset,limit,order_by,keyword,show_all);
    }

    data_transaction_search_results_object wallet_api::list_data_transactions_by_requester(string requester, uint32_t limit) const
    {
        return my->list_data_transactions_by_requester(requester, limit);
    }

    optional<data_transaction_object> wallet_api::get_data_transaction_by_request_id(string request_id) const
    {
        return my->get_data_transaction_by_request_id(request_id);
    }

    map<account_id_type, uint64_t> wallet_api::list_second_hand_datasources(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, uint32_t limit) const {
        return my->list_second_hand_datasources(start_date_time, end_date_time, limit);
    }

    uint32_t wallet_api::list_total_second_hand_transaction_counts_by_datasource(fc::time_point_sec start_date_time, fc::time_point_sec end_date_time, const string& datasource_account) const
    {
        return my->list_total_second_hand_transaction_counts_by_datasource(start_date_time, end_date_time, datasource_account);
    }

    void wallet_api::get_tps()
    {
        return my->get_tps();
    }

    string wallet_api::get_wallet_filename() const
    {
       return my->get_wallet_filename();
    }

    transaction_handle_type wallet_api::begin_builder_transaction()
    {
       return my->begin_builder_transaction();
    }

    void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
    {
       my->add_operation_to_builder_transaction(transaction_handle, op);
    }

    void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
    {
       my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
    }

    asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset)
    {
       return my->set_fees_on_builder_transaction(handle, fee_asset);
    }

    transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
    {
       return my->preview_builder_transaction(handle);
    }

    signed_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
    {
       return my->sign_builder_transaction(transaction_handle, broadcast);
    }

    pair<transaction_id_type,signed_transaction> wallet_api::broadcast_transaction(signed_transaction tx)
    {
       return my->broadcast_transaction(tx);
    }

    signed_transaction wallet_api::propose_builder_transaction(
       transaction_handle_type handle,
       time_point_sec expiration,
       uint32_t review_period_seconds,
       bool broadcast)
    {
       return my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
    }

    signed_transaction wallet_api::propose_builder_transaction2(
       transaction_handle_type handle,
       string account_name_or_id,
       time_point_sec expiration,
       uint32_t review_period_seconds,
       bool broadcast)
    {
       return my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
    }

    void wallet_api::remove_builder_transaction(transaction_handle_type handle)
    {
       return my->remove_builder_transaction(handle);
    }

    account_object wallet_api::get_account(string account_name_or_id) const
    {
       return my->get_account(account_name_or_id);
    }

    asset_object wallet_api::get_asset(string asset_name_or_id) const
    {
       auto a = my->find_asset(asset_name_or_id);
       FC_ASSERT(a);
       return *a;
    }

    asset_bitasset_data_object wallet_api::get_bitasset_data(string asset_name_or_id) const
    {
       auto asset = get_asset(asset_name_or_id);
       FC_ASSERT(asset.is_market_issued() && asset.bitasset_data_id);
       return my->get_object<asset_bitasset_data_object>(*asset.bitasset_data_id);
    }

    account_id_type wallet_api::get_account_id(string account_name_or_id) const
    {
       return my->get_account_id(account_name_or_id);
    }

    asset_id_type wallet_api::get_asset_id(string asset_symbol_or_id) const
    {
       return my->get_asset_id(asset_symbol_or_id);
    }

    bool wallet_api::import_key(string account_name_or_id, string wif_key)
    {
       FC_ASSERT(!is_locked());
       // backup wallet
       fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
       if (!optional_private_key)
          FC_THROW("Invalid private key");
       string shorthash = detail::address_to_shorthash(optional_private_key->get_public_key());
       copy_wallet_file( "before-import-key-" + shorthash );

       if( my->import_key(account_name_or_id, wif_key) )
       {
          save_wallet_file();
          copy_wallet_file( "after-import-key-" + shorthash );
          return true;
       }
       return false;
    }

    map<string, bool> wallet_api::import_accounts( string filename, string password )
    {
       FC_ASSERT( !is_locked() );
       FC_ASSERT( fc::exists( filename ) );

       const auto imported_keys = fc::json::from_file<exported_keys>( filename );

       const auto password_hash = fc::sha512::hash( password );
       FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

       map<string, bool> result;
       for( const auto& item : imported_keys.account_keys )
       {
           const auto import_this_account = [ & ]() -> bool
           {
               try
               {
                   const account_object account = get_account( item.account_name );
                   const auto& owner_keys = account.owner.get_keys();
                   const auto& active_keys = account.active.get_keys();

                   for( const auto& public_key : item.public_keys )
                   {
                       if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                           return true;

                       if( std::find( active_keys.begin(), active_keys.end(), public_key ) != active_keys.end() )
                           return true;
                   }
               }
               catch( ... )
               {
               }

               return false;
           };

           const auto should_proceed = import_this_account();
           result[ item.account_name ] = should_proceed;

           if( should_proceed )
           {
               uint32_t import_successes = 0;
               uint32_t import_failures = 0;
               // TODO: First check that all private keys match public keys
               for( const auto& encrypted_key : item.encrypted_private_keys )
               {
                   try
                   {
                      const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                      const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                      import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                      ++import_successes;
                   }
                   catch( const fc::exception& e )
                   {
                      elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                      ++import_failures;
                   }
               }
               ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
               if( import_failures > 0 )
                  elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
           }
       }

       return result;
    }

    bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
    {
       FC_ASSERT( !is_locked() );
       FC_ASSERT( fc::exists( filename ) );

       bool is_my_account = false;
       const auto accounts = list_my_accounts();
       for( const auto& account : accounts )
       {
           if( account.name == dest_account_name )
           {
               is_my_account = true;
               break;
           }
       }
       FC_ASSERT( is_my_account );

       const auto imported_keys = fc::json::from_file<exported_keys>( filename );

       const auto password_hash = fc::sha512::hash( password );
       FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

       bool found_account = false;
       for( const auto& item : imported_keys.account_keys )
       {
           if( item.account_name != src_account_name )
               continue;

           found_account = true;

           for( const auto& encrypted_key : item.encrypted_private_keys )
           {
               const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
               const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

               my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
           }

           return true;
       }
       save_wallet_file();

       FC_ASSERT( found_account );

       return false;
    }

    string wallet_api::normalize_brain_key(string s) const
    {
       return detail::normalize_brain_key( s );
    }

    variant wallet_api::info()
    {
       return my->info();
    }

    variant_object wallet_api::about() const
    {
        return my->about();
    }

    fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
    {
       return detail::derive_private_key( prefix_string, sequence_number );
    }

    signed_transaction wallet_api::register_account2(string name,
                                                    public_key_type owner_pubkey,
                                                    public_key_type active_pubkey,
                                                    public_key_type memo,
                                                    string  registrar_account,
                                                    string  referrer_account,
                                                    uint32_t referrer_percent,
                                                    string asset_symbol,
                                                    bool broadcast)
    {
       return my->register_account2( name, owner_pubkey, active_pubkey, memo, registrar_account, referrer_account, referrer_percent, asset_symbol, broadcast );
    }

    signed_transaction wallet_api::deploy_contract(string name,
                                                  string account,
                                                  string vm_type,
                                                  string vm_version,
                                                  string contract_dir,
                                                  string fee_asset_symbol,
                                                  bool broadcast)
    {
        return my->deploy_contract(name, account, vm_type, vm_version, contract_dir, fee_asset_symbol, broadcast);
    }

    signed_transaction wallet_api::call_contract(string account,
                                      string contract,
                                      optional<asset> amount,
                                      string method,
                                      string args,
                                      string fee_asset_symbol,
                                      bool broadcast)
    {
        return my->call_contract(account, contract, amount, method, args, fee_asset_symbol, broadcast);
    }

    variant wallet_api::get_contract_tables(string contract) const
    {
        return my->get_contract_tables(contract);
    }

    variant wallet_api::get_table_objects(string contract, string table) const
    {
        return my->get_table_objects(contract, table);
    }

    signed_transaction wallet_api::register_account(string name,
                                                    public_key_type owner_pubkey,
                                                    public_key_type active_pubkey,
                                                    string  registrar_account,
                                                    string  referrer_account,
                                                    uint32_t referrer_percent,
                                                    bool broadcast)
    {
       return my->register_account( name, owner_pubkey, active_pubkey, registrar_account, referrer_account, referrer_percent, broadcast );
    }
    signed_transaction wallet_api::create_account_with_brain_key(string brain_key, string account_name,
                                                                 string registrar_account, string referrer_account,
                                                                 bool broadcast /* = false */)
    {
       return my->create_account_with_brain_key(
                brain_key, account_name, registrar_account,
                referrer_account, broadcast
                );
    }

    signed_transaction wallet_api::issue_asset(string to_account,
                                                string amount,
                                                string symbol,
                                                string memo,
                                                string fee_asset_symbol,
                                                bool broadcast)
    {
       return my->issue_asset(to_account, amount, symbol, memo, fee_asset_symbol, broadcast);
    }

    signed_transaction wallet_api::transfer(string from, string to, string amount,
                                            string asset_symbol, string memo, bool broadcast /* = false */)
    {
       return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
    }

    pair<graphene::chain::transaction_id_type,signed_transaction> wallet_api::transfer3(string from,
                                                                                        string to,
                                                                                        string amount,
                                                                                        string asset_symbol,
                                                                                        string memo,
                                                                                        string fee_asset_symbol,
                                                                                        bool broadcast)
    {
       return my->transfer3(from, to, amount, asset_symbol, memo, fee_asset_symbol, broadcast);
    }

    signed_transaction wallet_api::create_asset(string issuer,
                                                string symbol,
                                                uint8_t precision,
                                                asset_options common,
                                                string fee_asset_symbol,
                                                bool broadcast)

    {
       return my->create_asset(issuer, symbol, precision, common, fee_asset_symbol, broadcast);
    }

    signed_transaction wallet_api::update_asset(string symbol,
                                                optional<string> new_issuer,
                                                asset_options new_options,
                                                string fee_asset_symbol,
                                                bool broadcast /* = false */)
    {
       return my->update_asset(symbol, new_issuer, new_options, fee_asset_symbol, broadcast);
    }

    signed_transaction wallet_api::update_bitasset(string symbol,
                                                   bitasset_options new_options,
                                                   bool broadcast /* = false */)
    {
       return my->update_bitasset(symbol, new_options, broadcast);
    }

    signed_transaction wallet_api::update_asset_feed_producers(string symbol,
                                                               flat_set<string> new_feed_producers,
                                                               bool broadcast /* = false */)
    {
       return my->update_asset_feed_producers(symbol, new_feed_producers, broadcast);
    }

    signed_transaction wallet_api::publish_asset_feed(string publishing_account,
                                                      string symbol,
                                                      price_feed feed,
                                                      bool broadcast /* = false */)
    {
       return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
    }

    signed_transaction wallet_api::fund_asset_fee_pool(string from,
                                                       string symbol,
                                                       string amount,
                                                       bool broadcast /* = false */)
    {
       return my->fund_asset_fee_pool(from, symbol, amount, broadcast);
    }

    signed_transaction wallet_api::reserve_asset(string from,
                                              string amount,
                                              string symbol,
                                              bool broadcast /* = false */)
    {
       return my->reserve_asset(from, amount, symbol, broadcast);
    }

    signed_transaction wallet_api::global_settle_asset(string symbol,
                                                       price settle_price,
                                                       bool broadcast /* = false */)
    {
       return my->global_settle_asset(symbol, settle_price, broadcast);
    }

    signed_transaction wallet_api::settle_asset(string account_to_settle,
                                                string amount_to_settle,
                                                string symbol,
                                                bool broadcast /* = false */)
    {
       return my->settle_asset(account_to_settle, amount_to_settle, symbol, broadcast);
    }

    signed_transaction wallet_api::whitelist_account(string authorizing_account,
                                                     string account_to_list,
                                                     account_whitelist_operation::account_listing new_listing_status,
                                                     bool broadcast /* = false */)
    {
       return my->whitelist_account(authorizing_account, account_to_list, new_listing_status, broadcast);
    }

    signed_transaction wallet_api::create_committee_member(string owner_account, string url,
                                                   bool broadcast /* = false */)
    {
       return my->create_committee_member(owner_account, url, broadcast);
    }

    map<string,witness_id_type> wallet_api::list_witnesses(const string& lowerbound, uint32_t limit)
    {
       return my->_remote_db->lookup_witness_accounts(lowerbound, limit);
    }

    map<string,committee_member_id_type> wallet_api::list_committee_members(const string& lowerbound, uint32_t limit)
    {
       return my->_remote_db->lookup_committee_member_accounts(lowerbound, limit);
    }

    witness_object wallet_api::get_witness(string owner_account)
    {
       return my->get_witness(owner_account);
    }

    committee_member_object wallet_api::get_committee_member(string owner_account)
    {
       return my->get_committee_member(owner_account);
    }

    signed_transaction wallet_api::create_witness(string owner_account,
                                                  string url,
                                                  string asset_symbol,
                                                  bool broadcast /* = false */)
    {
       return my->create_witness(owner_account, url, asset_symbol, broadcast);
    }

    signed_transaction wallet_api::create_worker(
       string owner_account,
       time_point_sec work_begin_date,
       time_point_sec work_end_date,
       share_type daily_pay,
       string name,
       string url,
       variant worker_settings,
       bool broadcast /* = false */)
    {
       return my->create_worker( owner_account, work_begin_date, work_end_date,
          daily_pay, name, url, worker_settings, broadcast );
    }

    signed_transaction wallet_api::update_worker_votes(
       string owner_account,
       worker_vote_delta delta,
       bool broadcast /* = false */)
    {
       return my->update_worker_votes( owner_account, delta, broadcast );
    }

    signed_transaction wallet_api::update_witness(
       string witness_name,
       string url,
       string block_signing_key,
       string asset_symbol,
       bool broadcast /* = false */)
    {
       return my->update_witness(witness_name, url, block_signing_key, asset_symbol, broadcast);
    }

    vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( string account_name )
    {
       return my->get_vesting_balances( account_name );
    }

    signed_transaction wallet_api::withdraw_vesting(
       string witness_name,
       string amount,
       string asset_symbol,
       bool broadcast /* = false */)
    {
       return my->withdraw_vesting( witness_name, amount, asset_symbol, broadcast );
    }

    signed_transaction wallet_api::update_account_multisig_keys(string account,
                                                           string type,
                                                           uint32_t weight_threshold,
                                                           vector<public_key_type> key_auths,
                                                           vector<weight_type> key_weights,
                                                           string fee_symbol,
                                                           bool broadcast)
    {
      return my->update_account_multisig_keys(account, type, weight_threshold, key_auths, key_weights, fee_symbol, broadcast);
    }

    signed_transaction wallet_api::update_account_multisig(string account,
                                                           string type,
                                                           uint32_t weight_threshold,
                                                           vector<string> account_auths,
                                                           vector<weight_type> account_weights,
                                                           string fee_symbol,
                                                           bool broadcast)
    {
      return my->update_account_multisig(account, type, weight_threshold, account_auths, account_weights, fee_symbol, broadcast);
    }

    signed_transaction wallet_api::vote_for_committee_member(string voting_account,
                                                     string witness,
                                                     bool approve,
                                                     bool broadcast /* = false */)
    {
       return my->vote_for_committee_member(voting_account, witness, approve, broadcast);
    }

    signed_transaction wallet_api::vote_for_witness(string voting_account,
                                                    string witness,
                                                    bool approve,
                                                    bool broadcast /* = false */)
    {
       return my->vote_for_witness(voting_account, witness, approve, broadcast);
    }

    signed_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                    optional<string> voting_account,
                                                    bool broadcast /* = false */)
    {
       return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
    }

    signed_transaction wallet_api::set_desired_witness_and_committee_member_count(string account_to_modify,
                                                                          uint16_t desired_number_of_witnesses,
                                                                          uint16_t desired_number_of_committee_members,
                                                                          bool broadcast /* = false */)
    {
       return my->set_desired_witness_and_committee_member_count(account_to_modify, desired_number_of_witnesses,
                                                         desired_number_of_committee_members, broadcast);
    }

    void wallet_api::set_wallet_filename(string wallet_filename)
    {
       my->_wallet_filename = wallet_filename;
    }

    signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
    { try {
       return my->sign_transaction( tx, broadcast);
    } FC_CAPTURE_AND_RETHROW( (tx) ) }

    operation wallet_api::get_prototype_operation(string operation_name)
    {
       return my->get_prototype_operation( operation_name );
    }

    void wallet_api::dbg_make_uia(string creator, string symbol)
    {
       FC_ASSERT(!is_locked());
       my->dbg_make_uia(creator, symbol);
    }

    void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
    {
       my->dbg_push_blocks( src_filename, count );
    }

    void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
    {
       my->dbg_generate_blocks( debug_wif_key, count );
    }

    void wallet_api::dbg_stream_json_objects( const std::string& filename )
    {
       my->dbg_stream_json_objects( filename );
    }

    void wallet_api::dbg_update_object( fc::variant_object update )
    {
       my->dbg_update_object( update );
    }

    void wallet_api::network_add_nodes( const vector<string>& nodes )
    {
       my->network_add_nodes( nodes );
    }

    vector< variant > wallet_api::network_get_connected_peers()
    {
       return my->network_get_connected_peers();
    }

    void wallet_api::flood_create_account(string account_prefix, uint32_t number_of_accounts)
    {
       FC_ASSERT(!is_locked());
       my->flood_create_account(account_prefix, number_of_accounts);
    }

    void wallet_api::flood_transfer(string from_account, string account_prefix, uint32_t number_of_accounts, uint32_t number_of_loop)
    {
       FC_ASSERT(!is_locked());
       my->flood_transfer(from_account, account_prefix, number_of_accounts, number_of_loop);
    }

    void wallet_api::transfer_test(account_id_type from_account, account_id_type to_account, uint32_t times)
    {
        my->transfer_test(from_account, to_account, times);
    }

    fc::sha256 wallet_api::get_hash(const string& value)
    {
        return my->get_hash(value);
    }

    signature_type wallet_api::sign_string(string wif_key, const string &raw_string)
    {
        return my->sign_string(wif_key, raw_string);
    }

    bool wallet_api::verify_transaction_signature(const signed_transaction& trx, public_key_type pub_key)
    {
        return my->verify_transaction_signature(trx, pub_key);
    }

    bool wallet_api::verify_proxy_transfer_signature(const proxy_transfer_params& param, public_key_type pub_key)
    {
        return my->verify_proxy_transfer_signature(param, pub_key);
    }

    signed_transaction wallet_api::custom(string account, uint16_t id, string data, string fee_symbol, bool broadcast)
    {
        return my->custom(account, id, data, fee_symbol, broadcast);
    }

    void wallet_api::flood_network(string account_prefix, uint32_t number_of_transactions)
    {
       FC_ASSERT(!is_locked());
       my->flood_network(account_prefix, number_of_transactions);
    }

    signed_transaction wallet_api::propose_league_update(
          const string& proposing_account,
          string league_id,
          string new_league_name,
          string new_brief_desc,
          vector<account_id_type> new_members,
          vector <league_data_product_id_type> new_data_products,
          vector <uint64_t> new_prices,
          string new_category_id,
          string new_icon,
          uint8_t new_status,
          vector<uint64_t> new_pocs_thresholds,
          vector<uint64_t> new_fee_bases,
          vector<uint64_t> new_pocs_weights,
          bool new_recommend,
          bool broadcast
          )
    {
        return my->propose_league_update(proposing_account ,league_id ,new_league_name ,new_brief_desc ,new_members, new_data_products ,new_prices ,new_category_id ,new_icon ,new_status ,new_pocs_thresholds, new_fee_bases, new_pocs_weights, new_recommend ,broadcast);
    }

    signed_transaction wallet_api::propose_parameter_change(
       const string& proposing_account,
       fc::time_point_sec expiration_time,
       const variant_object& changed_values,
       bool broadcast /* = false */
       )
    {
       return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
    }

    signed_transaction wallet_api::propose_gpo_extensions_change(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        const variant_object& changed_extensions,
        bool broadcast /*= false*/
        )
    {
        return my->propose_gpo_extensions_change(proposing_account, expiration_time, changed_extensions, broadcast);
    }

    signed_transaction wallet_api::propose_fee_change(
       const string& proposing_account,
       fc::time_point_sec expiration_time,
       const variant_object& changed_fees,
       bool broadcast /* = false */
       )
    {
       return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
    }

    signed_transaction wallet_api::propose_deprecate_datasource(string propose_account, string datasource_account, bool broadcast /*= false*/)
    {
        return my->propose_deprecate_datasource(propose_account, datasource_account, broadcast);
    }

    signed_transaction wallet_api::propose_deprecate_merchant(string propose_account, string merchant_account, bool broadcast/* = false*/)
    {
        return my->propose_deprecate_merchant(propose_account, merchant_account, broadcast);
    }

    signed_transaction wallet_api::approve_proposal(
       const string& fee_paying_account,
       const string& proposal_id,
       const approval_delta& delta,
       bool broadcast /* = false */
       )
    {
       return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
    }

    global_property_object wallet_api::get_global_properties() const
    {
       return my->get_global_properties();
    }

    dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
    {
       return my->get_dynamic_global_properties();
    }

    data_transaction_commission_percent_t wallet_api::get_commission_percent() const
    {
        return my->get_commission_percent();
    }

    string wallet_api::help()const
    {
       std::vector<std::string> method_names = my->method_documentation.get_method_names();
       std::stringstream ss;
       for (const std::string method_name : method_names)
       {
          try
          {
             ss << my->method_documentation.get_brief_description(method_name);
          }
          catch (const fc::key_not_found_exception&)
          {
             ss << method_name << " (no help available)\n";
          }
       }
       return ss.str();
    }

    string wallet_api::gethelp(const string& method)const
    {
       fc::api<wallet_api> tmp;
       std::stringstream ss;
       ss << "\n";

       if( method == "import_key" )
       {
          ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
          ss << "example: import_key \"1.3.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
          ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
       }
       else if( method == "transfer" )
       {
          ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
          ss << "example: transfer \"1.3.11\" \"1.3.4\" 1000.03 CORE \"memo\" true\n";
          ss << "example: transfer \"usera\" \"userb\" 1000.123 CORE \"memo\" true\n";
       }
       else if( method == "create_account_with_brain_key" )
       {
          ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR REFERRER BROADCAST\n\n";
          ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"1.3.11\" \"1.3.11\" true\n";
          ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"someaccount\" \"otheraccount\" true\n";
          ss << "\n";
          ss << "This method should be used if you would like the wallet to generate new keys derived from the brain key.\n";
          ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  Use\n";
          ss << "register_account if you already know the keys you know the public keys that you would like to register.\n";

       }
       else if( method == "register_account2" )
       {
          ss << "usage: register_account2 ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY MEMO_KEY REGISTRAR REFERRER REFERRER_PERCENT ASSET_SYMBOL BROADCAST\n\n";
          ss << "example: register_account2 \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" \"1.3.11\" 10 asset_symbol true\n";
          ss << "\n";
          ss << "Use this method to register an account for which you do not know the private keys.";
       }
       else if( method == "register_account" )
       {
          ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR REFERRER REFERRER_PERCENT BROADCAST\n\n";
          ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" \"1.3.11\" 50 true\n";
          ss << "\n";
          ss << "Use this method to register an account for which you do not know the private keys.";
       }
       else if (method == "create_asset")
       {
           ss << "usage: ISSUER SYMBOL PRECISION_DIGITS OPTIONS BITASSET_OPTIONS FEE_ASSET_SYMBOL BROADCAST\n\n";
           ss << "PRECISION_DIGITS: the number of digits after the decimal point\n\n";
           ss << "Example value of OPTIONS: \n";
           ss << fc::json::to_pretty_string(graphene::chain::asset_options());
           ss << "\nBITASSET_OPTIONS may be null\n";
       }
       else
       {
          std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
          if (!doxygenHelpString.empty())
             ss << doxygenHelpString;
          else
             ss << "No help defined for method " << method << "\n";
       }

       return ss.str();
    }

    bool wallet_api::load_wallet_file( string wallet_filename )
    {
       return my->load_wallet_file( wallet_filename );
    }

    void wallet_api::save_wallet_file( string wallet_filename )
    {
       my->save_wallet_file( wallet_filename );
    }

    std::map<string,std::function<string(fc::variant,const fc::variants&)> >
    wallet_api::get_result_formatters() const
    {
       return my->get_result_formatters();
    }

    bool wallet_api::is_locked()const
    {
       return my->is_locked();
    }
    bool wallet_api::is_new()const
    {
       return my->_wallet.cipher_keys.size() == 0;
    }

    void wallet_api::encrypt_keys()
    {
       my->encrypt_keys();
    }

    void wallet_api::lock()
    { try {
       FC_ASSERT( !is_locked() );
       encrypt_keys();
       for( auto key : my->_keys )
          key.second = key_to_wif(fc::ecc::private_key());
       my->_keys.clear();
       my->_checksum = fc::sha512();
       my->self.lock_changed(true);
    } FC_CAPTURE_AND_RETHROW() }

    void wallet_api::unlock(string password)
    { try {
       FC_ASSERT(password.size() > 0);
       auto pw = fc::sha512::hash(password.c_str(), password.size());
       vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
       auto pk = fc::raw::unpack<plain_keys>(decrypted);
       FC_ASSERT(pk.checksum == pw);
       my->_keys = std::move(pk.keys);
       my->_checksum = pk.checksum;
       my->self.lock_changed(false);
    } FC_CAPTURE_AND_RETHROW() }

    void wallet_api::set_password( string password )
    {
       if( !is_new() )
          FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
       my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
       lock();
    }

    vector< signed_transaction > wallet_api::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
    {
       return my->import_balance( name_or_id, wif_keys, broadcast );
    }

    namespace detail {

    vector< signed_transaction > wallet_api_impl::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
    { try {
       FC_ASSERT(!is_locked());
       const dynamic_global_property_object& dpo = _remote_db->get_dynamic_global_properties();
       account_object claimer = get_account( name_or_id );
       uint32_t max_ops_per_tx = 30;

       map< address, private_key_type > keys;  // local index of address -> private key
       vector< address > addrs;
       bool has_wildcard = false;
       addrs.reserve( wif_keys.size() );
       for( const string& wif_key : wif_keys )
       {
          if( wif_key == "*" )
          {
             if( has_wildcard )
                continue;
             for( const public_key_type& pub : _wallet.extra_keys[ claimer.id ] )
             {
                addrs.push_back( pub );
                auto it = _keys.find( pub );
                if( it != _keys.end() )
                {
                   fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
                   FC_ASSERT( privkey );
                   keys[ addrs.back() ] = *privkey;
                }
                else
                {
                   wlog( "Somehow _keys has no private key for extra_keys public key ${k}", ("k", pub) );
                }
             }
             has_wildcard = true;
          }
          else
          {
             optional< private_key_type > key = wif_to_key( wif_key );
             FC_ASSERT( key.valid(), "Invalid private key" );
             fc::ecc::public_key pk = key->get_public_key();
             addrs.push_back( pk );
             keys[addrs.back()] = *key;
             // see chain/balance_evaluator.cpp
             addrs.push_back( pts_address( pk, false, 56 ) );
             keys[addrs.back()] = *key;
             addrs.push_back( pts_address( pk, true, 56 ) );
             keys[addrs.back()] = *key;
             addrs.push_back( pts_address( pk, false, 0 ) );
             keys[addrs.back()] = *key;
             addrs.push_back( pts_address( pk, true, 0 ) );
             keys[addrs.back()] = *key;
          }
       }

       vector< balance_object > balances = _remote_db->get_balance_objects( addrs );
       wdump((balances));
       addrs.clear();

       set<asset_id_type> bal_types;
       for( auto b : balances ) bal_types.insert( b.balance.asset_id );

       struct claim_tx
       {
          vector< balance_claim_operation > ops;
          set< address > addrs;
       };
       vector< claim_tx > claim_txs;

       for( const asset_id_type& a : bal_types )
       {
          balance_claim_operation op;
          op.deposit_to_account = claimer.id;
          for( const balance_object& b : balances )
          {
             if( b.balance.asset_id == a )
             {
                op.total_claimed = b.available( dpo.time );
                if( op.total_claimed.amount == 0 )
                   continue;
                op.balance_to_claim = b.id;
                op.balance_owner_key = keys[b.owner].get_public_key();
                if( (claim_txs.empty()) || (claim_txs.back().ops.size() >= max_ops_per_tx) )
                   claim_txs.emplace_back();
                claim_txs.back().ops.push_back(op);
                claim_txs.back().addrs.insert(b.owner);
             }
          }
       }

       vector< signed_transaction > result;

       for( const claim_tx& ctx : claim_txs )
       {
          signed_transaction tx;
          tx.operations.reserve( ctx.ops.size() );
          for( const balance_claim_operation& op : ctx.ops )
             tx.operations.emplace_back( op );
          set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
          tx.validate();
          signed_transaction signed_tx = sign_transaction( tx, false );
          for( const address& addr : ctx.addrs )
             signed_tx.sign( keys[addr], _chain_id );
          // if the key for a balance object was the same as a key for the account we're importing it into,
          // we may end up with duplicate signatures, so remove those
          boost::erase(signed_tx.signatures, boost::unique<boost::return_found_end>(boost::sort(signed_tx.signatures)));
          result.push_back( signed_tx );
          if( broadcast )
             _remote_net_broadcast->broadcast_transaction(signed_tx);
       }

       return result;
    } FC_CAPTURE_AND_RETHROW( (name_or_id) ) }

    }

    map<public_key_type, string> wallet_api::dump_private_keys()
    {
       FC_ASSERT(!is_locked());
       return my->_keys;
    }

    signed_transaction wallet_api::upgrade_account( string name, string asset_symbol, bool broadcast )
    {
       return my->upgrade_account(name, asset_symbol, broadcast);
    }

    signed_transaction wallet_api::upgrade_data_transaction_member(string name, string auth_referrer, bool broadcast)
    {
        return my->upgrade_data_transaction_member(name, auth_referrer, broadcast);
    }

    /**
     * @brief upgrade_account_merchant
     * @param name
     * @param auth_referrer
     * @param broadcast
     * @return
     */
    signed_transaction wallet_api::upgrade_account_merchant(string name,string auth_referrer, bool broadcast)
    {
       return my->upgrade_account_merchant(name,auth_referrer,broadcast);
    }
    /**
     * @brief upgrade_account_datasource
     * @param name
     * @param auth_referrer
     * @param broadcast
     * @return
     */
    signed_transaction wallet_api::upgrade_account_datasource(string name,string auth_referrer, bool broadcast)
    {
       return my->upgrade_account_datasource(name,auth_referrer,broadcast);
    }

    string wallet_api::get_key_label( public_key_type key )const
    {
       auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
       if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
          return key_itr->label;
       return string();
    }

    string wallet_api::get_private_key( public_key_type pubkey )const
    {
       return key_to_wif( my->get_private_key( pubkey ) );
    }

    public_key_type  wallet_api::get_public_key( string label )const
    {
       try { return fc::variant(label).as<public_key_type>(); } catch ( ... ){}

       auto key_itr   = my->_wallet.labeled_keys.get<by_label>().find(label);
       if( key_itr != my->_wallet.labeled_keys.get<by_label>().end() )
          return key_itr->key;
       return public_key_type();
    }

    bool               wallet_api::set_key_label( public_key_type key, string label )
    {
       auto result = my->_wallet.labeled_keys.insert( key_label{label,key} );
       if( result.second  ) return true;

       auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
       auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
       if( label_itr == my->_wallet.labeled_keys.get<by_label>().end() )
       {
          if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
             return my->_wallet.labeled_keys.get<by_key>().modify( key_itr, [&]( key_label& obj ){ obj.label = label; } );
       }
       return false;
    }
    map<string,public_key_type> wallet_api::get_blind_accounts()const
    {
       map<string,public_key_type> result;
       for( const auto& item : my->_wallet.labeled_keys )
          result[item.label] = item.key;
       return result;
    }
    map<string,public_key_type> wallet_api::get_my_blind_accounts()const
    {
       FC_ASSERT( !is_locked() );
       map<string,public_key_type> result;
       for( const auto& item : my->_wallet.labeled_keys )
       {
          if( my->_keys.find(item.key) != my->_keys.end() )
             result[item.label] = item.key;
       }
       return result;
    }

    public_key_type    wallet_api::create_blind_account( string label, string brain_key  )
    {
       FC_ASSERT( !is_locked() );

       auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
       if( label_itr !=  my->_wallet.labeled_keys.get<by_label>().end() )
          FC_ASSERT( !"Key with label already exists" );
       brain_key = fc::trim_and_normalize_spaces( brain_key );
       auto secret = fc::sha256::hash( brain_key.c_str(), brain_key.size() );
       auto priv_key = fc::ecc::private_key::regenerate( secret );
       public_key_type pub_key  = priv_key.get_public_key();

       FC_ASSERT( set_key_label( pub_key, label ) );

       my->_keys[pub_key] = graphene::utilities::key_to_wif( priv_key );

       save_wallet_file();
       return pub_key;
    }

    vector<asset>   wallet_api::get_blind_balances( string key_or_label )
    {
       vector<asset> result;
       map<asset_id_type, share_type> balances;

       vector<commitment_type> used;

       auto pub_key = get_public_key( key_or_label );
       auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
       auto start = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(0),false)  );
       auto end = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(uint32_t(0xffffffff)),true)  );
       while( start != end )
       {
          if( !start->used  )
          {
             auto answer = my->_remote_db->get_blinded_balances( {start->commitment()} );
             if( answer.size() )
                balances[start->amount.asset_id] += start->amount.amount;
             else
                used.push_back( start->commitment() );
          }
          ++start;
       }
       for( const auto& u : used )
       {
          auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
          my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
       }
       for( auto item : balances )
          result.push_back( asset( item.second, item.first ) );
       return result;
    }

    blind_confirmation wallet_api::transfer_from_blind( string from_blind_account_key_or_label,
                                                        string to_account_id_or_name,
                                                        string amount_in,
                                                        string symbol,
                                                        bool broadcast )
    { try {
       transfer_from_blind_operation from_blind;


       auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;
       fc::optional<asset_object> asset_obj = get_asset(symbol);
       FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));
       auto amount = asset_obj->amount_from_string(amount_in);

       from_blind.fee  = fees->calculate_fee( from_blind, asset_obj->options.core_exchange_rate );

       auto blind_in = asset_obj->amount_to_string( from_blind.fee + amount );


       auto conf = blind_transfer_help( from_blind_account_key_or_label,
                                   from_blind_account_key_or_label,
                                   blind_in, symbol, false, true/*to_temp*/ );
       FC_ASSERT( conf.outputs.size() > 0 );

       auto to_account = my->get_account( to_account_id_or_name );
       from_blind.to = to_account.id;
       from_blind.amount = amount;
       from_blind.blinding_factor = conf.outputs.back().decrypted_memo.blinding_factor;
       from_blind.inputs.push_back( {conf.outputs.back().decrypted_memo.commitment, authority() } );
       from_blind.fee  = fees->calculate_fee( from_blind, asset_obj->options.core_exchange_rate );

       idump( (from_blind) );
       conf.trx.operations.push_back(from_blind);
       ilog( "about to validate" );
       conf.trx.validate();

       ilog( "about to broadcast" );
       conf.trx = sign_transaction( conf.trx, broadcast );

       if( broadcast && conf.outputs.size() == 2 ) {

           // Save the change
           blind_confirmation::output conf_output;
           blind_confirmation::output change_output = conf.outputs[0];

           // The wallet must have a private key for confirmation.to, this is used to decrypt the memo
           public_key_type from_key = get_public_key(from_blind_account_key_or_label);
           conf_output.confirmation.to = from_key;
           conf_output.confirmation.one_time_key = change_output.confirmation.one_time_key;
           conf_output.confirmation.encrypted_memo = change_output.confirmation.encrypted_memo;
           conf_output.confirmation_receipt = conf_output.confirmation;
           //try {
           receive_blind_transfer( conf_output.confirmation_receipt, from_blind_account_key_or_label, "@"+to_account.name );
           //} catch ( ... ){}
       }

       return conf;
    } FC_CAPTURE_AND_RETHROW( (from_blind_account_key_or_label)(to_account_id_or_name)(amount_in)(symbol) ) }

    blind_confirmation wallet_api::blind_transfer( string from_key_or_label,
                                                   string to_key_or_label,
                                                   string amount_in,
                                                   string symbol,
                                                   bool broadcast )
    {
       return blind_transfer_help( from_key_or_label, to_key_or_label, amount_in, symbol, broadcast, false );
    }
    blind_confirmation wallet_api::blind_transfer_help( string from_key_or_label,
                                                   string to_key_or_label,
                                                   string amount_in,
                                                   string symbol,
                                                   bool broadcast,
                                                   bool to_temp )
    {
       blind_confirmation confirm;
       try {

       FC_ASSERT( !is_locked() );
       public_key_type from_key = get_public_key(from_key_or_label);
       public_key_type to_key   = get_public_key(to_key_or_label);

       fc::optional<asset_object> asset_obj = get_asset(symbol);
       FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));

       blind_transfer_operation blind_tr;
       blind_tr.outputs.resize(2);

       auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;

       auto amount = asset_obj->amount_from_string(amount_in);

       asset total_amount = asset_obj->amount(0);

       vector<fc::sha256> blinding_factors;

       //auto from_priv_key = my->get_private_key( from_key );

       blind_tr.fee  = fees->calculate_fee( blind_tr, asset_obj->options.core_exchange_rate );

       vector<commitment_type> used;

       auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
       auto start = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,false)  );
       auto end = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,true)  );
       while( start != end )
       {
          auto result = my->_remote_db->get_blinded_balances( {start->commitment() } );
          if( result.size() == 0 )
          {
             used.push_back( start->commitment() );
          }
          else
          {
             blind_tr.inputs.push_back({start->commitment(), start->control_authority});
             blinding_factors.push_back( start->data.blinding_factor );
             total_amount += start->amount;

             if( total_amount >= amount + blind_tr.fee )
                break;
          }
          ++start;
       }
       for( const auto& u : used )
       {
          auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
          my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
       }

       FC_ASSERT( total_amount >= amount+blind_tr.fee, "Insufficent Balance", ("available",total_amount)("amount",amount)("fee",blind_tr.fee) );

       auto one_time_key = fc::ecc::private_key::generate();
       auto secret       = one_time_key.get_shared_secret( to_key );
       auto child        = fc::sha256::hash( secret );
       auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
       auto blind_factor = fc::sha256::hash( child );

       auto from_secret  = one_time_key.get_shared_secret( from_key );
       auto from_child   = fc::sha256::hash( from_secret );
       auto from_nonce   = fc::sha256::hash( nonce );

       auto change = total_amount - amount - blind_tr.fee;
       fc::sha256 change_blind_factor;
       fc::sha256 to_blind_factor;
       if( change.amount > 0 )
       {
          idump(("to_blind_factor")(blind_factor) );
          blinding_factors.push_back( blind_factor );
          change_blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() - 1 );
          wdump(("change_blind_factor")(change_blind_factor) );
       }
       else // change == 0
       {
          blind_tr.outputs.resize(1);
          blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );
          idump(("to_sum_blind_factor")(blind_factor) );
          blinding_factors.push_back( blind_factor );
          idump(("nochange to_blind_factor")(blind_factor) );
       }
       fc::ecc::public_key from_pub_key = from_key;
       fc::ecc::public_key to_pub_key = to_key;

       blind_output to_out;
       to_out.owner       = to_temp ? authority() : authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
       to_out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
       idump(("to_out.blind")(blind_factor)(to_out.commitment) );


       if( blind_tr.outputs.size() > 1 )
       {
          to_out.range_proof = fc::ecc::range_proof_sign( 0, to_out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );

          blind_output change_out;
          change_out.owner       = authority( 1, public_key_type( from_pub_key.child( from_child ) ), 1 );
          change_out.commitment  = fc::ecc::blind( change_blind_factor, change.amount.value );
          change_out.range_proof = fc::ecc::range_proof_sign( 0, change_out.commitment, change_blind_factor, from_nonce,  0, 0, change.amount.value );
          blind_tr.outputs[1] = change_out;


          blind_confirmation::output conf_output;
          conf_output.label = from_key_or_label;
          conf_output.pub_key = from_key;
          conf_output.decrypted_memo.from = from_key;
          conf_output.decrypted_memo.amount = change;
          conf_output.decrypted_memo.blinding_factor = change_blind_factor;
          conf_output.decrypted_memo.commitment = change_out.commitment;
          conf_output.decrypted_memo.check   = from_secret._hash[0];
          conf_output.confirmation.one_time_key = one_time_key.get_public_key();
          conf_output.confirmation.to = from_key;
          conf_output.confirmation.encrypted_memo = fc::aes_encrypt( from_secret, fc::raw::pack( conf_output.decrypted_memo ) );
          conf_output.auth = change_out.owner;
          conf_output.confirmation_receipt = conf_output.confirmation;

          confirm.outputs.push_back( conf_output );
       }
       blind_tr.outputs[0] = to_out;

       blind_confirmation::output conf_output;
       conf_output.label = to_key_or_label;
       conf_output.pub_key = to_key;
       conf_output.decrypted_memo.from = from_key;
       conf_output.decrypted_memo.amount = amount;
       conf_output.decrypted_memo.blinding_factor = blind_factor;
       conf_output.decrypted_memo.commitment = to_out.commitment;
       conf_output.decrypted_memo.check   = secret._hash[0];
       conf_output.confirmation.one_time_key = one_time_key.get_public_key();
       conf_output.confirmation.to = to_key;
       conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
       conf_output.auth = to_out.owner;
       conf_output.confirmation_receipt = conf_output.confirmation;

       confirm.outputs.push_back( conf_output );

       /** commitments must be in sorted order */
       std::sort( blind_tr.outputs.begin(), blind_tr.outputs.end(),
                  [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );
       std::sort( blind_tr.inputs.begin(), blind_tr.inputs.end(),
                  [&]( const blind_input& a, const blind_input& b ){ return a.commitment < b.commitment; } );

       confirm.trx.operations.emplace_back( std::move(blind_tr) );
       ilog( "validate before" );
       confirm.trx.validate();
       confirm.trx = sign_transaction(confirm.trx, broadcast);

       if( broadcast )
       {
          for( const auto& out : confirm.outputs )
          {
             try { receive_blind_transfer( out.confirmation_receipt, from_key_or_label, "" ); } catch ( ... ){}
          }
       }

       return confirm;
    } FC_CAPTURE_AND_RETHROW( (from_key_or_label)(to_key_or_label)(amount_in)(symbol)(broadcast)(confirm) ) }



    /**
     *  Transfers a public balance from @from to one or more blinded balances using a
     *  stealth transfer.
     */
    blind_confirmation wallet_api::transfer_to_blind( string from_account_id_or_name,
                                                      string asset_symbol,
                                                      /** map from key or label to amount */
                                                      vector<pair<string, string>> to_amounts,
                                                      bool broadcast )
    { try {
       FC_ASSERT( !is_locked() );
       idump((to_amounts));

       blind_confirmation confirm;
       account_object from_account = my->get_account(from_account_id_or_name);

       fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
       FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

       transfer_to_blind_operation bop;
       bop.from   = from_account.id;

       vector<fc::sha256> blinding_factors;

       asset total_amount = asset_obj->amount(0);

       for( auto item : to_amounts )
       {
          auto one_time_key = fc::ecc::private_key::generate();
          auto to_key       = get_public_key( item.first );
          auto secret       = one_time_key.get_shared_secret( to_key );
          auto child        = fc::sha256::hash( secret );
          auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
          auto blind_factor = fc::sha256::hash( child );

          blinding_factors.push_back( blind_factor );

          auto amount = asset_obj->amount_from_string(item.second);
          total_amount += amount;


          fc::ecc::public_key to_pub_key = to_key;
          blind_output out;
          out.owner       = authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
          out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
          if( to_amounts.size() > 1 )
             out.range_proof = fc::ecc::range_proof_sign( 0, out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );


          blind_confirmation::output conf_output;
          conf_output.label = item.first;
          conf_output.pub_key = to_key;
          conf_output.decrypted_memo.amount = amount;
          conf_output.decrypted_memo.blinding_factor = blind_factor;
          conf_output.decrypted_memo.commitment = out.commitment;
          conf_output.decrypted_memo.check   = secret._hash[0];
          conf_output.confirmation.one_time_key = one_time_key.get_public_key();
          conf_output.confirmation.to = to_key;
          conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
          conf_output.confirmation_receipt = conf_output.confirmation;

          confirm.outputs.push_back( conf_output );

          bop.outputs.push_back(out);
       }
       bop.amount          = total_amount;
       bop.blinding_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );

       /** commitments must be in sorted order */
       std::sort( bop.outputs.begin(), bop.outputs.end(),
                  [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );

       confirm.trx.operations.push_back( bop );
       my->set_operation_fees( confirm.trx, my->_remote_db->get_global_properties().parameters.current_fees);
       confirm.trx.validate();
       confirm.trx = sign_transaction(confirm.trx, broadcast);

       if( broadcast )
       {
          for( const auto& out : confirm.outputs )
          {
             try { receive_blind_transfer( out.confirmation_receipt, "@"+from_account.name, "from @"+from_account.name ); } catch ( ... ){}
          }
       }

       return confirm;
    } FC_CAPTURE_AND_RETHROW( (from_account_id_or_name)(asset_symbol)(to_amounts) ) }

    blind_receipt wallet_api::receive_blind_transfer( string confirmation_receipt, string opt_from, string opt_memo )
    {
       FC_ASSERT( !is_locked() );
       stealth_confirmation conf(confirmation_receipt);
       FC_ASSERT( conf.to );

       blind_receipt result;
       result.conf = conf;

       auto to_priv_key_itr = my->_keys.find( *conf.to );
       FC_ASSERT( to_priv_key_itr != my->_keys.end(), "No private key for receiver", ("conf",conf) );


       auto to_priv_key = wif_to_key( to_priv_key_itr->second );
       FC_ASSERT( to_priv_key );

       auto secret       = to_priv_key->get_shared_secret( conf.one_time_key );
       auto child        = fc::sha256::hash( secret );

       auto child_priv_key = to_priv_key->child( child );
       //auto blind_factor = fc::sha256::hash( child );

       auto plain_memo = fc::aes_decrypt( secret, conf.encrypted_memo );
       auto memo = fc::raw::unpack<stealth_confirmation::memo_data>( plain_memo );

       result.to_key   = *conf.to;
       result.to_label = get_key_label( result.to_key );
       if( memo.from )
       {
          result.from_key = *memo.from;
          result.from_label = get_key_label( result.from_key );
          if( result.from_label == string() )
          {
             result.from_label = opt_from;
             set_key_label( result.from_key, result.from_label );
          }
       }
       else
       {
          result.from_label = opt_from;
       }
       result.amount = memo.amount;
       result.memo = opt_memo;

       // confirm the amount matches the commitment (verify the blinding factor)
       auto commtiment_test = fc::ecc::blind( memo.blinding_factor, memo.amount.amount.value );
       FC_ASSERT( fc::ecc::verify_sum( {commtiment_test}, {memo.commitment}, 0 ) );

       blind_balance bal;
       bal.amount = memo.amount;
       bal.to     = *conf.to;
       if( memo.from ) bal.from   = *memo.from;
       bal.one_time_key = conf.one_time_key;
       bal.blinding_factor = memo.blinding_factor;
       bal.commitment = memo.commitment;
       bal.used = false;

       auto child_pubkey = child_priv_key.get_public_key();
       auto owner = authority(1, public_key_type(child_pubkey), 1);
       result.control_authority = owner;
       result.data = memo;

       auto child_key_itr = owner.key_auths.find( child_pubkey );
       if( child_key_itr != owner.key_auths.end() )
          my->_keys[child_key_itr->first] = key_to_wif( child_priv_key );

       // my->_wallet.blinded_balances[memo.amount.asset_id][bal.to].push_back( bal );

       result.date = fc::time_point::now();
       my->_wallet.blind_receipts.insert( result );
       my->_keys[child_pubkey] = key_to_wif( child_priv_key );

       save_wallet_file();

       return result;
    }

    vector<blind_receipt> wallet_api::blind_history( string key_or_account )
    {
       vector<blind_receipt> result;
       auto pub_key = get_public_key( key_or_account );

       if( pub_key == public_key_type() )
          return vector<blind_receipt>();

       for( auto& r : my->_wallet.blind_receipts )
       {
          if( r.from_key == pub_key || r.to_key == pub_key )
             result.push_back( r );
       }
       std::sort( result.begin(), result.end(), [&]( const blind_receipt& a, const blind_receipt& b ){ return a.date > b.date; } );
       return result;
    }

    order_book wallet_api::get_order_book( const string& base, const string& quote, unsigned limit )
    {
       return( my->_remote_db->get_order_book( base, quote, limit ) );
    }

    optional<pocs_object> wallet_api::get_pocs_object(league_id_type league_id, string account, object_id_type product_id)
    {
        account_id_type account_id = get_account(account).get_id();
        return my->_remote_db->get_pocs_object(league_id, account_id, product_id);
    }

    vesting_balance_object_with_info::vesting_balance_object_with_info( const vesting_balance_object& vbo, fc::time_point_sec now )
       : vesting_balance_object( vbo )
    {
       allowed_withdraw = get_allowed_withdraw( now );
       allowed_withdraw_time = now;
    }

    } } // graphene::wallet

    void fc::to_variant(const account_multi_index_type& accts, fc::variant& vo)
    {
       vo = vector<account_object>(accts.begin(), accts.end());
    }

    void fc::from_variant(const fc::variant& var, account_multi_index_type& vo)
    {
       const vector<account_object>& v = var.as<vector<account_object>>();
       vo = account_multi_index_type(v.begin(), v.end());
    }
