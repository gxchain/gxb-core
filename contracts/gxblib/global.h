#pragma once
#include <gxblib/types.h>

extern "C" {

    int64_t get_head_block_num();

    void get_head_block_id(checksum256* hash);

    int64_t get_head_block_time();

    int64_t get_trx_sender();

    int64_t get_trx_origin();
}
