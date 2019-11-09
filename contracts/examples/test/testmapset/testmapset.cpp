#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <map>

using namespace graphene;

class testmapset : public contract
{
  public:
    testmapset(uint64_t id)
        : contract(id), mapsettabs(_self, _self)
    {
    }

    /// @abi action
    void mapaction(std::map<std::string,uint64_t> mappar)
    {
        auto itor = mappar.begin();
        auto str = itor->first;
        auto num = itor->second;
        for (int i = 0; i < 1; ++i) {
            print("str:",str,"\n");
            print("num:",num,"\n");

        }
        uint64_t pk = mapsettabs.available_primary_key();
        mapsettabs.emplace(0, [&](auto &o) {
            o.id = pk;
            o.mymap = mappar;
        });
    }

    // @abi action
    void setaction(std::set<std::string> setpar)
    {
        auto itor = setpar.begin();
        print("set: ",*itor,"\n");

        uint64_t pk = mapsettabs.available_primary_key();
        mapsettabs.emplace(0, [&](auto &o) {
            o.id = pk;
            o.myset = setpar;
        });
    }

    //@abi table mapsettab i64
    struct mapsettab {
        uint64_t id;
        std::map<std::string,uint64_t> mymap;
        std::set<std::string> myset;

        uint64_t primary_key() const { return id; }

        GRAPHENE_SERIALIZE(mapsettab, (id)(mymap)(myset))
    };

    typedef graphene::multi_index<N(mapsettab), mapsettab> mapset_index;
    mapset_index mapsettabs;

};

GRAPHENE_ABI(testmapset, (mapaction)(setaction))
