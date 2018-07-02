# GXB-Core
---------------

GXB-Core is the GXB blockchain implementation and command-line interface.
Current binary version of the GXB-Core software for ubuntu 14.04 LTS, see [here](https://github.com/gxchain/gxb-core/releases).
Visit [gxb.io](https://gxs.gxb.io/en/) to learn about GXB.

## API Document
APIs are separated into two categories, namely
 * the Blockchain API which is used to query blockchain data (account, assets, trading history, etc.) and
 * the Wallet API which has your private keys loaded and is required when interacting with the blockchain with new transactions.

[Wallet API](https://github.com/gxchain/gxb-core/wiki/wallet_api).

[Blockchain API](https://github.com/gxchain/gxb-core/wiki/witness_node_api_json_rpc).


#### Document For Exchange
[Document For Exchange(中文)](https://github.com/gxchain/gxb-core/wiki/for_exchanges_cn)

[Document For Exchange(en)](https://github.com/gxchain/gxb-core/wiki/Instruction-for-exchanges)


#### Get Account History With Wallet API
The method ```get_irreversible_account_historys``` returns irreversible account history with txID.
In order to interface with the wallet, you need to run the CLI Wallet.
Use cli_wallet command:
```
unlocked >>> get_irreversible_account_historys <ACCOUNT> [<operation id>] <start> <limit>

```

Using an HTTP client such as curl:
```
$ curl -d '{"jsonrpc": "2.0", "method": "get_irreversible_account_historys", "params": ["1.2.17",[],1,100], "id": 1}' http://127.0.0.1:8091/rpc

```

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


## Need Help ?
---------------
Report bugs, issues using [GitHub issues](https://github.com/gxchain/gxb-core/issues/new).

## License
---------------
GXB-Core is under the GNU General Public License v3. See [LICENSE](https://github.com/gxchain/gxb-core/blob/master/LICENSE).
