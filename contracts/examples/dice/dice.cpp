#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/system.h>
#include <string>
#include <utility>
#include <vector>

using namespace graphene;

class dice : public contract
{
  public:
    const uint32_t FIVE_MINUTES = 5 * 60;

    dice(uint64_t self)
        : contract(self)
        , offers(_self, _self)
        , games(_self, _self)
        , global_dices(_self, _self)
        , accounts(_self, _self)
    {
    }

    //@abi action
    void offerbet(const contract_asset &bet, const checksum256 &commitment)
    {
        uint64_t player = get_trx_sender();

        auto cur_player_itr = accounts.find(player);
        graphene_assert(cur_player_itr != accounts.end(), "unknown account");
        graphene_assert(cur_player_itr->balance >= bet, "balance not enouth");

        // Store new offer
        auto new_offer_itr = offers.emplace(_self, [&](auto &offer) {
            offer.id = offers.available_primary_key();
            offer.bet = bet;
            offer.owner = player;
            offer.commitment = commitment;
            offer.gameid = 0;
        });

        bool new_game = false;
        auto matched_offer_itr = offers.begin();
        for (; matched_offer_itr != offers.end(); ++matched_offer_itr) {
            if (matched_offer_itr->owner != new_offer_itr->owner && matched_offer_itr->bet == new_offer_itr->bet) {
                new_game = true;
                break;
            }
        }

        if(new_game == false) {
            accounts.modify(cur_player_itr, 0, [&](auto &acnt) {
                graphene_assert(acnt.balance >= bet, "insufficient balance");
                acnt.balance -= bet;
                acnt.open_offers++;
            });
            return;
        }
        
        auto gdice_itr = global_dices.begin();
        if (gdice_itr == global_dices.end()) {
            gdice_itr = global_dices.emplace(_self, [&](auto &gdice) {
                gdice.nextgameid = 0;
            });
        }

        global_dices.modify(gdice_itr, 0, [&](auto &gdice) {
            gdice.nextgameid++;
        });

        auto game_itr = games.emplace(_self, [&](auto &new_game) {
            new_game.id = gdice_itr->nextgameid;
            new_game.bet = new_offer_itr->bet;
            new_game.deadline = 0;

            new_game.player1.commitment = matched_offer_itr->commitment;
            memset(&new_game.player1.reveal, 0, sizeof(checksum256));

            new_game.player2.commitment = new_offer_itr->commitment;
            memset(&new_game.player2.reveal, 0, sizeof(checksum256));
        });

        offers.modify(matched_offer_itr, 0, [&](auto &offer) {
            offer.bet.amount = 0;
            offer.gameid = game_itr->id;
        });

        offers.modify(new_offer_itr, 0, [&](auto &offer) {
            offer.bet.amount = 0;
            offer.gameid = game_itr->id;
        });

        accounts.modify(accounts.find(matched_offer_itr->owner), 0, [&](auto &acnt) {
            acnt.open_offers--;
            acnt.open_games++;
        });

        accounts.modify(cur_player_itr, 0, [&](auto &acnt) {
            graphene_assert(acnt.balance >= bet, "insufficient balance");
            acnt.balance -= bet;
            acnt.open_games++;
        });
    }

    //@abi action
    void canceloffer(const checksum256 &commitment)
    {
        auto offer_it = offers.begin();
        bool offer_exist = false;
        for (; offer_it != offers.end(); ++offer_it) {
            if (0 == memcmp(offer_it->commitment.hash, commitment.hash, 32)) {
                offer_exist = true;
                break;
            }
        }

        if (offer_exist == true) {
            auto acnt_it = accounts.find(offer_it->owner);
            accounts.modify(acnt_it, 0, [&](auto &acnt) {
                acnt.open_offers--;
                acnt.balance += offer_it->bet;
            });
            offers.erase(offer_it);
        }
    }

    //@abi action
    void reveal(const checksum256 &commitment, const checksum256 &source)
    {
        assert_sha256((char *) &source, sizeof(source), (const checksum256 *) &commitment);

        auto curr_revealer_offer = offers.begin();
        bool offer_exist = false;
        for (; curr_revealer_offer != offers.end(); ++curr_revealer_offer) {
            if (0 == memcmp(curr_revealer_offer->commitment.hash, commitment.hash, 32)) {
                offer_exist = true;
                break;
            }
        }

        if(offer_exist == false) {
            return;
        }
        
        auto game_itr = games.find(curr_revealer_offer->gameid);

        player curr_reveal = game_itr->player1;
        player prev_reveal = game_itr->player2;

        if (!is_equal(curr_reveal.commitment, commitment)) {
            std::swap(curr_reveal, prev_reveal);
        }

        graphene_assert(is_zero(curr_reveal.reveal) == true, "player already revealed");

        if (!is_zero(prev_reveal.reveal)) {
            checksum256 result;

            sha256((char *) &game_itr->player1, sizeof(player) * 2, &result);

            auto prev_revealer_offer = offers.begin();
            bool prev_offer_exist = false;
            for(;prev_revealer_offer != offers.end();++prev_revealer_offer) {
                if (0 == memcmp(prev_revealer_offer->commitment.hash, prev_reveal.commitment.hash, 32)) {
                    prev_offer_exist = true;
                    break;
                }
            }

            int winner = result.hash[1] < result.hash[0] ? 0 : 1;

            if (winner) {
                pay_and_clean(*game_itr, *curr_revealer_offer, *prev_revealer_offer);
            } else {
                pay_and_clean(*game_itr, *prev_revealer_offer, *curr_revealer_offer);
            }
        } else {
            games.modify(game_itr, 0, [&](auto &game) {
                if (is_equal(curr_reveal.commitment, game.player1.commitment))
                    game.player1.reveal = source;
                else
                    game.player2.reveal = source;

                game.deadline = get_head_block_time() + FIVE_MINUTES;
            });
        }
    }

    //@abi action
    void claimexpired(const uint64_t gameid)
    {
        auto game_itr = games.find(gameid);

        graphene_assert(game_itr != games.end(), "game not found");
        graphene_assert(game_itr->deadline != 0 && get_head_block_time() > game_itr->deadline, "game not expired");

        auto p1_offer = offers.begin();
        auto p2_offer = offers.begin();
        
        for(;p1_offer!=offers.end();++p1_offer) {
            if(0 == memcmp(p1_offer->commitment.hash, game_itr->player1.commitment.hash, 32)) {
                break;
            }
        }
        
        for(;p2_offer!=offers.end();++p2_offer) {
            if(0 == memcmp(p2_offer->commitment.hash, game_itr->player2.commitment.hash, 32)) {
                break;
            }
        }
        
        if (!is_zero(game_itr->player1.reveal)) {
            graphene_assert(is_zero(game_itr->player2.reveal), "game error");
            pay_and_clean(*game_itr, *p1_offer, *p2_offer);
        } else {
            graphene_assert(is_zero(game_itr->player1.reveal), "game error");
            pay_and_clean(*game_itr, *p2_offer, *p1_offer);
        }
    }

    //@abi action
    //@abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

        auto itr = accounts.find(owner);
        if (itr == accounts.end()) {
            itr = accounts.emplace(_self, [&](auto &acnt) {
                acnt.owner = owner;
            });
        }

        contract_asset ast{asset_amount, asset_id};
        accounts.modify(itr, 0, [&](auto &acnt) {
            acnt.balance += ast;
        });
    }

    //@abi action
    void withdraw(const contract_asset &quantity)
    {
        uint64_t owner = get_trx_sender();
        graphene_assert(quantity.amount > 0, "must withdraw positive quantity");

        auto itr = accounts.find(owner);
        graphene_assert(itr != accounts.end(), "unknown account");

        accounts.modify(itr, 0, [&](auto &acnt) {
            graphene_assert(acnt.balance >= quantity, "insufficient balance");
            acnt.balance -= quantity;
        });

        if (itr->is_empty()) {
            accounts.erase(itr);
        }

        withdraw_asset(_self, owner, quantity.asset_id, quantity.amount);
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t id;
        uint64_t owner;
        contract_asset bet;
        checksum256 commitment;
        uint64_t gameid = 0;

        uint64_t primary_key() const { return id; }

        uint64_t by_bet() const { return (uint64_t) bet.amount; }

        key256 by_commitment() const { return get_commitment(commitment); }

        static key256 get_commitment(const checksum256 &commitment)
        {
            const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&commitment);
            return key256::make_from_word_sequence<uint64_t>(p64[0], p64[1], p64[2], p64[3]);
        }

        GRAPHENE_SERIALIZE(offer, (id)(owner)(bet)(commitment)(gameid))
    };

    typedef multi_index<N(offer), offer>
        offer_index;

    struct player {
        checksum256 commitment;
        checksum256 reveal;

        GRAPHENE_SERIALIZE(player, (commitment)(reveal))
    };

    //@abi table game i64
    struct game {
        uint64_t id;
        contract_asset bet;
        int64_t deadline;
        player player1;
        player player2;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(game, (id)(bet)(deadline)(player1)(player2))
    };

    typedef multi_index<N(game), game> game_index;

    //@abi table global i64
    struct global_dice {
        uint64_t id = 0;
        uint64_t nextgameid = 0;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(global_dice, (id)(nextgameid))
    };

    typedef multi_index<N(global), global_dice> global_dice_index;

    //@abi table account i64
    struct account {
        uint64_t owner;
        contract_asset balance;
        uint32_t open_offers = 0;
        uint32_t open_games = 0;

        bool is_empty() const { return !(balance.amount | open_offers | open_games); }

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balance)(open_offers)(open_games))
    };

    typedef multi_index<N(account), account> account_index;

    offer_index offers;
    game_index games;
    global_dice_index global_dices;
    account_index accounts;

    bool is_equal(const checksum256 &a, const checksum256 &b) const
    {
        return memcmp((void *) &a, (const void *) &b, sizeof(checksum256)) == 0;
    }

    bool is_zero(const checksum256 &a) const
    {
        const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&a);
        return p64[0] == 0 && p64[1] == 0 && p64[2] == 0 && p64[3] == 0;
    }

    void pay_and_clean(const game &g,
                       const offer &winner_offer,
                       const offer &loser_offer)
    {
        auto winner_account = accounts.find(winner_offer.owner);
        accounts.modify(winner_account, 0, [&](auto &acnt) {
            acnt.balance += 2 * g.bet;
            acnt.open_games--;
        });

        auto loser_account = accounts.find(loser_offer.owner);
        accounts.modify(loser_account, 0, [&](auto &acnt) {
            acnt.open_games--;
        });

        if (loser_account->is_empty()) {
            accounts.erase(loser_account);
        }

        games.erase(g);
        offers.erase(winner_offer);
        offers.erase(loser_offer);
    }
};

GRAPHENE_ABI(dice, (offerbet)(canceloffer)(reveal)(claimexpired)(deposit)(withdraw))
