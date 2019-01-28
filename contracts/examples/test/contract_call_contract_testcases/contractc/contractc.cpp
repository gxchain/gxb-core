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
    void common()
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
    }

    // @abi action
    // @abi payable
    void circle(const std::string &ccca, const std::string &cccb, const std::string &cccc) {
        uint64_t sender = get_trx_sender();
        int64_t accountc_id = get_account_id(cccb.c_str(), cccb.length());
        graphene_assert(sender == accountc_id, "contractb senderpass testcase fail");

        print("i'm contractc, function: circle\n");
        circle_params_t params{ccca, cccb, cccc};
        action act(ccca, N(circle), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void senderpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t sender = get_trx_sender();
        int64_t accountb_id = get_account_id(cccb.c_str(), cccb.length());
        graphene_assert(sender == accountb_id, "contractc senderpass testcase fail");

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(ccca, N(senderpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void receiverpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t me = current_receiver();
        int64_t accountc_id = get_account_id(cccc.c_str(), cccc.length());
        graphene_assert(-1 != accountc_id && me == (uint64_t)accountc_id, "contractc receiverpass fail");

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(ccca, N(receiverpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void originpass(const std::string &originacc, const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t origin = get_trx_origin();
        int64_t originacc_id = get_account_id(originacc.c_str(), originacc.length());
        graphene_assert(-1 != originacc_id && origin == (uint64_t)originacc_id, "contractc originpass fail");

        origin_params_t params{originacc, ccca, cccb, cccc, cnt+1};
        action act(ccca, N(originpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramadd(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool aadd, bool badd, bool cadd)
    {
        if(cadd)
            tcs.emplace(0, [this](auto &o) {
                o.owner = tcs.available_primary_key();
            });
    }

    // @abi action
    // @abi payable
    void ramdel(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool adel, bool bdel, bool cdel, uint64_t pk)
    {
        if(cdel) {
            const auto &i =tcs.find(pk);
            if(i != tcs.end())
                tcs.erase(*i);
        }
    }

    // @abi action
    // @abi payable
    void ramdelall()
    {
        for(auto itor = tcs.begin(); itor!=tcs.end();){
            itor = tcs.erase(itor);
        }
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

GRAPHENE_ABI(contractc, (common)(circle)(senderpass)(receiverpass)(originpass)(ramadd)(ramdel)(ramdelall))
