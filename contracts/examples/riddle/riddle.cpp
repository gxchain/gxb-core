#include <graphenelib/graphene.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/global.h>

using namespace graphene;

class riddle : public contract
{
  public:
    riddle(uint64_t id)
        : contract(id)
        , records(_self, _self)
    {
    }

    /// @abi action
    void issue(const std::string& question, const checksum256& hashed_answer)
    {
        uint64_t owner = get_trx_sender();
        records.emplace(owner, [&](auto &p) {
                p.issuer = owner;
                p.question = question;
                p.hashed_answer = hashed_answer;
        });
    }

    /// @abi action
    void reveal(const std::string& issuer, const std::string& answer)
    {
        int64_t issuer_id = get_account_id(issuer.c_str(), issuer.size());
        graphene_assert(issuer_id >= 0, "issuer not exist");
        auto iter = records.find(issuer_id);
        graphene_assert(iter != records.end(), "no record");

        checksum256 hashed_answer;
        sha256(const_cast<char *>(answer.c_str()), answer.length(), &hashed_answer);

        if (iter->hashed_answer == hashed_answer) {
            print("reveal success! \n");
            records.erase(iter);
        } else {
            print("answer is wrong! \n");
        }
    }

  private:
    // @abi table record i64
    struct record {
        uint64_t            issuer;
        std::string         question;
        checksum256         hashed_answer;

        uint64_t primary_key() const { return issuer; }

        GRAPHENE_SERIALIZE(record, (issuer)(question)(hashed_answer))
    };

    typedef graphene::multi_index<N(record), record> record_index;
    record_index        records;
};

GRAPHENE_ABI(riddle, (issue)(reveal))
