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

#include <sstream>

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
            gxb_assert(it->amount.asset_id == asset_id, "you can only use one asset type");
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

        gxb_assert(it->amount >= amount, "balance not enough");

        if (it->amount == amount) {
            balances.erase(it);
        } else {
            print("withdraw part, to udpate");
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
            print("balance not enough");
            return;
        }

        gxb_assert(it->amount >= amount, "balance not enough");

        auto packet_it = packets.find(owner);
        if (packet_it != packets.end()) {
            print("you can only create one packet");
            return;
        }

        packets.emplace(owner, [&](auto &o) {
            o.account_id = owner;
            o.pub_key = pubkey;
            o.amount = amount;
            o.number = number;
            o.subpackets.reserve(number);

            int shares_sum = 0;
            vector<int> shares(10);

            checksum160 sum160;

            std::stringstream stream;
            
            int64_t block_num = get_head_block_num();
            for (int i = 0; i < number; i++) {
                stream << pubkey << i << block_num;
                std::string s = stream.str();
                ripemd160(const_cast<char*>(s.c_str()), s.length(), &sum160);
                shares.emplace_back(sum160.hash[0]);
                shares_sum += sum160.hash[0];
            }

            for (int i = 0; i < number; i++) {
                o.subpackets.emplace_back(amount * shares[i] / 100);
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

        std::stringstream stream;
        stream << nonce;
        std::string stringnonce = stream.str();

        signature sg;
        sig.copy((char*)sg.data, 65, 0);
        int ret = verify_signature(const_cast<char*>(stringnonce.c_str()), stringnonce.length(), &sg, const_cast<char*>(it->pub_key.c_str()), it->pub_key.length());

        print("ret=", ret);
        if (ret != 0) {
            print("you hava no auth");
            return;
        }

        auto record_it = packetrecords.find(owner);
        if (record_it != packetrecords.end()) {
            print("you can only rob once, you have rob from the packet");
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
    }
};

GXB_ABI(redpacket, (deposit)(withdraw)(withdrawall)(issuepacket)(robpacket))
