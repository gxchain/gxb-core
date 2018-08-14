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

  private:
    auto get_matched_offer(const contract_asset &bet, uint64_t owner) const
    {
        auto offer_it = offers.begin();
        for (; offer_it != offers.end(); ++offer_it) {
            if (offer_it->bet == bet && offer_it->owner != owner) {
                break;
            }
        }

        return offer_it;
    }

    auto get_offer_by_commitment(const checksum256 &commitment)
    {
        auto matched_offer_itr = offers.begin();
        for (; matched_offer_itr != offers.end(); ++matched_offer_itr) {
            if (0 == memcmp(matched_offer_itr->commitment.hash, commitment.hash, 32)) {
                break;
            }
        }

        return matched_offer_itr;
    }

  public:
    //@abi action
    //@abi payable
    void deposit()
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();

        auto itr = accounts.find(owner);
        if (itr == accounts.end()) {
            itr = accounts.emplace(0, [&](auto &o) {
                o.owner = owner;
            });
        }

        contract_asset ast{asset_amount, asset_id};
        accounts.modify(itr, 0, [&](auto &o) {
            o.balance += ast;
        });
    }

    //@abi action
    void withdraw(const contract_asset &quantity)
    {
        graphene_assert(quantity.amount > 0, "must withdraw positive quantity");

        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);
        graphene_assert(it != accounts.end(), "unknown account");

        accounts.modify(it, 0, [&](auto &o) {
            graphene_assert(o.balance >= quantity, "insufficient balance");
            o.balance -= quantity;
        });

        if (it->is_empty()) {
            accounts.erase(it);
        }

        withdraw_asset(_self, owner, quantity.asset_id, quantity.amount);
    }

    //@abi action
    void offerbet(const contract_asset &bet, const checksum256 &commitment)
    {
        graphene_assert(get_offer_by_commitment(commitment) == offers.end(), "offer with this commitment already exist");
        uint64_t player = get_trx_sender();
        auto cur_player_it = accounts.find(player);

        graphene_assert(cur_player_it != accounts.end(), "unknown account");
        graphene_assert(cur_player_it->balance >= bet, "balance not enouth");

        auto new_offer_it = offers.emplace(player, [&](auto &offer) {
            offer.id = offers.available_primary_key();
            offer.bet = bet;
            offer.owner = player;
            offer.commitment = commitment;
            offer.gameid = 0;
        });

        auto matched_offer_it = get_matched_offer(bet, player);
        if (matched_offer_it == offers.end()) {
            accounts.modify(cur_player_it, 0, [&](auto &o) {
                graphene_assert(o.balance >= bet, "insufficient balance");
                o.balance -= bet;
                o.open_offers++;
            });
            return;
        }

        auto gdice_it = global_dices.begin();
        if (gdice_it == global_dices.end()) {
            gdice_it = global_dices.emplace(0, [&](auto &o) {
                o.nextgameid = 0;
            });
        }

        global_dices.modify(gdice_it, 0, [&](auto &o) {
            o.nextgameid++;
        });

        auto game_it = games.emplace(0, [&](auto &o) {
            o.id = gdice_it->nextgameid;
            o.bet = new_offer_it->bet;
            o.deadline = 0;

            o.player1.commitment = matched_offer_it->commitment;
            memset(&o.player1.reveal, 0, sizeof(checksum256));

            o.player2.commitment = new_offer_it->commitment;
            memset(&o.player2.reveal, 0, sizeof(checksum256));
        });

        offers.modify(matched_offer_it, 0, [&](auto &o) {
            o.gameid = game_it->id;
        });

        offers.modify(new_offer_it, 0, [&](auto &o) {
            o.gameid = game_it->id;
        });

        accounts.modify(accounts.find(matched_offer_it->owner), 0, [&](auto &o) {
            o.open_offers--;
            o.open_games++;
        });

        accounts.modify(cur_player_it, 0, [&](auto &o) {
            graphene_assert(o.balance >= bet, "insufficient balance");
            o.balance -= bet;
            o.open_games++;
        });
    }

    //@abi action
    void canceloffer(const checksum256 &commitment)
    {
        auto offer_it = get_offer_by_commitment(commitment);

        if (offer_it != offers.end()) {
            auto acnt_it = accounts.find(offer_it->owner);
            accounts.modify(acnt_it, 0, [&](auto &o) {
                o.open_offers--;
                o.balance += offer_it->bet;
            });

            offers.erase(offer_it);
        }
    }

    //@abi action
    void reveal(const checksum256 &commitment, const checksum256 &source)
    {
        assert_sha256((char *) &source, sizeof(source), (const checksum256 *) &commitment);

        auto cur_reveal_offer = get_offer_by_commitment(commitment);
        graphene_assert(cur_reveal_offer != offers.end(), "commetment not exist");

        auto game_it = games.find(cur_reveal_offer->gameid);

        player curr_reveal = game_it->player1;
        player prev_reveal = game_it->player2;

        if (!is_equal(curr_reveal.commitment, commitment)) {
            std::swap(curr_reveal, prev_reveal);
        }

        graphene_assert(is_zero(curr_reveal.reveal) == true, "player already revealed");

        if (!is_zero(prev_reveal.reveal)) {
            checksum256 result;
            sha256((char *) &game_it->player1, sizeof(player) * 2, &result);

            auto prev_reveal_offer = get_offer_by_commitment(prev_reveal.commitment);
            int win = result.hash[1] < result.hash[0] ? 0 : 1;

            if (win) {
                pay_and_clean(*game_it, *cur_reveal_offer, *prev_reveal_offer);
            } else {
                pay_and_clean(*game_it, *prev_reveal_offer, *cur_reveal_offer);
            }
        } else {
            games.modify(game_it, 0, [&](auto &game) {
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
        auto game_it = games.find(gameid);

        graphene_assert(game_it != games.end(), "game not found");
        graphene_assert(game_it->deadline != 0 && get_head_block_time() > game_it->deadline, "game not expired");

        auto p1_offer = get_offer_by_commitment(game_it->player1.commitment);
        auto p2_offer = get_offer_by_commitment(game_it->player2.commitment);

        if (!is_zero(game_it->player1.reveal)) {
            graphene_assert(is_zero(game_it->player2.reveal), "game error");
            pay_and_clean(*game_it, *p1_offer, *p2_offer);
        } else {
            graphene_assert(is_zero(game_it->player1.reveal), "game error");
            pay_and_clean(*game_it, *p2_offer, *p1_offer);
        }
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t        id;
        uint64_t        owner;
        contract_asset  bet;
        checksum256     commitment;
        uint64_t        gameid = 0;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(offer, (id)(owner)(bet)(commitment)(gameid))
    };

    typedef multi_index<N(offer), offer> offer_index;

    struct player {
        checksum256     commitment;
        checksum256     reveal;

        GRAPHENE_SERIALIZE(player, (commitment)(reveal))
    };

    //@abi table game i64
    struct game {
        uint64_t        id;
        contract_asset  bet;
        int64_t         deadline;
        player          player1;
        player          player2;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(game, (id)(bet)(deadline)(player1)(player2))
    };

    typedef multi_index<N(game), game> game_index;

    //@abi table global_dice i64
    struct global_dice {
        uint64_t    id = 0;
        uint64_t    nextgameid = 0;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(global_dice, (id)(nextgameid))
    };

    typedef multi_index<N(global_dice), global_dice> global_dice_index;

    //@abi table account i64
    struct account {
        uint64_t        owner;
        contract_asset  balance;
        uint32_t        open_offers = 0;
        uint32_t        open_games = 0;

        bool is_empty() const { return !(balance.amount | open_offers | open_games); }

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balance)(open_offers)(open_games))
    };

    typedef multi_index<N(account), account> account_index;

    offer_index         offers;
    game_index          games;
    global_dice_index   global_dices;
    account_index       accounts;

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

GRAPHENE_ABI(dice, (deposit)(withdraw)(offerbet)(canceloffer)(reveal)(claimexpired))
