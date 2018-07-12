#pragma once
#include <gxblib/print.hpp>
#include <gxblib/serialize.hpp>
#include <gxblib/types.hpp>
#include <tuple>

namespace graphene
{

extern const int64_t scaled_precision_lut[];

struct asset {
    asset(uint64_t a = 0, uint64_t id = 0)
        : amount(a)
        , asset_id(id)
    {
    }

    uint64_t amount;
    uint64_t asset_id;

    asset &operator+=(const asset &o)
    {
        assert(asset_id == o.asset_id);
        amount += o.amount;
        return *this;
    }
    asset &operator-=(const asset &o)
    {
        assert(asset_id == o.asset_id);
        amount -= o.amount;
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

    static uint64_t scaled_precision(uint8_t precision)
    {
        assert(precision < 19);
        return scaled_precision_lut[precision];
    }

    void print() const
    {
        prints(".");
        //       prints_l( fraction, uint32_t(p) );
        prints(" ");
    }

    GXBLIB_SERIALIZE(asset, (amount)(asset_id))
};
}
