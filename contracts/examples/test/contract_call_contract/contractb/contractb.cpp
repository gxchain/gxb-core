#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <../parameters.hpp>

using namespace graphene;

class contractb : public contract
{
  public:
    contractb(uint64_t uname)
        : contract(uname)
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

        p p1{ca_id, cb_id, cc_id, pk, payer};
        action b(cc_id, N(hi), std::move(p1), _self, {100000, 1});
        b.send();
    }
};

GRAPHENE_ABI(contractb, (hi))
