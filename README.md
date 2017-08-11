# GXB-Core
---------------

GXB-Core is the GXB blockchain implementation and command-line interface.  Binary version of the GXB-Core software, see [here](https://github.com/gxchain/gxb-release/releases). Visit [gxb.io](https://www.gxb.io/) to learn about GXB.

## Getting Started
---------------

### Dependencies
A decent C++11 compiler (GNU GCC 5.4.1+). CMake version 2.8+. Boost version 1.57.0.
```
# dependencies for OS X, macOS Sierra 10.12.6 recommended
brew install openssl cmake git openssl autoconf automake doxygen autoreconfls libtool


# dependencies for ubuntu 14.04 LTS
sudo apt-get install cmake make libbz2-dev libdb++-dev libdb-dev libssl-dev openssl libreadline-dev autoconf libtool git ntp

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
