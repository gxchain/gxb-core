#pragma once
#include <graphenelib/types.h>

extern "C" {
    // return head block num
    int64_t get_head_block_num();
    // return head block id
    void get_head_block_id(checksum160* hash);
    void get_block_id_for_num(checksum160* hash, uint32_t block_num);
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
    
    int read_transaction(char* dst, uint32_t len);
    int transaction_size();
    int expiration();
    int tapos_block_num();
    int tapos_block_prefix();
}
