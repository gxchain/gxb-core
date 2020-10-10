#include <graphenelib/dispatcher.hpp>
#include <graphenelib/contract.hpp>

#define INVALID_SENDER "Invalid sender."

using namespace graphene;

// swap合约账户.
static const int64_t SWAPACCOUNT = 5308;

class log : public contract {
    public:
        log(int64_t account_id)
            : contract(account_id) {}

        //@abi action
        void addlq(int64_t amount1, int64_t amount2, int64_t lq) {
            graphene_assert(get_trx_sender() == SWAPACCOUNT, INVALID_SENDER);
        }
};
GRAPHENE_ABI(log, (addlq))