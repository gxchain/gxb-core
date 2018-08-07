#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class multindex : public contract
{
  public:
    multindex(uint64_t id)
        : contract(id)
        , offers(_self, _self)
    {
    }

    //@abi action
    void additem(uint64_t i1)
    {
        uint64_t pk = offers.available_primary_key();
        print("pk=", pk);
        offers.emplace(0, [&](auto &o) {
            o.id = pk;
            o.i1 = i1;
        });
    }

    //@abi action
    void getbypk(uint64_t key)
    {
        auto it = offers.find(key);
        if (it != offers.end()) {
            print("item:", it->id, ", ", it->i1);
        }
    }

    //@abi action
    void getbyi1(uint64_t key)
    {
        auto idx = offers.template get_index<N(i1)>();
        auto matched_offer_itr = idx.lower_bound(key);
        if (matched_offer_itr != idx.end()) {
            print("matched_offer_itr:", matched_offer_itr->id);
        }
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t id;
        uint64_t i1;

        uint64_t primary_key() const { return id; }

        uint64_t by_i1() const { return i1; }

        GRAPHENE_SERIALIZE(offer, (id)(i1))
    };

    typedef multi_index<N(offer), offer,
                        indexed_by<N(i1), const_mem_fun<offer, uint64_t, &offer::by_i1>>>
        offer_index;

    offer_index offers;
};

GRAPHENE_ABI(multindex, (additem)(getbypk)(getbyi1))
