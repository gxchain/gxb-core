#pragma once
#include <gxblib/print.hpp>
#include <gxblib/serialize.hpp>
#include <gxblib/system.h>
#include <gxblib/types.hpp>
#include <tuple>

namespace graphene {

extern const int64_t scaled_precision_lut[];

struct contract_asset {
    contract_asset(int64_t a = 0, uint64_t id = 0)
        : amount(a)
        , asset_id(id & GRAPHENE_DB_MAX_INSTANCE_ID)
    {
        gxb_assert(is_amount_within_range(), "magnitude of asset amount must be less than 2^62");
    }

    int64_t     amount;
    uint64_t    asset_id;

    static constexpr int64_t max_amount = (1LL << 62) - 1;

    bool is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

    contract_asset &operator+=(const contract_asset &o)
    {
        gxb_assert(asset_id == o.asset_id, "asset_id invalid");
        amount += o.amount;
        gxb_assert(-max_amount <= amount, "subtraction underflow");
        gxb_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset &operator-=(const contract_asset &o)
    {
        gxb_assert(asset_id == o.asset_id, "asset_id invalid");
        amount -= o.amount;
        gxb_assert(-max_amount <= amount, "subtraction underflow");
        gxb_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset operator-() const { return contract_asset(-amount, asset_id); }

    contract_asset &operator*=(int64_t a)
    {
        gxb_assert(a == 0 || (amount * a) / a == amount, "multiplication overflow or underflow");
        amount *= a;
        gxb_assert(-max_amount <= amount, "multiplication underflow");
        gxb_assert(amount <= max_amount, "multiplication overflow");
        return *this;
    }

    friend contract_asset operator*(const contract_asset &a, int64_t b)
    {
        contract_asset result = a;
        result *= b;
        return result;
    }

    friend contract_asset operator*(int64_t b, const contract_asset &a)
    {
        contract_asset result = a;
        result *= b;
        return result;
    }

    contract_asset &operator/=(int64_t a)
    {
        amount /= a;
        return *this;
    }

    friend contract_asset operator/(const contract_asset &a, int64_t b)
    {
        contract_asset result = a;
        result /= b;
        return result;
    }


    friend bool operator==(const contract_asset &a, const contract_asset &b)
    {
        return std::tie(a.asset_id, a.amount) == std::tie(b.asset_id, b.amount);
    }
    friend bool operator<(const contract_asset &a, const contract_asset &b)
    {
        gxb_assert(a.asset_id == b.asset_id, "asset_id invalid");
        return a.amount < b.amount;
    }
    friend bool operator<=(const contract_asset &a, const contract_asset &b)
    {
        return (a == b) || (a < b);
    }

    friend bool operator!=(const contract_asset &a, const contract_asset &b)
    {
        return !(a == b);
    }
    friend bool operator>(const contract_asset &a, const contract_asset &b)
    {
        return !(a <= b);
    }
    friend bool operator>=(const contract_asset &a, const contract_asset &b)
    {
        return !(a < b);
    }

    friend contract_asset operator-(const contract_asset &a, const contract_asset &b)
    {
        gxb_assert(a.asset_id == b.asset_id, "asset_id invalid");
        return contract_asset(a.amount - b.amount, a.asset_id);
    }
    friend contract_asset operator+(const contract_asset &a, const contract_asset &b)
    {
        gxb_assert(a.asset_id == b.asset_id, "asset_id invalid");
        return contract_asset(a.amount + b.amount, a.asset_id);
    }

    static int64_t scaled_precision(uint8_t precision)
    {
        gxb_assert(precision < 19, "precision invalid");
        return scaled_precision_lut[precision];
    }

    GXBLIB_SERIALIZE(contract_asset, (amount)(asset_id))
};

}
