#include <graphenelib/action.h>
#include <graphenelib/action.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include "../../contract_call_contract_testcases/parameters.hpp"

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
    void callself(const std::string &ccca, uint64_t cnt) {
        print("i'm contracta, function: callself, cnt=", cnt, "\n");

        struct p {
            std::string ccca;
            uint64_t cnt;
        };

        p p1{ccca, cnt+1};
        action a(ccca, N(callself), std::move(p1), _self);
        a.send();
    }

    // @abi action
    // @abi payable
    void circle(const std::string &ccca, const std::string &cccb, const std::string &cccc) {
        print("i'm contracta, function: circle\n");
        circle_params_t params{ccca, cccb, cccc};
        action act(cccb, N(circle), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void senderpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t sender = get_trx_sender();
        uint64_t origin = get_trx_origin();
        if(0 == cnt) {
            graphene_assert(sender == origin, "contracta senderpass testcase fail");
        } else if(3 == cnt) {
            int64_t accountc_id = get_account_id(cccc.c_str(), cccc.length());
            graphene_assert(sender == accountc_id, "contracta senderpass testcase fail");
            return;
        }

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(cccb, N(senderpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void senderfail(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        sender_params_t params{ccca, cccb, cccc, 0};
        action act(cccb, N(senderfail), std::move(params), 100);
        act.send();
    }

    // @abi action
    // @abi payable
    void receiverpass(const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t me = current_receiver();
        int64_t accounta_id = get_account_id(ccca.c_str(), ccca.length());
        graphene_assert(-1 != accounta_id && me == (uint64_t)accounta_id, "contracta receiverpass fail");
        if(3 == cnt)
            return;

        sender_params_t params{ccca, cccb, cccc, cnt+1};
        action act(cccb, N(receiverpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void originpass(const std::string &originacc, const std::string &ccca, const std::string &cccb, const std::string &cccc, uint64_t cnt) {
        uint64_t origin = get_trx_origin();
        int64_t originacc_id = get_account_id(originacc.c_str(), originacc.length());
        graphene_assert(-1 != originacc_id && origin == (uint64_t)originacc_id, "contracta originpass fail");

        if(3 == cnt)
            return;

        origin_params_t params{originacc, ccca, cccb, cccc, cnt+1};
        action act(cccb, N(originpass), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramadd(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool aadd, bool badd, bool cadd)
    {
        if(aadd)
            tas.emplace(0, [this](auto &o) {
                o.owner = tas.available_primary_key();
            });

        ram_params_t params{ccca, cccb, cccc, aadd, badd, cadd};
        action act(cccb, N(ramadd), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramdel(const std::string &ccca, const std::string &cccb, const std::string &cccc, bool adel, bool bdel, bool cdel, uint64_t pk)
    {
        if(adel) {
            const auto &i =tas.find(pk);
            if(i != tas.end())
                tas.erase(*i);
        }

        ram_params_t params{ccca, cccb, cccc, adel, bdel, cdel, pk};
        action act(cccb, N(ramdel), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void ramdelall(const std::string &ccca, const std::string &cccb, const std::string &cccc)
    {
        for(auto itor = tas.begin(); itor!=tas.end();){
            itor = tas.erase(itor);
        }

        circle_params_t params{ccca, cccb, cccc};
        action act(cccb, N(ramdelall), std::move(params), _self);
        act.send();
    }

    // @abi action
    // @abi payable
    void minustrans(const std::string &cccb)
    {
        action act(cccb, N(minustrans), std::move(""), _self, {-100000, 1});
        act.send();
    }

    // @abi action
    // @abi payable
    void transfer(const std::string &ccca, const std::string &cccb, const std::string &cccc)
    {
        circle_params_t params{ccca, cccb, cccc};
        action act(cccb, N(transfer), std::move(params), _self, {100000, 1});
        act.send();
    }

    // @abi action
    // @abi payable
    void actioncheck(const std::string &cccb)
    {
        circle_params_t params{"", cccb, ""};
        action act(cccb, N(actioncheck), std::move(params), _self);
        act.send();
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

GRAPHENE_ABI(contracta, (common)(callself)(circle)(senderpass)(senderfail)(receiverpass)(originpass)(ramadd)(ramdel)(ramdelall)(minustrans)(transfer)(actioncheck))
