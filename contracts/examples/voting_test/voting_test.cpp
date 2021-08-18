#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>

using namespace graphene;

class voting : public contract
{
  public:
    voting(uint64_t account_id)
        : contract(account_id)
        , users_table(_self, _self)
    {
    }

    // @abi action
    void vote(bool approve)
    {
        uint64_t blocknumber = get_head_block_num();
        graphene_assert(blocknumber <= ENDLINEBLOCK, "The last time has been reached, you can not vote");
        uint64_t voter = get_trx_sender();
        auto it = users_table.find(voter);
        if (it == users_table.end()) {
            users_table.emplace(voter, [&](auto &o) {
                o.uid = voter;
                o.approve = approve;
            });
        } else {
            users_table.modify(it, voter, [&](auto &o) {
                o.approve = approve;
            });
        }
    }

  private:
    static const uint64_t ENDLINEBLOCK = 37064083;

    //@abi table users i64
    struct users {
        uint64_t uid;
        bool approve;

        uint64_t primary_key() const { return uid; }
        GRAPHENE_SERIALIZE(users, (uid) (approve))
    };
    typedef multi_index<N(users), users> users_index;

    users_index users_table;
};

GRAPHENE_ABI(voting, (vote))