#pragma once
#include <gxblib/gxb.hpp>
#include <gxblib/asset.hpp>
#include <gxblib/multi_index.hpp>

namespace graphene {

class contract
{
  public:
    contract(account_name n)
        : _self(n)
    {
    }

    inline account_name get_self() const { return _self; }

  protected:
    account_name _self;

  public:
    struct account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol; }
    };

    typedef graphene::multi_index<N(accounts), account> accounts;

  public:
    /// @abi action
    void subbalance(account_name owner, asset value)
    {
        accounts from_acnts(_self, owner);

        const auto &from = from_acnts.get(value.symbol, "no balance object found");
        // eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

        if (from.balance.amount == value.amount) {
            from_acnts.erase(from);
        } else {
            from_acnts.modify(from, owner, [&](auto &a) {
                a.balance -= value;
            });
        }
    }

    /// @abi action
    void addbalance(account_name owner, asset value, account_name ram_payer)
    {
        accounts to_acnts(_self, owner);
        auto to = to_acnts.find(value.symbol);
        if (to == to_acnts.end()) {
            to_acnts.emplace(ram_payer, [&](auto &a) {
                print("addbalance, ", owner, value);
                a.balance = value;
            });
        } else {
            to_acnts.modify(to, 0, [&](auto &a) {
                a.balance += value;
            });
        }
    }

    /// @abi action
    asset getbalance(account_name owner, symbol_name sym) const
    {
        accounts accountstable(_self, owner);
        const auto &ac = accountstable.get(sym);
        return ac.balance;
    }
};

} /// namespace graphene
