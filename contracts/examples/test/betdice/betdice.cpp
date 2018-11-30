#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/system.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <utility>
#include <vector>

using namespace graphene;

struct st_seeds {
    checksum256 seed1;
    checksum256 seed2;
};

class betdice : public contract
{
  public:
    const uint64_t OPERATOR_ACCOUNT_ID = 17;
    const uint64_t ASSET_ID = 0;

    const uint64_t MINBET = 1;

    const uint64_t BETID_ID = 1;
    const uint64_t TOTALAMTBET_ID = 2;
    const uint64_t TOTALAMTWON_ID = 3;
    const uint64_t LIABILITIES_ID = 4;

    const uint64_t HOUSEEDGE_times10000 = 200;

    betdice(uint64_t self)
        : contract(self)
        , bets(_self, _self)
        , globalvars(_self, _self)
    {
    }

    //@abi action
    void init()
    {
        uint64_t caller = get_trx_sender();
        graphene_assert(caller == OPERATOR_ACCOUNT_ID, "no auth");

        auto globalvars_itr = globalvars.begin();
        graphene_assert(globalvars_itr == globalvars.end(), "init is done");

        globalvars.emplace(0, [&](auto &g) {
            g.id = BETID_ID;
            g.val = 0;
        });

        globalvars.emplace(0, [&](auto &g) {
            g.id = TOTALAMTBET_ID;
            g.val = 0;
        });

        globalvars.emplace(0, [&](auto &g) {
            g.id = TOTALAMTWON_ID;
            g.val = 0;
        });

        globalvars.emplace(0, [&](auto &g) {
            g.id = LIABILITIES_ID;
            g.val = 0;
        });
    }

    //@abi action
    //@abi payable
    void deposit()
    {
        uint64_t caller = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        graphene_assert(caller == OPERATOR_ACCOUNT_ID, "no auth");
        graphene_assert(asset_id == ASSET_ID, "asset not supported");

        increment_global_var(LIABILITIES_ID, asset_amount);
    }

    //@abi action
    void withdraw(uint64_t amt, uint64_t to)
    {
        uint64_t caller = get_trx_sender();
        graphene_assert(caller == OPERATOR_ACCOUNT_ID, "no auth");
        graphene_assert(get_global_var(LIABILITIES_ID) >= amt, "invalid amt");
        decrement_global_var(LIABILITIES_ID, amt);
        withdraw_asset(_self, to, ASSET_ID, amt);
    }

    //@abi action
    //@abi payable
    void roll(uint64_t roll_under, std::string roll_seed)
    {
        uint64_t bettor = get_trx_sender();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();

        graphene_assert(asset_id == ASSET_ID, "asset not supported");
        graphene_assert(asset_amount >= MINBET, "min bet is 1");
        graphene_assert(roll_under >= 2 && roll_under <= 95, "roll_down must >=2 and <=95");

        increment_global_var(LIABILITIES_ID, asset_amount);
        increment_global_var(BETID_ID, 1);
        increment_global_var(TOTALAMTBET_ID, asset_amount);

        uint64_t payout = (asset_amount * get_payout_mult_times10000(roll_under, HOUSEEDGE_times10000)) / 10000;
        uint64_t your_win_amount = payout - asset_amount;
        graphene_assert(your_win_amount < get_max_win(), "Bet less than max");

        std::string final_roll_seed = roll_seed + std::to_string(asset_amount) + std::to_string(roll_under);
        checksum256 roll_seed_hash;
        sha256((char *) &final_roll_seed, final_roll_seed.length(), &roll_seed_hash);

        int tx_size = transaction_size();
        char *tx = (char *) malloc(tx_size);
        read_transaction(tx, tx_size);
        
        checksum256 tx_hash;
        sha256(tx, tx_size, &tx_hash);

        free(tx);
        
        st_seeds seeds;
        seeds.seed1 = roll_seed_hash;
        seeds.seed2 = tx_hash;

        checksum256 seed_hash;
        sha256((char *) &seeds.seed1, sizeof(seeds.seed1) * 2, &seed_hash);

        uint64_t random_roll = ((seed_hash.hash[0] + seed_hash.hash[1] + seed_hash.hash[2] + seed_hash.hash[3] + seed_hash.hash[4] + seed_hash.hash[5] + seed_hash.hash[6] + seed_hash.hash[7]) * (seed_hash.hash[20] + seed_hash.hash[30]) % 100) + 1;

        if (random_roll < roll_under) {
            print("you win");
            increment_global_var(TOTALAMTWON_ID, payout);
            decrement_global_var(LIABILITIES_ID, payout);
            withdraw_asset(_self, bettor, ASSET_ID, payout);
        } else {
            print("you loss");
        }

        bets.emplace(0, [&](auto &o) {
            o.id = get_global_var(BETID_ID);
            o.bettor = bettor;
            o.bet_amt = asset_amount;
            o.roll_under = roll_under;
            o.seed = seed_hash;
            o.roll = random_roll;
        });
    }

  private:
    uint64_t get_max_win()
    {
        const uint64_t jackpot = get_global_var(LIABILITIES_ID);
        return jackpot / 25;
    }

    void decrement_global_var(uint64_t id, uint64_t delta)
    {
        auto globalvars_itr = globalvars.find(id);
        globalvars.modify(globalvars_itr, 0, [&](auto &g) {
            g.val -= delta;
        });
    }

    void increment_global_var(uint64_t id, uint64_t delta)
    {
        auto globalvars_itr = globalvars.find(id);
        globalvars.modify(globalvars_itr, 0, [&](auto &g) {
            g.val += delta;
        });
    }

    uint64_t get_global_var(uint64_t id)
    {
        auto globalvars_itr = globalvars.find(id);
        return globalvars_itr->val;
    }

    uint64_t get_payout_mult_times10000(const uint64_t roll_under, const uint64_t house_edge_times_10000) const
    {
        return ((10000 - house_edge_times_10000) * 100) / (roll_under - 1);
    }

  private:
    // @abi table bet i64
    struct bet {
        uint64_t id;
        uint64_t bettor;
        uint64_t bet_amt;
        uint64_t roll_under;
        checksum256 seed;
        uint64_t roll;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(bet, (id)(bettor)(bet_amt)(roll_under)(seed)(roll))
    };

    typedef multi_index<N(bet), bet> bet_index;

    // @abi table globalvar i64
    struct globalvar {
        uint64_t id;
        uint64_t val;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(globalvar, (id)(val));
    };

    typedef multi_index<N(globalvar), globalvar> globalvar_index;

    bet_index bets;
    globalvar_index globalvars;
};

GRAPHENE_ABI(betdice, (init)(deposit)(withdraw)(roll))
