#pragma once
#include <gxblib/print.hpp>
#include <gxblib/serialize.hpp>
#include <gxblib/system.h>
#include <gxblib/types.hpp>
#include <tuple>

namespace graphene {

extern const int64_t scaled_precision_lut[];
#define GRAPHENE_DB_MAX_INSTANCE_ID  (uint64_t(-1)>>16)

struct asset {
    asset(int64_t a = 0, uint64_t id = 0)
        : amount(a)
        , asset_id(id & GRAPHENE_DB_MAX_INSTANCE_ID)
    {
        gxb_assert(is_amount_within_range(), "magnitude of asset amount must be less than 2^62");
    }

    int64_t     amount;
    uint64_t    asset_id;

    static constexpr int64_t max_amount = (1LL << 62) - 1;

    bool is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

    asset &operator+=(const asset &o)
    {
        assert(asset_id == o.asset_id);
        amount += o.amount;
        gxb_assert(-max_amount <= amount, "subtraction underflow");
        gxb_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    asset &operator-=(const asset &o)
    {
        assert(asset_id == o.asset_id);
        amount -= o.amount;
        gxb_assert(-max_amount <= amount, "subtraction underflow");
        gxb_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    asset operator-() const { return asset(-amount, asset_id); }

    friend bool operator==(const asset &a, const asset &b)
    {
        return std::tie(a.asset_id, a.amount) == std::tie(b.asset_id, b.amount);
    }
    friend bool operator<(const asset &a, const asset &b)
    {
        assert(a.asset_id == b.asset_id);
        return a.amount < b.amount;
    }
    friend bool operator<=(const asset &a, const asset &b)
    {
        return (a == b) || (a < b);
    }

    friend bool operator!=(const asset &a, const asset &b)
    {
        return !(a == b);
    }
    friend bool operator>(const asset &a, const asset &b)
    {
        return !(a <= b);
    }
    friend bool operator>=(const asset &a, const asset &b)
    {
        return !(a < b);
    }

    friend asset operator-(const asset &a, const asset &b)
    {
        assert(a.asset_id == b.asset_id);
        return asset(a.amount - b.amount, a.asset_id);
    }
    friend asset operator+(const asset &a, const asset &b)
    {
        assert(a.asset_id == b.asset_id);
        return asset(a.amount + b.amount, a.asset_id);
    }

    static int64_t scaled_precision(uint8_t precision)
    {
        assert(precision < 19);
        return scaled_precision_lut[precision];
    }

    GXBLIB_SERIALIZE(asset, (amount)(asset_id))
};

}
