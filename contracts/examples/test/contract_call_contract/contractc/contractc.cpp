#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <../parameters.hpp>

using namespace graphene;

class contractc : public contract
{
  public:
    contractc(uint64_t uname)
        : contract(uname)
        ,tcs(_self, _self)
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

//        p p1{ca_id, cb_id, cc_id, pk, payer};
//        action b(ca_id, N(hi), std::move(p1), _self);
//        b.send();

        tcs.emplace(sender, [&pk](auto &o) {
        	o.owner = pk+1;
        });
    }

  private:
    //@abi table tc i64
    struct tc {
        uint64_t owner;
        uint64_t primary_key() const { return owner; }
        GRAPHENE_SERIALIZE(tc, (owner))
    };

    typedef graphene::multi_index<N(tc), tc> tc_index;

    tc_index tcs;
};

GRAPHENE_ABI(contractc, (hi))
