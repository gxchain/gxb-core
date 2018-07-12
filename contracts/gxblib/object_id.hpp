#pragma once
#include <boost/algorithm/string.hpp>
//#include <boost/lexical_cast.hpp>
#include <gxblib/serialize.hpp>
#include <memory>
#include <string>

#define GRAPHENE_DB_MAX_INSTANCE_ID (uint64_t(-1) >> 16)

namespace gxblib
{
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

struct object_id_type {
    object_id_type(uint8_t s, uint8_t t, uint64_t i)
    {
        assert(i >> 48 == 0);
        number = (uint64_t(s) << 56) | (uint64_t(t) << 48) | i;
    }
    object_id_type() { number = 0; }

    uint8_t space() const { return number >> 56; }
    uint8_t type() const { return number >> 48 & 0x00ff; }
    uint16_t space_type() const { return number >> 48; }
    uint64_t instance() const { return number & GRAPHENE_DB_MAX_INSTANCE_ID; }
    bool is_null() const { return number == 0; }
    explicit operator uint64_t() const { return number; }

    friend bool operator==(const object_id_type &a, const object_id_type &b) { return a.number == b.number; }
    friend bool operator!=(const object_id_type &a, const object_id_type &b) { return a.number != b.number; }
    friend bool operator<(const object_id_type &a, const object_id_type &b) { return a.number < b.number; }
    friend bool operator>(const object_id_type &a, const object_id_type &b) { return a.number > b.number; }

    object_id_type &operator++(int)
    {
        ++number;
        return *this;
    }
    object_id_type &operator++()
    {
        ++number;
        return *this;
    }

    friend object_id_type operator+(const object_id_type &a, int delta)
    {
        return object_id_type(a.space(), a.type(), a.instance() + delta);
    }
    friend object_id_type operator+(const object_id_type &a, int64_t delta)
    {
        return object_id_type(a.space(), a.type(), a.instance() + delta);
    }
    friend size_t hash_value(object_id_type v) { return std::hash<uint64_t>()(v.number); }

    template <typename T>
    bool is() const
    {
        return (number >> 48) == ((T::space_id << 8) | (T::type_id));
    }

    template <typename T>
    T as() const
    {
        FC_ASSERT(is<T>());
        return T(*this);
    }

    //    explicit operator std::string() const
    //    {
    //        return boost::lexical_cast<std::string>(space()) + "." + boost::lexical_cast<std::string>(type()) + "." + boost::lexical_cast<std::string>(instance());
    //    }

    uint64_t number;
    GXBLIB_SERIALIZE(object_id_type, (number))
};

class object;

template <uint8_t SpaceID, uint8_t TypeID, typename T = object>
struct object_id {
    typedef T type;
    static const uint8_t space_id = SpaceID;
    static const uint8_t type_id = TypeID;

    object_id() {}
    object_id(uint32_t i)
        : instance(i)
    {
    }
    explicit object_id(uint64_t i)
        : instance(i)
    {
        assert((i >> 48) == 0);
    }
    object_id(object_id_type id)
        : instance(id.instance())
    {
    }

    friend object_id operator+(const object_id a, int64_t delta) { return object_id(uint64_t(a.instance.value + delta)); }
    friend object_id operator+(const object_id a, int delta) { return object_id(uint64_t(a.instance.value + delta)); }

    //    operator object_id_type() const { return object_id_type(SpaceID, TypeID, instance.value); }
    explicit operator uint64_t() const { return object_id_type(*this).number; }

    template <typename DB>
    const T &operator()(const DB &db) const
    {
        return db.get(*this);
    }

    friend bool operator==(const object_id &a, const object_id &b) { return a.instance == b.instance; }
    friend bool operator!=(const object_id &a, const object_id &b) { return a.instance != b.instance; }
    friend bool operator==(const object_id_type &a, const object_id &b) { return a == object_id_type(b); }
    friend bool operator!=(const object_id_type &a, const object_id &b) { return a != object_id_type(b); }
    friend bool operator==(const object_id &b, const object_id_type &a) { return a == object_id_type(b); }
    friend bool operator!=(const object_id &b, const object_id_type &a) { return a != object_id_type(b); }

    friend bool operator<(const object_id &a, const object_id &b) { return a.instance.value < b.instance.value; }
    friend bool operator>(const object_id &a, const object_id &b) { return a.instance.value > b.instance.value; }

    friend size_t hash_value(object_id v) { return std::hash<uint64_t>()(v.instance.value); }

    uint32_t instance;

    GXBLIB_SERIALIZE(object_id, (instance))
};
}
