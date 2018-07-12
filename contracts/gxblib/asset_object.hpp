#pragma once
#include <gxblib/object.hpp>
#include <gxblib/types.hpp>
#include <stdint.h>

namespace gxblib
{

using std::string;

class asset_object : public gxblib::abstract_object<asset_object>
{
  public:
    static const uint8_t space_id = 1;
    static const uint8_t type_id = 3;

    string symbol;
    uint8_t precision = 0;
};
}
