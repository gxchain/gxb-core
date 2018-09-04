#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

static const uint64_t bet_asset_id = 0; //GXC

class simpledice : public contract
{
  public:
    const uint32_t FIVE_MINUTES = 5 * 60;

    simpledice(uint64_t self)
        : contract(self)
        , offers(_self, _self)
        , accounts(_self, _self)
    {
    }

    //@abi action
    void offerbet(const contract_asset &bet)
    {
        graphene_assert(bet_asset_id == bet.asset_id, "not supported asset");

        uint64_t p1_id = get_trx_sender();
        auto p1_acnt_it = accounts.find(p1_id);
        graphene_assert(p1_acnt_it != accounts.end(), "unknown account");
        graphene_assert(p1_acnt_it->balance >= bet, "balance not enouth");

        auto p1_offer_it = offers.find(p1_id);
        graphene_assert(p1_offer_it == offers.end(), "you have a unfinished offer, you can cancel it and rebet");

        p1_offer_it = offers.emplace(0, [&](auto &o) {
            o.owner = p1_id;
            o.bet = bet;
            o.matchedplayer = 0;
        });

        for (auto p2_offer_it = offers.begin(); p2_offer_it != offers.end(); ++p2_offer_it) {
            if (p2_offer_it->matchedplayer == 0 && p2_offer_it->owner != p1_id && p2_offer_it->bet == bet) {
                int64_t head_block_num = get_head_block_num();
                offers.modify(p1_offer_it, 0, [&](auto &o) {
                    o.matchedplayer = p2_offer_it->owner;
                    o.betblocknum = head_block_num + 3;
                });

                offers.modify(p2_offer_it, 0, [&](auto &o) {
                    o.matchedplayer = p1_id;
                    o.betblocknum = head_block_num + 3;
                });

                break;
            }
        }

        accounts.modify(p1_acnt_it, 0, [&](auto &o) {
            graphene_assert(o.balance >= bet, "insufficient balance");
            o.balance -= bet;
        });
    }

    //@abi action
    void canceloffer()
    {
        uint64_t p1_id = get_trx_sender();
        auto p1_offer_it = offers.find(p1_id);

        graphene_assert(p1_offer_it != offers.end(), "offer does not exists");
        graphene_assert(p1_offer_it->matchedplayer == 0, "unable to cancel offer");

        auto p1_acnt_it = accounts.find(p1_offer_it->owner);
        accounts.modify(p1_acnt_it, 0, [&](auto &o) {
            o.balance += p1_offer_it->bet;
        });

        offers.erase(p1_offer_it);
    }

    //@abi action
    void reveal()
    {
        uint64_t p1_id = get_trx_sender();
        auto p1_offer_it = offers.find(p1_id);

        graphene_assert(p1_offer_it != offers.end(), "offer not found");
        graphene_assert(p1_offer_it->matchedplayer > 0, "unable to reveal");

        auto p2_offer_it = offers.find(p1_offer_it->matchedplayer);
        graphene_assert(p2_offer_it != offers.end(), "offer not found");
        graphene_assert(p2_offer_it->matchedplayer > 0, "unable to reveal");

        checksum160 block_id;
        get_block_id_for_num(&block_id, p1_offer_it->betblocknum);

        if (block_id.hash[p1_offer_it->owner % 20] > block_id.hash[p1_offer_it->matchedplayer % 20]) {
            auto p1_acnt_it = accounts.find(p1_offer_it->owner);
            accounts.modify(p1_acnt_it, 0, [&](auto &o) {
                o.balance += 2 * p1_offer_it->bet;
            });
        } else {
            auto p2_acnt_it = accounts.find(p2_offer_it->owner);
            accounts.modify(p2_acnt_it, 0, [&](auto &o) {
                o.balance += 2 * p1_offer_it->bet;
            });
        }

        offers.erase(p1_offer_it);
        offers.erase(p2_offer_it);
    }

    //@abi action
    //@abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

        graphene_assert(bet_asset_id == asset_id, "not supported asset");
        graphene_assert(asset_amount > 0, "not supported asset");

        auto acnt_it = accounts.find(owner);
        if (acnt_it == accounts.end()) {
            acnt_it = accounts.emplace(_self, [&](auto &o) {
                o.owner = owner;
            });
        }

        contract_asset ast{asset_amount, asset_id};
        accounts.modify(acnt_it, 0, [&](auto &o) {
            o.balance += ast;
        });
    }

    //@abi action
    void withdraw(const contract_asset &quantity)
    {
        uint64_t owner = get_trx_sender();
        graphene_assert(quantity.amount > 0, "must withdraw positive quantity");

        auto owner_it = accounts.find(owner);
        graphene_assert(owner_it != accounts.end(), "unknown account");

        accounts.modify(owner_it, 0, [&](auto &o) {
            graphene_assert(o.balance >= quantity, "insufficient balance");
            o.balance -= quantity;
        });

        auto offer_it = offers.find(owner);
        if (owner_it->balance == 0 && offer_it != offers.end()) {
            accounts.erase(owner_it);
        }

        withdraw_asset(_self, owner, quantity.asset_id, quantity.amount);
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

    //@abi table account i64
    struct account {
        uint64_t owner;
        contract_asset balance;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balance))
    };

    typedef multi_index<N(account), account> account_index;

    offer_index offers;
    account_index accounts;
};

GRAPHENE_ABI(simpledice, (offerbet)(canceloffer)(reveal)(deposit)(withdraw))
