# GXB-Core

GXB-Core is the GXChain implementation and command-line interface.
Current binary version of the GXB-Core software for ubuntu 14.04 LTS, see [here](https://github.com/gxchain/gxb-core/releases).

<p>
   <a href="https://github.com/gxchain/gxips">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-gxips-en.png'/>
   </a>
   <a href="https://github.com/gxchain/gxclient-node">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-gxclient-en.png'/>
   </a>
   <a href="https://github.com/gxchain/gxchain-wallet">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-mwallet-en.png'/>
   </a>
</p>
<p>
   <a href="https://github.com/gxchain/gxchain-light">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-wallet-en.png'/>
   </a>
   <a href="https://github.com/gxchain/docs">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-docs-en.png'/>
   </a>
   <a href="https://github.com/gxchain/gxc-smart-contract-ide">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-ide-en.png'/>
   </a>
</p>

---

<p>
   <a href="https://github.com/gxchain/gxips">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-gxips.png'/>
   </a>
   <a href="https://github.com/gxchain/gxclient-node">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-gxclient.png'/>
   </a>
   <a href="https://github.com/gxchain/gxchain-wallet">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-mwallet.png'/>
   </a>
</p>
<p>
   <a href="https://github.com/gxchain/gxchain-light">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-wallet.png'/>
   </a>
   <a href="https://github.com/gxchain/docs">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-docs.png'/>
   </a>
   <a href="https://github.com/gxchain/gxc-smart-contract-ide">
      <img width="200px" src='https://raw.githubusercontent.com/gxchain/gxips/master/assets/images/task-ide.png'/>
   </a>
</p>

## Docs
Technical document is available [here](https://docs.gxchain.org/)


#### Document For Exchange
1. [Document For Exchange(中文)](https://github.com/gxchain/gxb-core/wiki/for_exchanges_cn)

2. [Document For Exchange(en)](https://github.com/gxchain/gxb-core/wiki/Instruction-for-exchanges)


## Getting Started

#### Building

 1. [Build on Ubuntu](https://github.com/gxchain/gxb-core/wiki/BUILD_UBUNTU)
 2. [Build on OS X](https://github.com/gxchain/gxb-core/wiki/BUILD_OS_X)
 3. [Build on CentOS7](https://github.com/gxchain/gxb-core/wiki/BUILD_CENTOS7)

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
unlocked >>> deploy_contract helloworld nathan 0 0 ./helloworld GXC true
```

#### Call Contract
```
unlocked >>> call_contract nathan helloworld null hi "{\"user\":\"albert\"}" GXC true

```

Smart Contract QuickStart for testnet, see [here](https://github.com/gxchain/Technical-Documents/blob/master/gxchain_contract_start.md)

## Support
---------------
Report bugs, issues using [GitHub issues](https://github.com/gxchain/gxb-core/issues/new?template=bug_report.md).

Technical support is also available in the [GXS forum](https://forum.gxb.io/category/3/for-developers-%E5%BC%80%E5%8F%91%E8%80%85%E4%B8%93%E5%8C%BA)



## License
---------------
GXB-Core is under the GNU General Public License v3. See [LICENSE](https://github.com/gxchain/gxb-core/blob/master/LICENSE).
