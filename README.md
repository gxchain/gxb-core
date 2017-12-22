# GXB-Core
---------------

GXB-Core is the GXB blockchain implementation and command-line interface.
Current binary version of the GXB-Core software for ubuntu 14.04 LTS, see [here](https://github.com/gxchain/gxb-core/releases).
Visit [gxb.io](https://www.gxb.io/) to learn about GXB.

## API Document
APIs are separated into two categories, namely
 * the Blockchain API which is used to query blockchain data (account, assets, trading history, etc.) and
 * the Wallet API which has your private keys loaded and is required when interacting with the blockchain with new transactions.

[Wallet API](https://github.com/gxchain/gxb-core/wiki/wallet_api).

[Blockchain API](https://github.com/gxchain/gxb-core/wiki/witness_node_api_json_rpc).


### Document For Exchange
[Document For Exchange(中文)](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/doc/%E4%BA%A4%E6%98%93%E6%89%80%E6%8E%A5%E5%85%A5%E6%96%87%E6%A1%A3.pdf)

[Document For Exchange(en)](https://github.com/gxchain/gxb-core/wiki/Instruction-for-exchanges)


### Get Account History With Wallet API
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

### Dependencies
A decent C++11 compiler (GNU GCC 5.4.1+ on ubuntu, or Apple LLVM version 8.1.0 (clang-802.0.42) on MacOS). CMake version 2.8+. Boost version 1.57.0.
The repository contains the install scripts for gcc5 and boost 1.57.0, see [here](https://github.com/gxchain/gxb-core/tree/master/script).
```
# dependencies for OS X, macOS Sierra 10.12.6 recommended
brew install openssl cmake git openssl autoconf automake doxygen autoreconfls libtool

# dependencies for ubuntu 14.04 LTS
sudo apt-get install cmake make python-dev libbz2-dev libdb++-dev libdb-dev libssl-dev openssl libreadline-dev autoconf libtool git ntp

```
**NOTE**: GXB-Core requires an OpenSSL version in the 1.0.x series. If your system OpenSSL version is newer, then you will need to manually provide an older version of OpenSSL and specify it to CMake using -DOPENSSL_INCLUDE_DIR, -DOPENSSL_SSL_LIBRARY, and -DOPENSSL_CRYPTO_LIBRARY.

#### Building

To build after all dependencies are installed:

    git clone https://github.com/gxchain/gxb-core.git
    cd gxb-core
    git checkout <LATEST_RELEASE_TAG>
    git submodule update --init --recursive
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
    make

Alternate Boost versions can be specified using the `DBOOST_ROOT` CMake argument.

### Running
After building, the witness node can be launched with:
```
    ./programs/witness_node/witness_node --rpc-endpoint="127.0.0.1:8090"
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
