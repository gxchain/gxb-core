#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/system.h>
#include <graphenelib/types.h>

using namespace graphene;

class redpacket : public contract
{
  private:
    //@abi table balance i64
    struct balance {
        uint64_t account_id;
        contract_asset amount;

        uint64_t primary_key() const { return account_id; }
        GXBLIB_SERIALIZE(balance, (account_id)(amount))
    };
    typedef graphene::multi_index<N(balance), balance> balance_index;

    //@abi table packet i64
    struct packet {
        uint64_t account_id;
        std::string pub_key;
        contract_asset amount;
        uint32_t number;
        vector<contract_asset> subpackets;

        uint64_t primary_key() const { return account_id; }

        GXBLIB_SERIALIZE(packet, (account_id)(pub_key)(amount)(number)(subpackets))
    };
    typedef graphene::multi_index<N(packet), packet> packet_index;

    struct record {
        uint64_t account_id;
        contract_asset amount;
    };

    //@abi table packetrecord i64
    struct packetrecord {
        uint64_t packet_id;
        std::vector<record> records;

        uint64_t primary_key() const { return packet_id; }

        GXBLIB_SERIALIZE(packetrecord, (packet_id)(records))
    };
    typedef graphene::multi_index<N(packetrecord), packetrecord> packetrecord_index;

    balance_index balances;
    packet_index packets;
    packetrecord_index packetrecords;

  public:
    redpacket(uint64_t id)
        : contract(id)
        , balances(_self, _self)
        , packets(_self, _self)
        , packetrecords(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        uint64_t owner = get_trx_sender();
        auto it = balances.find(owner);
        if (it == balances.end()) {
            balances.emplace(owner, [&](auto &o) {
                o.account_id = owner;
                o.amount.amount = asset_amount;
                o.amount.asset_id = asset_id;
            });
        } else {
            graphene_assert(it->amount.asset_id == asset_id, "you can only use one asset type");
            balances.modify(it, 0, [&](auto &o) {
                o.amount.amount += asset_amount;
            });
        }
    }

    // @abi action
    void withdraw(int64_t amount)
    {
        uint64_t owner = get_trx_sender();

        auto it = balances.find(owner);
        if (it == balances.end()) {
            return;
        }

        graphene_assert(it->amount >= amount, "balance not enough");

        if (it->amount == amount) {
            balances.erase(it);
        } else {
            print("withdraw part, to udpate\n");
            balances.modify(it, 0, [&](auto &o) {
                o.amount -= amount;
            });
        }

        withdraw_asset(_self, owner, it->amount.asset_id, amount);
    }

    // @abi action
    void withdrawall()
    {
        uint64_t owner = get_trx_sender();

        auto it = balances.find(owner);
        if (it == balances.end()) {
            return;
        }

        balances.erase(it);
        withdraw_asset(_self, owner, it->amount.asset_id, it->amount.amount);
    }

    // @abi action
    void issuepacket(std::string pubkey, int64_t amount, uint64_t number)
    {
        uint64_t owner = get_trx_sender();

        auto it = balances.find(owner);
        if (it == balances.end()) {
            print("balance not enough\n");
            return;
        }

        graphene_assert(it->amount.amount >= amount, "balance not enough");

        auto packet_it = packets.find(owner);
        if (packet_it != packets.end()) {
            print("you can only create one packet");
            return;
        }

        int64_t block_num = get_head_block_num();
        int shares_sum = 0;
        vector<int> shares;
        shares.reserve(number);
        checksum160 sum160;

        std::string random_bash;
        for (int i = 0; i < number; i++) {
            random_bash = pubkey + std::to_string(i) + std::to_string(block_num);
            print("random_bash=", random_bash.c_str(), "\n");
            ripemd160(const_cast<char *>(random_bash.c_str()), random_bash.length(), &sum160);
            shares.emplace_back(sum160.hash[0]);
            shares_sum += sum160.hash[0];
        }

        packets.emplace(owner, [&](auto &o) {
            o.account_id = owner;
            o.pub_key = pubkey;
            o.amount = amount;
            o.number = number;
            o.subpackets.reserve(number);

            for (int i = 0; i < number; i++) {
                print("share=", shares[i], "\n");
                o.subpackets.emplace_back(contract_asset{amount * shares[i] / shares_sum, it->amount.asset_id});
            }
        });

        if (it->amount.amount == amount) {
            balances.erase(it);
        } else {
            balances.modify(it, 0, [&](auto &o) {
                o.amount.amount -= amount;
            });
        }
    }

    // @abi action
    void robpacket(uint64_t packetowner, std::string &sig, uint64_t nonce)
    {
        uint64_t owner = get_trx_sender();

        auto it = packets.find(packetowner);
        if (it == packets.end()) {
            print("owner:", packetowner, "has no issued red packet");
            return;
        }

        std::string stringnonce = std::to_string(nonce);

        print("stringnonce=", stringnonce, "\n");
        print("x=", it->pub_key.c_str(), "\n");
        print("sig=", sig, "\n");
        int ret = verify_signature(stringnonce.c_str(), stringnonce.length(), sig.c_str(), sig.length(), it->pub_key.c_str(), it->pub_key.length());

        print("ret=", ret, "\n");
        if (ret != 0) {
            print("you hava no auth");
            return;
        }

        auto record_it = packetrecords.find(owner);
        if (record_it != packetrecords.end()) {
            print("you can only rob once, you have rob from the packet\n");
            return;
        }

        int subpacketsindex = nonce % it->subpackets.size();

        withdraw_asset(_self, owner, it->amount.asset_id, it->subpackets[subpacketsindex].amount);

        auto subpacket_it = it->subpackets.begin() + subpacketsindex;

        packetrecords.emplace(owner, [&](auto &o) {
            record r;
            r.account_id = owner;
            r.amount.amount = it->subpackets[subpacketsindex].amount;

            o.packet_id = owner;
            o.records.emplace_back(r);
        });

        packets.modify(it, 0, [&](auto &o) {
            o.subpackets.erase(subpacket_it);
        });
        
        if(it->subpackets.size() == 0) {
            packets.erase(it);
            for(auto record_it = packetrecords.begin();record_it != packetrecords.end();) {
                record_it = packetrecords.erase(record_it);
            }
        }
    }
};

GXB_ABI(redpacket, (deposit)(withdraw)(withdrawall)(issuepacket)(robpacket))
