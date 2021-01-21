#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>

using namespace graphene;

static const int64_t BLACKHOLEACCOUNT = 3;
static const int64_t ADMINACCOUNT = 22;

class nft : public contract
{
  public:
    nft(uint64_t account_id)
        : contract(account_id)
        , tokens(_self, _self)
        , accounts(_self, _self)
    {
    }

    //@abi action
    void mint(uint64_t to, uint64_t id, std::string name, std::string link, std::string extra)
    {
        auto sender = get_trx_sender();
        graphene_assert(sender == ADMINACCOUNT, "You do not have permission");
        graphene_assert(_exists(id) == false, "The id is existed");
        tokens.emplace(sender, [&](auto &t) {
            t.id = id;
            t.name = name;
            t.link = link;
            t.extra = extra;
            t.owner = to;
            t.approve = BLACKHOLEACCOUNT;
        });
        auto account_itr = accounts.find(to);
        if (account_itr == accounts.end()) {
            accounts.emplace(sender, [&](auto &a) {
                a.owner = to;
                a.ids.insert(id);
            });
        } else {
            accounts.modify(*account_itr, sender, [&](auto &a) {
                a.ids.insert(id);
            });
        }
    }

    //@abi action
    void approve(uint64_t id, uint64_t to)
    {
        auto sender = get_trx_sender();
        graphene_assert(sender != to, "You can not approve to yourself");
        graphene_assert(_exists(id) == true, "The id is not existed");
        auto token_itr = tokens.find(id);
        auto owner = token_itr->owner;
        graphene_assert(owner == sender || _isallapproved(owner, sender), "You do not have permission");
        tokens.modify(*token_itr, sender, [&](auto &t) {
            t.approve = to;
        });
    }

    //@abi action
    void transfer(uint64_t id, uint64_t to)
    {
        auto sender = get_trx_sender();
        graphene_assert(_exists(id) == true, "The id is not existed");
        graphene_assert(sender != to, "You can not transfer to yourself");
        auto token_itr = tokens.find(id);
        uint64_t owner = token_itr->owner;
        graphene_assert(owner == sender || _isallapproved(owner, sender) || _isapproved(id, sender), "You do not have permission");
        tokens.modify(*token_itr, sender, [&](auto &t) {
            t.owner = to;
            t.approve = BLACKHOLEACCOUNT;
        });
        auto old_itr = accounts.find(owner);
        auto new_itr = accounts.find(to);
        accounts.modify(*old_itr, sender, [&](auto &a) {
            a.ids.erase(id);
        });
        if (old_itr->ids.empty() && old_itr->allowance.empty()) {
            accounts.erase(old_itr);
        }
        if (new_itr == accounts.end()) {
            accounts.emplace(sender, [&](auto &a) {
                a.owner = to;
                a.ids.insert(id);
            });
        } else {
            accounts.modify(*new_itr, sender, [&](auto &a) {
                a.ids.insert(id);
            });
        }
    }

    //@abi action
    void burn(uint64_t id)
    {
        auto sender = get_trx_sender();
        graphene_assert(_exists(id) == true, "The id is not existed");
        auto token_itr = tokens.find(id);
        uint64_t owner = token_itr->owner;
        graphene_assert(owner == sender, "You do not have permission");
        tokens.erase(token_itr);
        auto old_itr = accounts.find(owner);
        accounts.modify(*old_itr, sender, [&](auto &a) {
            a.ids.erase(id);
        });
        if (old_itr->ids.empty() && old_itr->allowance.empty()) {
            accounts.erase(old_itr);
        }
    }

    //@abi action
    void approveall(uint64_t to)
    {
        auto sender = get_trx_sender();
        graphene_assert(sender != to, "You can not approve to yourself");
        graphene_assert(_isallapproved(sender, to) == false, "The account has been approved");
        auto owner_itr = accounts.find(sender);
        accounts.modify(*owner_itr, sender, [&](auto &a) {
            a.allowance.insert(to);
        });
    }

    //@abi action
    void appallremove(uint64_t to)
    {
        auto sender = get_trx_sender();
        graphene_assert(_isallapproved(sender, to) == true, "The account has not been approved");
        auto owner_itr = accounts.find(sender);
        accounts.modify(*owner_itr, sender, [&](auto &a) {
            a.allowance.erase(to);
        });
        if (owner_itr->ids.empty() && owner_itr->allowance.empty()) {
            accounts.erase(owner_itr);
        }
    }

  private:
    bool _isapproved(uint64_t id, uint64_t account)
    {
        graphene_assert(_exists(id) == true, "The id is not existed");
        auto token_itr = tokens.find(id);
        uint64_t approveid = token_itr->approve;
        return approveid == account;
    }

    bool _isallapproved(uint64_t owner, uint64_t _operator)
    {
        auto account_itr = accounts.find(owner);
        graphene_assert(account_itr != accounts.end(), "the account do not have nft");
        auto approve_itr = account_itr->allowance.find(_operator);
        return approve_itr != account_itr->allowance.end();
    }

    bool _exists(uint64_t id)
    {
        auto token_itr = tokens.find(id);
        return token_itr != tokens.end();
    }

    //@abi table token i64
    struct token {
        uint64_t id;
        std::string name;
        std::string link;
        std::string extra;
        uint64_t owner;
        uint64_t approve;
        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(token, (id)(name)(link)(extra)(owner)(approve))
    };
    typedef graphene::multi_index<N(token), token> token_index;
    token_index tokens;

    //@abi table account i64
    struct account {
        uint64_t owner;
        std::set<uint64_t> ids;
        std::set<uint64_t> allowance;
        uint64_t primary_key() const { return owner; }
        GRAPHENE_SERIALIZE(account, (owner)(ids)(allowance))
    };
    typedef graphene::multi_index<N(account), account> account_index;
    account_index accounts;
};
GRAPHENE_ABI(nft, (mint)(approve)(transfer)(burn)(approveall)(appallremove))