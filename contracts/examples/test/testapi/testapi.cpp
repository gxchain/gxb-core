#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>

using namespace graphene;

class testapi : public contract
{
  public:
    testapi(uint64_t id)
        : contract(id)
        , pingrecords(_self, _self)
    {
    }

    /// @abi action
    void ping(uint32_t ahead_num)
    {
        uint64_t owner = get_trx_sender();
        auto pingit = pingrecords.find(owner);
        if(pingit != pingrecords.end()) {
            print("exist\n");
            return;
        } else {
            int64_t head_block_num = get_head_block_num();
            int64_t ahead_block_num = head_block_num + ahead_num;
            pingrecords.emplace(0, [&](auto &o) {
                o.owner = owner;
                o.ahead_block_num = ahead_block_num;
            });
        }
    }

    /// @abi action
    void pong()
    {
        uint64_t owner = get_trx_sender();
        auto pingit = pingrecords.find(owner);
        if(pingit != pingrecords.end()) {
            checksum160 hash;
            print("ahead_block_num = ", pingit->ahead_block_num, "\n");
            get_block_id_for_num(&hash, pingit->ahead_block_num);
            printhex((hash.hash), 20);
            pingrecords.erase(pingit);
        } else {
            print("not exist\n");
        }
    }

  private:
    //@abi table pingrecord i64
    struct pingrecord {
        uint64_t owner;
        uint32_t ahead_block_num;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(pingrecord, (owner)(ahead_block_num))
    };

    typedef multi_index<N(pingrecord), pingrecord> pingrecord_index;

    pingrecord_index pingrecords;
};

GRAPHENE_ABI(testapi, (ping)(pong))
