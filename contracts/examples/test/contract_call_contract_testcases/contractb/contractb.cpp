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
        , tbs(_self, _self)
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
        print("i'm contractb, function: circle\n");
        circle_params_t params{ccca, cccb, cccc};
        action act(cccc, N(circle), std::move(params), _self);
        act.send();
    }

    // @abi action
    void senderpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t sender = get_trx_sender();
        int64_t accountc_id = get_account_id(ccca.c_str(), ccca.length());
        graphene_assert(sender == accountc_id, "contractb senderpass testcase fail");

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(cccc, N(senderpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void senderfail(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
    }

    // @abi action
    // @abi payable
    void receiverpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t me = current_receiver();
        int64_t accountb_id = get_account_id(cccb.c_str(), cccb.length());
        graphene_assert(-1 != accountb_id && me == (uint64_t)accountb_id, "contractb receiverpass fail");

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(cccc, N(receiverpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void originpass(const std::string &originacc, const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t origin = get_trx_origin();
        int64_t originacc_id = get_account_id(originacc.c_str(), originacc.length());
        graphene_assert(-1 != originacc_id && origin == (uint64_t)originacc_id, "contractb originpass fail");

        origin_params_t params{originacc, ccca, cccb, cccc, cnt+1};
        action act(cccc, N(originpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void minustrans(const std::string &cccb)
    {
    }

    // @abi action
    // @abi payable
    void ramadd(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool aadd, bool badd, bool cadd)
    {
        if(badd)
            tbs.emplace(0, [this](auto &o) {
                o.owner = tbs.available_primary_key();
            });

        ram_params_t params{ccca, cccb, cccc, aadd, badd, cadd};
        action act(cccc, N(ramadd), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramdel(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool adel, bool bdel, bool cdel, uint64_t pk)
    {
        if(bdel) {
            const auto &i =tbs.find(pk);
            if(i != tbs.end())
                tbs.erase(*i);
        }

        ram_params_t params{ccca, cccb, cccc, adel, bdel, cdel, pk};
        action act(cccc, N(ramdel), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramdelall(const std::string &ccca, const std::string &cccb, const std::string &cccc)
    {
        for(auto itor = tbs.begin(); itor!=tbs.end();){
            itor = tbs.erase(itor);
        }

        action act(cccc, N(ramdelall), std::move(""), _self);
        act.send();
    }

    // @abi action
    void transfer(const std::string &ccca, const std::string &cccb, const std::string &cccc)
    {
    }

  private:
    //@abi table tb i64
    struct tb {
        uint64_t owner;
        uint64_t primary_key() const { return owner; }
        GRAPHENE_SERIALIZE(tb, (owner))
    };

    typedef graphene::multi_index<N(tb), tb> tb_index;

    tb_index tbs;
};

GRAPHENE_ABI(contractb, (common)(circle)(senderpass)(senderfail)(receiverpass)(originpass)(minustrans)(ramadd)(ramdel)(ramdelall)(transfer))
