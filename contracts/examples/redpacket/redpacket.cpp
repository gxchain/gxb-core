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

#include<cmath>

using namespace graphene;

static const uint64_t redpacket_asset_id = 1;//GXS

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
    void issue(std::string pubkey, uint64_t number)
    {
        // check publick key
        // TODO
        std::string prefix("GXC");
        const size_t prefix_len = prefix.size();
        graphene_assert(pubkey.size() > prefix_len, "invalid public key");
        graphene_assert(pubkey.substr(0, prefix_len) == prefix, "invalid public key");

        int64_t total_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        graphene_assert(redpacket_asset_id == asset_id, "not supported asset");
        uint64_t owner = get_trx_sender();

        auto packet_it = packets.find(owner);
        graphene_assert(packet_it == packets.end(), "already has one redpacket");
        graphene_assert(number <= 200, "max 200 redpacket");

        // allocate redpacket
        int64_t block_num = get_head_block_num();
        vector<int> shares;
        int64_t shares_sum = 0;

        std::string random_str = pubkey + std::to_string(number) + std::to_string(block_num);
        checksum160 sum160;
        ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
        for (int i = 0; i < number; i++) {
            uint8_t share = (uint8_t)(sum160.hash[i % 20] * (i + 1));
            share = share == 0 ? i : share;
            shares.emplace_back(share);
            shares_sum += share;
        }

        packets.emplace(owner, [&](auto &o) {
            o.issuer = owner;
            o.pub_key = pubkey;
            o.total_amount = contract_asset{total_amount, asset_id};
            o.number = number;
            int64_t share_used_sum = 0;
            for (int i = 0; i < number - 1; i++) {
                int64_t share_amount = total_amount * shares[i] / shares_sum;
                o.subpackets.emplace_back(share_amount);
                share_used_sum += share_amount;
            }
            o.subpackets.emplace_back(total_amount - share_used_sum);
        });
    }

    // @abi action
    void open(std::string issuer, signature sig)
    {
        uint64_t sender = get_trx_sender();
        int64_t now = get_head_block_time();

        // check redpacket
        int64_t issuer_id = get_account_id(issuer.c_str(), issuer.size());
        graphene_assert(issuer_id >= 0, "invalid account_name issuer");
        auto packet_iter = packets.find(issuer_id);
        graphene_assert(packet_iter != packets.end(), "no redpacket");

        // check signature
        std::string s = std::to_string(sender);
        bool flag = verify_signature(s.c_str(), s.length(), &sig, packet_iter->pub_key.c_str(), packet_iter->pub_key.length());
        graphene_assert(flag, "signature not valid");

        // check record
        auto record_iter = records.find(issuer_id);
        if (record_iter == records.end()) {
            records.emplace(sender, [&](auto& o){
                            o.packet_issuer = issuer_id;
                            o.accounts = {};
                            });
            record_iter = records.find(issuer_id);
        } else {
            auto act_iter = std::find_if(record_iter->accounts.begin(), record_iter->accounts.end(),
                                    [&](const account& act) { return act.account_id == sender; });
            graphene_assert(act_iter == record_iter->accounts.end(), "redpacket can only be opened once");
        }

        // update records
        uint64_t current_idx = sender % packet_iter->subpackets.size();
        int64_t current_amount = packet_iter->subpackets[current_idx];
        records.modify(record_iter, sender, [&](auto &o) {
            o.packet_issuer = issuer_id;
            o.accounts.push_back({sender, current_amount});
        });
        print("got redpacket amount:", current_amount);

        auto subpacket_it = packet_iter->subpackets.begin() + current_idx;
        packets.modify(packet_iter, sender, [&](auto &o) {
            o.subpackets.erase(subpacket_it);
        });

        // if left packet number is 0
        if (packet_iter->subpackets.empty()) {
            // remove packet
            packets.erase(packet_iter);
            records.erase(record_iter);
        }
        withdraw_asset(_self, sender, packet_iter->total_amount.asset_id, current_amount);
    }

    // @abi action
    void close()
    {
        uint64_t owner = get_trx_sender();
        auto packet_iter = packets.find(owner);
        graphene_assert(packet_iter != packets.end(), "no redpacket");

        uint64_t asset_id = packet_iter->total_amount.asset_id;
        int64_t left_amount;
        for (uint64_t subpacket : packet_iter->subpackets) {
            left_amount += subpacket;
        }
        packets.erase(packet_iter);

        // remove records
        auto record_iter = records.find(owner);
        if (record_iter != records.end()) {
            records.erase(record_iter);
        }

        withdraw_asset(_self, owner, asset_id, left_amount);
    }

  private:
    //@abi table packet i64
    struct packet {
        uint64_t                issuer;
        std::string             pub_key;
        contract_asset          total_amount;
        uint32_t                number;
        vector<int64_t>         subpackets;

        uint64_t primary_key() const { return issuer; }

        GRAPHENE_SERIALIZE(packet, (issuer)(pub_key)(total_amount)(number)(subpackets))
    };
    typedef graphene::multi_index<N(packet), packet> packet_index;

    struct account {
        uint64_t    account_id;
        int64_t     amount;

        GRAPHENE_SERIALIZE(account, (account_id)(amount))
    };

    //@abi table record i64
    struct record {
        uint64_t packet_issuer;
        std::vector<account> accounts;

        uint64_t primary_key() const { return packet_issuer; }

        GRAPHENE_SERIALIZE(record, (packet_issuer)(accounts))
    };
    typedef graphene::multi_index<N(record), record> record_index;

    packet_index            packets;
    record_index            records;
};

GRAPHENE_ABI(redpacket, (issue)(open)(close))
