#pragma once

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

  private:
    struct account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.name(); }
    };

    typedef gxblib::multi_index<N(accounts), account> accounts;

  protected:
    void sub_balance(account_name owner, asset value)
    {
        accounts from_acnts(_self, owner);

        const auto &from = from_acnts.get(value.symbol.name(), "no balance object found");
        eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

        if (from.balance.amount == value.amount) {
            from_acnts.erase(from);
        } else {
            from_acnts.modify(from, owner, [&](auto &a) {
                a.balance -= value;
            });
        }
    }

    void add_balance(account_name owner, asset value, account_name ram_payer)
    {
        accounts to_acnts(_self, owner);
        auto to = to_acnts.find(value.symbol.name());
        if (to == to_acnts.end()) {
            to_acnts.emplace(ram_payer, [&](auto &a) {
                a.balance = value;
            });
        } else {
            to_acnts.modify(to, 0, [&](auto &a) {
                a.balance += value;
            });
        }
    }

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
       accounts accountstable(_self, owner);
       const auto &ac = accountstable.get(sym);
       return ac.balance;
   }
};

} /// namespace graphene
