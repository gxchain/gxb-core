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
  public:
    redpacket(uint64_t id)
        : contract(id)
        , packets(_self, _self)
        , records(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void create(std::string pubkey, uint64_t number)
    {
        // validate pubkey

        int64_t amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

        auto packet_it = packets.find(owner);
        graphene_assert(packet_it == packets.end(), "already has one redpacket");
        graphene_assert(number <= 1000, "max 1000 redpacket");

        // allocate redpacket
        int64_t block_num = get_head_block_num();
        int shares_sum = 0;

        vector<int> shares;
        for (int i = 0; i < number; i++) {
            std::string random_str = pubkey + std::to_string(i) + std::to_string(block_num);
            print("random_str = ", random_str.c_str(), "\n");

            checksum160 sum160;
            ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
            uint8_t share = sum160.hash[0] == 0 ? 10 : sum160.hash[0];
            shares.emplace_back(share);
            shares_sum += share;
        }

        packets.emplace(owner, [&](auto &o) {
            o.account_id = owner;
            o.pub_key = pubkey;
            o.amount = amount;
            o.number = number;
            o.subpackets.reserve(number);

            int64_t share_used_sum = 0;
            for (int i = 0; i < number - 1; i++) {
                print("share=", shares[i], "\n");
                int64_t share_amount = (int64_t)(1.0f * amount / shares_sum * shares[i]);
                o.subpackets.emplace_back(contract_asset{share_amount, asset_id});
                share_used_sum += share_amount;
            }
            o.subpackets.emplace_back(contract_asset{amount - share_used_sum, asset_id});
        });
    }

    // @abi action
    void open(uint64_t packetowner, std::string &sig, uint64_t nonce)
    {
        uint64_t owner = get_trx_sender();

        auto it = packets.find(packetowner);
        if (it == packets.end()) {
            print("owner:", packetowner, "has no issued red packet");
            return;
        }

        std::string stringnonce = std::to_string(nonce);

        int ret = verify_signature(stringnonce.c_str(), stringnonce.length(), sig.c_str(), sig.length(), it->pub_key.c_str(), it->pub_key.length());
        if (ret != 0) {
            print("you sig is invalid, please check you private key");
            return;
        }

        auto record_it = records.find(owner);
        if (record_it != records.end()) {
            print("you can only rob once, you have rob from the packet\n");
            return;
        }

        int subpacketsindex = nonce % it->subpackets.size();

        withdraw_asset(_self, owner, it->amount.asset_id, it->subpackets[subpacketsindex].amount);

        auto subpacket_it = it->subpackets.begin() + subpacketsindex;

        records.emplace(owner, [&](auto &o) {
            account r;
            r.account_id = owner;
            r.amount.amount = it->subpackets[subpacketsindex].amount;

            o.packet_id = owner;
            o.accounts.emplace_back(r);
        });

        packets.modify(it, 0, [&](auto &o) {
            o.subpackets.erase(subpacket_it);
        });

        if (it->subpackets.size() == 0) {
            packets.erase(it);
            for (auto record_it = records.begin(); record_it != records.end();) {
                record_it = records.erase(record_it);
            }
        }
    }

    // @abi action
    void close()
    {
        uint64_t owner = get_trx_sender();

        auto it = packets.find(owner);
        graphene_assert(it != packets.end(), "no redpacket");

        int64_t amount;
        packets.modify(it, 0, [&](auto &o) {
            for (auto subpacket_it = o.subpackets.begin(); subpacket_it != o.subpackets.end();) {
                amount += subpacket_it->amount;
                subpacket_it = o.subpackets.erase(subpacket_it);
            }
        });

        packets.erase(it);

        for (auto packetrecords_it = records.begin(); packetrecords_it != records.end(); ++packetrecords_it) {
            records.erase(packetrecords_it);
        }

        withdraw_asset(_self, owner, it->amount.asset_id, amount);
    }

  private:
    //@abi table packet i64
    struct packet {
        uint64_t account_id;
        std::string pub_key;
        contract_asset amount;
        uint32_t number;
        vector<contract_asset> subpackets;

        uint64_t primary_key() const { return account_id; }

        GRAPHENE_SERIALIZE(packet, (account_id)(pub_key)(amount)(number)(subpackets))
    };
    typedef graphene::multi_index<N(packet), packet> packet_index;

    struct account {
        uint64_t account_id;
        contract_asset amount;
    };

    //@abi table record i64
    struct record {
        uint64_t packet_id;
        std::vector<account> accounts;

        uint64_t primary_key() const { return packet_id; }

        GRAPHENE_SERIALIZE(record, (packet_id)(accounts))
    };
    typedef graphene::multi_index<N(record), record> record_index;

    packet_index        packets;
    record_index        records;

};

GRAPHENE_ABI(redpacket, (create)(open)(close))
