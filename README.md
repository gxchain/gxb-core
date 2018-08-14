# GXB-Core
---------------

GXB-Core is the GXChain implementation and command-line interface.
Current binary version of the GXB-Core software for ubuntu 14.04 LTS, see [here](https://github.com/gxchain/gxb-core/releases).

## API Document
APIs are separated into two categories, namely
 * the Blockchain API which is used to query blockchain data (account, assets, trading history, etc.) and
 * the Wallet API which has your private keys loaded and is required when interacting with the blockchain with new transactions.

1. [Wallet API](https://github.com/gxchain/gxb-core/wiki/wallet_api).

2. [Blockchain API](https://github.com/gxchain/gxb-core/wiki/witness_node_api_json_rpc).


#### Document For Exchange
1. [Document For Exchange(中文)](https://github.com/gxchain/gxb-core/wiki/for_exchanges_cn)

2. [Document For Exchange(en)](https://github.com/gxchain/gxb-core/wiki/Instruction-for-exchanges)


## Getting Started
---------------

#### Building

 1. [Build on Ubuntu](https://github.com/gxchain/gxb-core/wiki/BUILD_UBUNTU)
 2. [Build on OS X](https://github.com/gxchain/gxb-core/wiki/BUILD_OS_X)

#### Running
After building, the witness node can be launched with:
```
    ./programs/witness_node/witness_node --rpc-endpoint="127.0.0.1:8090" --max-ops-per-account=0 --partial-operations=true --data-transaction-lifetime=1
```
The node will automatically create a data directory including a config file. It may take several minutes to fully synchronize
the blockchain.

After starting the witness node, in a separate terminal you can run cli_wallet:
```
 ./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090
```
Set your inital password:
```
new >>> set_password <PASSWORD>
locked >>> unlock <PASSWORD>
```
To import your wif_key(active key):
```
unlocked >>> import_key <ACCOUNT NAME> [<WIF_KEY>] true
```
Import balances:
```
unlocked >>> import_balance <ACCOUNT NAME> [<WIF_KEY>] true
```
Transferring Currency:
```
unlocked >>> transfer <FROM ACCOUNT> <TO ACCOUNT> 100 GXC "" true
```

If you send private keys over this connection, `rpc-endpoint` should be bound to localhost for security.

Use `help` to see all available wallet commands.

## Smart Contract Getting Started
---------------

#### Create Contract

create contract use gxx:
```
gxx -n helloworld
```

build contract:
```
gxx -g helloworld/helloworld.abi helloworld/helloworld.cpp
```

generate wast:
```
gxx -o helloworld/helloworld.wast helloworld/helloworld.cpp
```

generate abi:
```
gxx -g helloworld/helloworld.abi helloworld/helloworld.cpp
```


#### Deploy Contract
You can deploy and call contract with cli_wallet.

```
unlocked >>> deploy_contract helloworld nathan 0 0 ./helloworld GXS true
```

#### Call Contract
```
unlocked >>> call_contract nathan helloworld null hi "{\"user\":\"albert\"}" GXS true

```

Smart Contract QuickStart for testnet, see [here](https://github.com/gxchain/Technical-Documents/blob/master/gxchain_contract_start.md)

## Support
---------------
Report bugs, issues using [GitHub issues](https://github.com/gxchain/gxb-core/issues/new).

Technical support is also available in the [GXS forum](https://forum.gxb.io/category/3/for-developers-%E5%BC%80%E5%8F%91%E8%80%85%E4%B8%93%E5%8C%BA)



## License
---------------
GXB-Core is under the GNU General Public License v3. See [LICENSE](https://github.com/gxchain/gxb-core/blob/master/LICENSE).
