#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <../parameters.hpp>

using namespace graphene;

class contracta : public contract
{
  public:
    contracta(uint64_t uname)
        : contract(uname)
        , tas(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void hi(uint64_t ca_id, uint64_t cb_id, uint64_t cc_id, uint64_t pk, uint64_t payer)
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t sender = get_trx_sender();
        uint64_t origin = get_trx_origin();
        uint64_t me = current_receiver();
        print("asset_amount=", asset_amount, "\n");
        print("asset_id=", asset_id, "\n");
        print("sender=", sender, "\n");
        print("origin=", origin, "\n");
        print("current_contract=", me, "\n");

        tas.emplace(0, [&pk](auto &o) {
        	o.owner = pk+1;
        });

        p p1{ca_id, cb_id, cc_id, pk, payer};
        action b(cb_id, N(hi), std::move(p1), _self, {300000, 1});
        b.send();

        tas.emplace(sender, [&pk](auto &o) {
        	o.owner = pk+2;
        });

        tas.emplace(payer, [&pk](auto &o) {
        	o.owner = pk+3;
        });
    }

    // @abi action
    void delbykey(uint64_t pk)
    {
        const auto &i =tas.find(pk);
        if(i != tas.end())
            tas.erase(*i);
    }

  private:
    //@abi table ta i64
    struct ta {
        uint64_t owner;
        uint64_t primary_key() const { return owner; }
        GRAPHENE_SERIALIZE(ta, (owner))
    };

    typedef graphene::multi_index<N(ta), ta> ta_index;

    ta_index tas;
};

GRAPHENE_ABI(contracta, (hi)(delbykey))
