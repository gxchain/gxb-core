#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

static const uint64_t bet_asset_id = 0; //GXC
static const uint32_t ahead_block_num = 3;

class simpledice : public contract
{
  public:
    simpledice(uint64_t self)
        : contract(self)
        , offers(_self, _self)
    {
    }

    //@abi action
    //@abi payable
    void offerbet()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        graphene_assert(bet_asset_id == asset_id, "not supported asset");
        graphene_assert(asset_amount > 0, "not supported asset");

        uint64_t player = get_trx_sender();
        auto cur_player_itr = offers.find(player);
        graphene_assert(cur_player_itr == offers.end(), "you have a unfinished offer, you can cancel it and rebet");

        const contract_asset bet{asset_amount, asset_id};
        cur_player_itr = offers.emplace(0, [&](auto &o) {
            o.owner = player;
            o.bet = bet;
            o.matchedplayer = 0;
        });

        for (auto matched_player_itr = offers.begin(); matched_player_itr != offers.end(); ++matched_player_itr) {
            if (matched_player_itr->matchedplayer == 0 && matched_player_itr->owner != player && matched_player_itr->bet == bet) {
                int64_t head_block_num = get_head_block_num();
                offers.modify(cur_player_itr, 0, [&](auto &o) {
                    o.matchedplayer = matched_player_itr->owner;
                    o.betblocknum = head_block_num + ahead_block_num;
                });

                offers.modify(matched_player_itr, 0, [&](auto &o) {
                    o.matchedplayer = player;
                    o.betblocknum = head_block_num + ahead_block_num;
                });

                break;
            }
        }
    }

    //@abi action
    void canceloffer()
    {
        uint64_t player = get_trx_sender();
        auto player_offer_itr = offers.find(player);

        graphene_assert(player_offer_itr != offers.end(), "offer does not exists");
        graphene_assert(player_offer_itr->matchedplayer == 0, "unable to cancel offer than had been matched");

        withdraw_asset(_self, player, player_offer_itr->bet.asset_id, player_offer_itr->bet.amount);
        offers.erase(player_offer_itr);
    }

    //@abi action
    void reveal()
    {
        uint64_t player = get_trx_sender();
        auto player_offer_itr = offers.find(player);

        graphene_assert(player_offer_itr != offers.end(), "offer not found");
        graphene_assert(player_offer_itr->matchedplayer > 0, "unable to reveal offer that not been matched");

        auto matched_offer_itr = offers.find(player_offer_itr->matchedplayer);
        graphene_assert(matched_offer_itr != offers.end(), "matched offer not found");
        graphene_assert(matched_offer_itr->matchedplayer > 0, "unable to reveal that not been matched");

        checksum160 block_id;
        get_block_id_for_num(&block_id, player_offer_itr->betblocknum);

        if (block_id.hash[player_offer_itr->owner % 20] > block_id.hash[player_offer_itr->matchedplayer % 20]) {
            withdraw_asset(_self, player, player_offer_itr->bet.asset_id, 2 * player_offer_itr->bet.amount);
        } else {
            withdraw_asset(_self, player_offer_itr->matchedplayer, player_offer_itr->bet.asset_id, 2 * player_offer_itr->bet.amount);
        }

        offers.erase(player_offer_itr);
        offers.erase(matched_offer_itr);
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t owner;
        contract_asset bet;
        uint64_t matchedplayer;
        uint32_t betblocknum;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(offer, (owner)(bet)(matchedplayer)(betblocknum))
    };

    typedef multi_index<N(offer), offer> offer_index;

    offer_index offers;
};

GRAPHENE_ABI(simpledice, (offerbet)(canceloffer)(reveal))
