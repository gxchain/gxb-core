#pragma once
#include <gxblib/gxb.hpp>
#include <gxblib/asset.hpp>
#include <gxblib/global.h>
#include <gxblib/system.h>
#include <gxblib/asset.h>
#include <gxblib/multi_index.hpp>

namespace graphene {

class contract
{
  public:
    contract(account_name n)
        : _self(n),
          accounts(_self, _self)
    {
    }

    inline account_name get_self() const { return _self; }

  protected:
    account_name _self;

  public:
    //@abi table account i64
    struct account {
        account_name owner;
        asset balance;

        uint64_t primary_key() const { return owner; }
    };

    typedef graphene::multi_index<N(accounts), account> account_index;

    account_index accounts;

  public:
    /// @abi action
    void deposit(account_name from, asset value)
    {
        // from must be msg.sender
        print("deposit trx origin: ", get_trx_origin(), "\n");
        print("deposit trx from: ",  from, "\n");
        print("deposit asset_id: ",  value.asset_id, ", amount:", value.amount, "\n");
        // gxb_assert(get_trx_origin() == from, "no deposit permission");
        // check amount
        // gxb_assert(get_trx_value() == value.amount);

        transfer_asset(from, _self, value.asset_id, value.amount);
        addbalance(from, value);
        print("balance: ", getbalance(from, value.asset_id), "\n");
    }

    /// @abi action
    void withdraw(account_name from, account_name to, asset value)
    {
        // from must be msg.sender
        print("withdraw trx origin: ", get_trx_origin(), "\n");
        print("withdraw trx from: ",  from, "\n");
        print("withdraw asset_id: ",  value.asset_id, ", amount:", value.amount, "\n");
        // gxb_assert(get_trx_origin() == from, "no withdraw permission");
        // check amount
        // gxb_assert(get_balance(from, value.symbol) >= value.amount);

        subbalance(from, value);
        transfer_asset(_self, to, value.asset_id, value.amount);
    }

  private:
    void subbalance(account_name owner, asset value)
    {
        auto it = accounts.find(owner);
        if(it == accounts.end()) {
            print("account not found");
            return;
        }

        if (it->balance.amount == value.amount) {
            accounts.erase(it);
        } else {
            accounts.modify(it, owner, [&](auto &a) {
                a.balance -= value;
            });
        }
    }

    void addbalance(account_name owner, asset value)
    {
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            accounts.emplace(owner, [&](auto &a) {
                print("addbalance, owner: ", owner, ", asset_id: ", value.asset_id, ", amount: ", value.amount, "\n");
                a.owner = owner;
                a.balance = value;

            });
        } else {
            print("owner exist, to modify");
            accounts.modify(it, 0, [&](auto &a) {
                a.balance += value;
            });
        }
    }

    uint64_t getbalance(account_name owner, uint64_t asset_id)
    {
        auto it = accounts.find(owner);
        if(it == accounts.end()) {
            print("account not found");
            return 0;
        }

        return it->balance.amount;
    }
};

} /// namespace graphene
