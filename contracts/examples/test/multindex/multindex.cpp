#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class multindex : public contract
{
    struct offer;

  public:
    multindex(uint64_t id)
        : contract(id)
        , offers(_self, _self)
    {
    }

    //@abi action
    void additem(uint64_t i1, uint64_t i2)
    {
        uint64_t pk = offers.available_primary_key();
        print("pk=", pk);
        offers.emplace(0, [&](auto &o) {
            o.id = pk;
            o.idx1 = i1;
            o.idx2 = i2;
        });
    }

    //@abi action
    void getbypk(uint64_t key)
    {
        auto it = offers.find(key);
        if (it != offers.end()) {
            print("item:", it->id, ", ", it->idx1);
        }
    }

    //@abi action
    void getbyidx1(uint64_t key)
    {
        auto idx = offers.template get_index<N(idx1)>();
        auto matched_offer_itr = idx.lower_bound(key);
        if (matched_offer_itr != idx.end()) {
            dump_item(*matched_offer_itr);
        }

        auto ii = idx.find(key);
        if (ii != idx.end()) {
            dump_item(*ii);
            idx.erase(ii);
        }
    }

    //@abi action
    void getbyidx2(uint64_t key)
    {
        auto idx = offers.template get_index<N(idx2)>();
        auto matched_offer_itr = idx.lower_bound(key);
        if (matched_offer_itr != idx.end()) {
            dump_item(*matched_offer_itr);
        }
    }

  private:
    void dump_item(const offer &o)
    {
        print("offer.id:", o.id, "\n");
        print("offer.idx1:", o.idx1, "\n");
        print("offer.idx2:", o.idx2, "\n");
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t id;
        uint64_t idx1;
        uint64_t idx2;

        uint64_t primary_key() const { return id; }

        uint64_t by_index1() const { return idx1; }

        uint64_t by_index2() const { return idx2; }

        GRAPHENE_SERIALIZE(offer, (id)(idx1)(idx2))
    };

    typedef multi_index<N(offer), offer,
                        indexed_by<N(idx1), const_mem_fun<offer, uint64_t, &offer::by_index1>>,
                        indexed_by<N(idx2), const_mem_fun<offer, uint64_t, &offer::by_index2>>>
        offer_index;

    offer_index offers;
};

GRAPHENE_ABI(multindex, (additem)(getbypk)(getbyidx1)(getbyidx2))
