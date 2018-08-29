#pragma once
#include <graphenelib/types.h>

extern "C" {
    // return head block num
    int64_t get_head_block_num();
    // return head block id
    void get_head_block_id(checksum160* hash);
    // return head block time
    int64_t get_head_block_time();
    // return trx sender
    int64_t get_trx_sender();
    // return original trx sender
    int64_t get_trx_origin();
    // get account_id by name
    int64_t get_account_id(const char *data, uint32_t length);
    // get asset_id by symbol
    int64_t get_asset_id(const char *data, uint32_t length);
}
