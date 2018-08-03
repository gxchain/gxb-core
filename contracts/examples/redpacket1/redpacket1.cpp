#include <gxblib/asset.h>
#include <gxblib/contract.hpp>
#include <gxblib/contract_asset.hpp>
#include <gxblib/crypto.h>
#include <gxblib/dispatcher.hpp>
#include <gxblib/global.h>
#include <gxblib/multi_index.hpp>
#include <gxblib/print.hpp>
#include <gxblib/system.h>
#include <gxblib/types.h>

using namespace graphene;

class redpacket1 : public contract
{
  private:
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

    packet_index packets;
    packetrecord_index packetrecords;

  public:
    redpacket1(uint64_t id)
        : contract(id)
        , packets(_self, _self)
        , packetrecords(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void issuepacket(std::string pubkey, uint64_t number)
    {
        int64_t amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

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

        std::string random_base;
        for (int i = 0; i < number; i++) {
            random_base = pubkey + std::to_string(i) + std::to_string(block_num);
            print("random_bash=", random_base.c_str(), "\n");
            ripemd160(const_cast<char *>(random_base.c_str()), random_base.length(), &sum160);
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
                o.subpackets.emplace_back(contract_asset{amount * shares[i] / shares_sum, asset_id});
            }
        });
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

GXB_ABI(redpacket1, (issuepacket)(robpacket))
