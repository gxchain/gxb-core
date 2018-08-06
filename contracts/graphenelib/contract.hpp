#pragma once
#include <stdint.h>

namespace graphene {

class contract
{
  public:
    contract(uint64_t account_id)
        : _self(account_id)
    {
    }

    inline uint64_t get_self() const { return _self; }

  protected:
    uint64_t _self;
};

}
