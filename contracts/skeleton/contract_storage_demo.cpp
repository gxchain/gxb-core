#include <gxblib/gxb.hpp>
#include <gxblib/multi_index.hpp>
#include <string>

using namespace gxblib;
using namespace graphene;
using std::string;

//TODO FIXME int and float type can not compile by gxbcpp

class contract_storage_demo : public graphene::contract
{
  public:
    using contract::contract;

    contract_storage_demo(account_name self)
        : graphene::contract(self)
        , cpus(_self, _self)
    {
    }

    /// @abi action
    void store(uint64_t id, string manufactor, string name, uint64_t frequency)
    {
        cpus.emplace(_self, [&](auto &cpu) {
            cpu.id = id;
            cpu.manufactor = manufactor;
            cpu.name = name;
            cpu.frequency = frequency;
        });
    }

    /// @abi action
    void find(uint64_t id)
    {
        auto cpu_itr = cpus.find(id);
        if (cpu_itr == cpus.end()) {
            print("can not find this cpu");
        } else {
            print("cpu.name = ${name}", ("name", cpu_itr->name));
        }
    }

  private:
    //@abi table cpu i64
    struct cpu {
        uint64_t    id;
        string      manufactor;
        string      name;
        uint64_t    frequency;
        
        uint64_t primary_key() { return id; }

        GXBLIB_SERIALIZE(cpu, (id)(manufactor)(name)(frequency));
    };

    typedef graphene::multi_index<N(cpu), cpu> cpu_index;

    cpu_index cpus;
};

GXB_ABI(contract_storage_demo, (store)(find))
