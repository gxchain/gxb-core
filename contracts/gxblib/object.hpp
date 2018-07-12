#pragma once
#include <memory>
#include <stdint.h>
#include <gxblib/object_id.hpp>

namespace gxblib
{

using std::shared_ptr;
using std::unique_ptr;
using std::vector;

class object
{
  public:
    object() {}
    virtual ~object() {}

    static const uint8_t space_id = 0;
    static const uint8_t type_id = 0;

    object_id_type id;

    virtual unique_ptr<object> clone() const = 0;
    virtual void move_from(object &obj) = 0;
    virtual vector<char> pack() const = 0;
};

template <typename DerivedClass>
class abstract_object : public object
{
  public:
    virtual unique_ptr<object> clone() const
    {
        return unique_ptr<object>(new DerivedClass(*static_cast<const DerivedClass *>(this)));
    }

    virtual void move_from(object &obj)
    {
        static_cast<DerivedClass &>(*this) = std::move(static_cast<DerivedClass &>(obj));
    }
//    virtual vector<char> pack() const { return fc::raw::pack(static_cast<const DerivedClass &>(*this)); }
};
}
