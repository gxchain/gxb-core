#pragma once
#include <gxblib/types.h>

extern "C" {

    void deposit_asset(int64_t from, int64_t to, int64_t asset_id, int64_t amount);
    void withdraw_asset(int64_t from, int64_t to, int64_t asset_id, int64_t amount);
    int64_t get_balance(int64_t *account, int64_t *asset_id);

}
