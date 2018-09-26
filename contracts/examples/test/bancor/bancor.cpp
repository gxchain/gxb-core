#include <cmath>
#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>

using namespace graphene;

static const uint64_t BANCOR_ID = N("bancorcoin") % 10000 + 10000;
static const uint64_t GXS_ASSET_ID = 1;

class bancor : public contract
{
  public:
    bancor(uint64_t id)
        : contract(id)
        , coinmarkets(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void issue()
    {
        uint64_t asset_id = get_action_asset_id();
        int64_t asset_amount = get_action_asset_amount();

        auto coinmarket_itr = coinmarkets.find(BANCOR_ID);
        if (coinmarket_itr == coinmarkets.end()) {
            coinmarket_itr = coinmarkets.emplace(0, [&](auto &o) {
                o.bancorid = BANCOR_ID;
                o.supply = 100000000ll;
                if (asset_id == GXS_ASSET_ID) {
                    graphene_assert(o.base.balance.amount == 0, "base init finished");
                    o.base.balance.amount = asset_amount;
                    o.base.balance.asset_id = asset_id;
                } else {
                    graphene_assert(o.quote.balance.amount == 0, "quote init finished");
                    o.quote.balance.amount = asset_amount;
                    o.quote.balance.asset_id = asset_id;
                }
            });
        } else {
            coinmarkets.modify(coinmarket_itr, 0, [&](auto &o) {
                if (asset_id == GXS_ASSET_ID) {
                    graphene_assert(o.base.balance.amount == 0, "base init finished");
                    o.base.balance.amount = asset_amount;
                    o.base.balance.asset_id = asset_id;
                } else {
                    graphene_assert(o.quote.balance.amount == 0, "quote init finished");
                    o.quote.balance.amount = asset_amount;
                    o.quote.balance.asset_id = asset_id;
                }
            });
        }
    }

    // @abi action
    // @abi payable
    void buy()
    {
        auto coinmarket_itr = coinmarkets.find(BANCOR_ID);
        graphene_assert(coinmarket_itr != coinmarkets.end(), "no issued");
        graphene_assert(coinmarket_itr->isready(), "no ready");

        uint64_t asset_id = get_action_asset_id();
        int64_t asset_amount = get_action_asset_amount();
        uint64_t owner = get_trx_sender();

        int64_t output;
        if (asset_id == coinmarket_itr->base.balance.asset_id) {
            coinmarkets.modify(coinmarket_itr, 0, [&](auto &o) {
                output = o.convert(contract_asset{asset_amount, asset_id}, coinmarket_itr->quote.balance.asset_id).amount;
            });

            graphene_assert(output > 0, "must reserve a positive amount");

            withdraw_asset(_self, owner, coinmarket_itr->quote.balance.asset_id, output);
        } else if (asset_id == coinmarket_itr->quote.balance.asset_id) {
            coinmarkets.modify(coinmarket_itr, 0, [&](auto &o) {
                output = o.convert(contract_asset{asset_amount, asset_id}, coinmarket_itr->base.balance.asset_id).amount;
            });

            graphene_assert(output > 0, "must reserve a positive amount");

            withdraw_asset(_self, owner, coinmarket_itr->base.balance.asset_id, output);
        } else {
            graphene_assert(false, "invalid asset");
        }
    }

  private:
    /**
     *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
     *  bancor exchange is entirely contained within this struct. There are no external
     *  side effects associated with using this API.
     */
    //@abi table coinmarket i64
    struct coinmarket {
        uint64_t bancorid;
        uint64_t supply;

        struct connector {
            contract_asset balance;
            double weight = .5;

            GRAPHENE_SERIALIZE(connector, (balance)(weight))
        };

        connector base;
        connector quote;

        uint64_t primary_key() const { return bancorid; }

        bool isready() const
        {
            return supply > 0 && base.balance.amount > 0 && quote.balance.amount > 0;
        }

        contract_asset convert_to_exchange(connector &c, contract_asset in)
        {
            double R(supply);
            double C(c.balance.amount + in.amount);
            double F(c.weight / 1000.0);
            double T(in.amount);
            double ONE(1.0);

            double E = -R * (ONE - std::pow(ONE + T / C, F));
            int64_t issued = int64_t(E);

            supply += issued;
            c.balance.amount += in.amount;

            print("convert_to_exchange: issued = ", issued, ", BANCOR_ID:", BANCOR_ID, "\n");
            return contract_asset{issued, BANCOR_ID};
        }

        contract_asset convert_from_exchange(connector &c, contract_asset in)
        {
            graphene_assert(in.asset_id == BANCOR_ID, "unexpected asset symbol input");

            double R(supply - in.amount);
            double C(c.balance.amount);
            double F(1000.0 / c.weight);
            double E(in.amount);
            double ONE(1.0);

            double T = C * (std::pow(ONE + E / R, F) - ONE);
            int64_t out = int64_t(T);

            supply -= in.amount;
            c.balance.amount -= out;

            print("convert_from_exchange: out = ", out, ", asset_id:", c.balance.asset_id, "\n");
            return contract_asset(out, c.balance.asset_id);
        }

        contract_asset convert(contract_asset from, uint64_t to)
        {
            auto sell_asset_id = from.asset_id;
            auto ex_asset_id = BANCOR_ID;
            auto base_asset_id = base.balance.asset_id;
            auto quote_asset_id = quote.balance.asset_id;

            contract_asset middle_state_asset{0, 0};
            if (sell_asset_id != ex_asset_id) {
                if (sell_asset_id == base_asset_id) {
                    auto exchange_asset = convert_to_exchange(base, from);
                    middle_state_asset.amount = exchange_asset.amount;
                    middle_state_asset.asset_id = exchange_asset.asset_id;
                } else if (sell_asset_id == quote_asset_id) {
                    auto exchange_asset = convert_to_exchange(quote, from);
                    middle_state_asset.amount = exchange_asset.amount;
                    middle_state_asset.asset_id = exchange_asset.asset_id;
                } else {
                    graphene_assert(false, "invalid sell");
                }
            } else {
                if (to == base_asset_id) {
                    auto base_asset = convert_from_exchange(base, from);
                    middle_state_asset.amount = base_asset.amount;
                    middle_state_asset.asset_id = base_asset.asset_id;
                } else if (to == quote_asset_id) {
                    auto quote_asset = convert_from_exchange(quote, from);
                    middle_state_asset.amount = quote_asset.amount;
                    middle_state_asset.asset_id = quote_asset.asset_id;
                } else {
                    graphene_assert(false, "invalid conversion");
                }
            }

            if (to != middle_state_asset.asset_id)
                return convert(middle_state_asset, to);

            return middle_state_asset;
        }

        GRAPHENE_SERIALIZE(coinmarket, (bancorid)(supply)(base)(quote))
    };

    typedef graphene::multi_index<N(coinmarket), coinmarket> coinmarket_index;

    coinmarket_index coinmarkets;
};

GRAPHENE_ABI(bancor, (issue)(buy))
