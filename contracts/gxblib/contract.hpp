#pragma once
#include <gxblib/gxb.hpp>
#include <gxblib/asset.hpp>
#include <gxblib/multi_index.hpp>
#include <map>

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
        std::map<asset_id_type, asset> balances;

        uint64_t primary_key() const { return owner; }
    };

    typedef graphene::multi_index<N(accounts), account> account_index;

    account_index accounts;

  public:
    /// @abi action
    void subbalance(account_name owner, asset value)
    {
        if(value.amount <= 0)
            return;
        
        auto it = accounts.find(owner);
        if(it == accounts.end()) {
            print("account not found");
            return;
        }

        auto asset_it = it->balances.find(value.asset_id);
        
        if(asset_it != it->balances.end()) {
            if (asset_it->second.amount == value.amount) {
                std::map<asset_id_type, asset> &t_balances = const_cast<std::map<asset_id_type, asset>&>(it->balances);
                t_balances.erase(value.asset_id);
                if(it->balances.size() == 0) {
                    accounts.erase(it);
                }
            } else {
                accounts.modify(it, owner, [&](auto &a) {
                    a.balances[value.asset_id] -= value;
                });
            }
        } else {
            print("asset not found");
            return;
        }
        

    }

    /// @abi action
    void addbalance(account_name owner, asset value)
    {
        if(value.amount <= 0)
            return;
        
        auto it = accounts.find(owner);
        if (it == accounts.end()) {
            accounts.emplace(owner, [&](auto &a) {
                print("addbalance, ", owner, value);
                a.owner = owner;
                a.balances[value.asset_id] = value;

            });
        } else {
            print("owner exist, to modify");
            std::map<asset_id_type, asset> &t_balances = const_cast<std::map<asset_id_type, asset>&>(it->balances);
            auto asset_it = it->balances.find(value.asset_id);
            if(asset_it != it->balances.end()) {
                print("asset exist, to update");
                accounts.modify(it, 0, [&](auto &a) {
                    a.balances[value.asset_id] += value;
                });
            } else {
                print("asset not exist, to add");
                t_balances[value.asset_id] = value;
            }
                

        }
    }

    /// @abi action
    asset getbalance(account_name owner, asset_id_type asset_id) const
    {
        auto it = accounts.find(owner);
        if(it == accounts.end()) {
            print("account not found");
            return asset();
        }

        return asset();
//        return it->balances[asset_id];
    }
};

} /// namespace graphene
