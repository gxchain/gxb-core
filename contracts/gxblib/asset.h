#pragma once
#include <gxblib/types.h>

extern "C" {

    void transfer_asset(int64_t from, int64_t to, int64_t symbol, int64_t amount);
    int64_t get_balance(int64_t *account, int64_t *symbol);

}
