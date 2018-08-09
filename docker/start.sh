#!/bin/bash

APPPATH="/gxb/programs"
ARGS=""

# Translate environmental variables
if [[ "$bool_wallet" == "true" ]] || [[ -z "$bool_wallet" ]]; then
    APPPATH+="/cli_wallet/cli_wallet"
    ARGS+=""
    if [[ ! -z "$ws_server" ]]; then
        ARGS+=" -s${ws_server}"
    fi

    if [[ ! -z "$rpc_endpoint" ]]; then
        ARGS+=" -r${rpc_endpoint}"
    fi

    if [[ ! -z "$data_dir" ]]; then
        ARGS+=" --data-dir=${data_dir}"
    fi

    if [[ ! -z "$wallet_file" ]]; then
        ARGS+=" --wallet-file=${wallet_file}"
    fi
elif [[ "$bool_wallet" == "false" ]]
    APPPATH+="/witness_node/witness_node"
    ARGS+=" --enable-stale-production"
    if [[ ! -z "$data_dir" ]]; then
        ARGS+=" --data-dir=${data_dir}"
    fi

    if [[ ! -z "$rpc_endpoint" ]]; then
        ARGS+=" --rpc-endpoint=${rpc_endpoint}"
    fi

    if [[ ! -z "$witness_id" ]]; then
        ARGS+=" --witness-id=${witness_id}"
    fi

    if [[ ! -z "$seed_nodes" ]]; then
        ARGS+=" --seed-nodes=${seed_nodes}"
    fi

    if [[ ! -z "$p2p_endpoint" ]]; then
        ARGS+=" --p2p-endpoint=${p2p_endpoint}"
    fi

    if [[ ! -z "$genesis_json" ]]; then
        ARGS+=" --genesis-json=${genesis_json}"
    fi

    if [[ "$replay" == "true" ]] || [[ -z "$replay" ]]; then
        ARGS+=" --replay-blockchain"
    elif [[ "$replay" == "false" ]]; then
        ARGS+=" --resync-blockchain"
    fi
fi

echo $APPPATH
echo $ARGS
$APPPATH $ARGS
