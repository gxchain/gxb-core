# For Exchanges

## 1. Introduction

Connect GXB wallet to exchanges is same as bitshares(BTS),both witness_node and cli_wallet are required to run.

witness_node
Connect GXChain in a peer-to-peer manner, receiving updated block from the network, and broadcast locally signed transaction pack.

cli_wallet
Connect witness_node through websocket and managing wallet files. Supporting transaction signature, broadcast through witness_node, and provide API via http rpc for other progarms.

## 2.System requirements

Ubuntu 14.04 LTS 64-bit
RAM: 8GB.
Free hard disk space: 50GB.
Dependency Package: not required
witness_node require system calibrate current time, NTP service need to be installed：

```bash
sudo apt-get install ntp
```
## 3. Deploy and start
### 1. Installation package download

Choose either link below to install

|github| https://github.com/gxchain/gxb-core/releases/download/1.0.171031/gxb_1.0.171031.tar.gz |
|Alibaba Cloud| http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/gxb_1.0.171031.tar.gz |


### 2. Unzip package

Put the files in your deploy catalog, then execute：

```bash
tar zxvf gxb_1.0.171031.tar.gz
```

### 3. Start witness_node and synchronise data

Go to gxb catalog, start witness_node

```bash
# These parameter could be used for saving RAM： --track-account and --partial-operations=true
nohup ./programs/witness_node/witness_node --data-dir=trusted_node --rpc-endpoint=127.0.0.1:28090 \
--p2p-endpoint=0.0.0.0:6789 --log-file --track-account "\"1.2.2999\""  --track-account "\"1.2.3000\"" \
--partial-operations=true >>witness.out 2>&1 &
```

--help (used to search command)
witness_node start：
```
# assign directory for data and configuration files
--data-dir=trusted_node

# Assign monitor address and port for rpc service(port could be changed)，127.0.0.1: limit to local access rpc service, if you do not need to limit local access, 0.0.0.0 could be used.
--rpc-endpoint=127.0.0.1:28090

# For connecting p2p network, we do not recommend to change this parameter.
--p2p-endpoint=0.0.0.0:6789

# Output log file, log file will output to control panel without this parameter.
--log-file

# Historical transaction of specified account will be tracked in RAM, one can use multiple times to track multiple accounts. Please change 1.2.2999 to 1.2.account digital id(below the account name, there is a 5 digit number)
--track-account "\"1.2.2999\""

# "NUM" records will be saved for each accessed account in RAM, default: entire operation record will be saved.
--max-ops-per-account=NUM

# Combine --track-account and --max-ops-per-account two choices could further saving RAM, recommended.
--partial-operations=true

& denotes programs are running background.

# Replay all the downloaded blocks and rebuild indexes(time consuming).
--replay-blockchain

# Delete all the downloaded data, re-synchronize blockchain
--resync-blockchain
```

Currently, full node use more than 12GB RAM, when combine --track-account account_id(account id in the form of 1.2.x)and --partial-operations=true, only historical transaction from exchange would be saved, which could reduce RAM usage to 4 GB.
```
# Please change 1.2.2999 to 1.2.account digital id,"1.2." denotes the type is account. The account digital id is from the wallet registered on https://wallet.gxb.io
--track-account="\"1.2.2999\""
--partial-operations=true
```

It takes around 30 minutes to finish block synchronization. One could use trusted\_node/logs/witness.log to check the status of block synchronization. Check the newest block on https://block.gxb.io
After block synchronization, one could use command-line wallet.

### 4. Run command-line wallet: cli_wallet

Use command-line wallet, cli_wallet to connect witness_node:

```bash
./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:28090 \
--enable-rpc-log -r 127.0.0.1:8091 --data-dir=trusted_node
```

cli_wallet start parameters：
```
# Connect websocket rpc address of witness_node
-s

# Output rpc log file
--enable-rpc-log

# cli_wallet provides websocket rpc address, start API service of cli_wallet
# note：do not configure as 0.0.0.0, because all the host could access your wallet.
-r 127.0.0.1:8091

# daemon mode
-d

& denote the program is running at background
```

Set a password for the wallet(The password is locally for unlock wallet)：

```
new >>> set_password my_password
# pass, then display:
locked >>>
# then unlock wallet:
locked >>> unlock my_password
# pass, then display：
unlocked >>>
```
info command could check current block synchronise status.
```
unlocked >>> info
info
{
  "head_block_num": 3913758,
  "head_block_id": "003bb81eec2abfdb2cf58ffdf4dd547ea190530e",
  "head_block_age": "3 seconds old",
  "next_maintenance_time": "0 second ago",
  "chain_id": "4f7d07969c446f8342033acb3ab2ae5044cbe0fde93db02de75bd17fa8fd84b8",
  "participation": "100.00000000000000000",
  ...
 }
 # head_block_age denotes the updated blocktime, 3 seconds for each block
 # participation denotes the participation rate of the witnesses, participation rate must above 70.
```
For more cli_wallet API, check[wallet api documentation](https://doc.gxb.io/core/ming-ling-xing-qian-bao-cli-wallet-api-shuo-ming.html)。

### 5. Use command-line wallet cli_wallet, import private key

If you do not have an account, you need download GXB light wallet or visit online wallet and register an account(Do not forget backup private key file).

| GXB light wallet | [https://www.gxb.io/\#downLoad](https://www.gxb.io/#downLoad) |
| :--- | :--- |
| Online wallet| [https://wallet.gxb.io](https://wallet.gxb.io) |

Regarding how to use wallet, refer https://steemit.com/blockchain/@gxshares/how-to-creat-a-wallet-on-gxchain-gxshare-wallet

Execute following commands at command-line wallet：

```
// import private key
unlocked >>> import_key account_name wif_key true
```

```
// Check all accounts controled by local wallet
unlocked >>> list_my_accounts
```

```
// Check account information
unlocked >>> get_account account_name
```

```
// Check account balance
unlocked >>> list_account_balances account_name
```

```
// Transfer(balance should > 0)
// GXS stands for GXShares, GXC stands for GXCoin(not for exchange)
unlocked >>> transfer from_account to_account 100 GXS "" true
```

```
// Check updated block height, will retuen head_block_number
unlocked >>> get_object 2.1.0
```

```
// Check block information, or check specific block
unlocked >>> get_block 881577
```

There are two command-line interface for transfer: transfer and transfer2, transfer2 execute successfully will return current transaction id.


### 6. Running cli_wallet in the background

After you import wallet private key, press "ctrl + c" to exit, local wallet file will be generated. Restart with -d &, as follows：

```
nohup ./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:28090 \
--enable-rpc-log -r 127.0.0.1:8091 -d >>wallet.out 2>1 &
```
After start above commands, press "enter", then input "exit" at shell to exit terminal. Output from control panel is redirect to wallet.out


## 4. Check historical transaction and top-up monitor
#### Check historical transaction, and get txID of transaction.

Besides command-line interface, cli\_wallet provides JSON RPC interface. when wallet turn on http rpc API service,which is same as input command in wallet. 可以过wscat或者使用http客户端\(for instance, curl\) for call.

其中method 传入命令名，params 数组传入参数清单\(无参数时，params传空数组\)， id为请求的标识，返回结果中的id和请求id一致。如果执行成功，结果会有 result ，否则会有 error

cli_wallet provides three API for check historical transactions：get_account_history, get_relative_account_history, and get_account_history_by_operations. get_account_history_by_operations will return transaction id(txID). More information for these three API, check[wallet api documentation](https://doc.gxb.io/core/ming-ling-xing-qian-bao-cli-wallet-api-shuo-ming.html)
Take get_account_history_by_operations for example as follows：
1. Unlock wallet.
2. Check account id by account name.
3. Use account id to check historical transactions.

**Below is the process in detail, using gxb-light as example**：

#### 1. Unlock wallet:

###### request：
```js
// Unlock wallet，my_password is the password
curl --data '{"jsonrpc": "2.0", "method": "unlock", "params": ["my_password"], "id": 1}' http://127.0.0.1:8091/rpc
```
Success, return：
```js
{"id":1,"jsonrpc":"2.0","result":null}
```

#### 2. Get account id in gxb-light:

If account id is known, skip this step.
###### request：
```js
// get_account_id, use account name as a parameter to get account id
curl --data '{"jsonrpc": "2.0", "method": "get_account_id", "params": ["gxb-light"], "id": 1}' http://127.0.0.1:8091/rpc
```

Return as follows, account id is "1.2.3054", "1.2." denotes the type is account.

```js
{"id":1,"result":"1.2.3054"}
```

#### 3. Call get_account_history_by_operations API, check account historical transaction, information returned include txID:

If wallet is locked, one cannot use memo to decrypt transaction. Only sender and receiver could decrypt memo.

###### request:
```js
// get_account_history_by_operations, the first parameter is account id; the second parameter is operation assay,[] could be used;the third parameter is initial number;the fourth parameter is limit, to get limit numbers of historical transaction.
curl --data '{"jsonrpc": "2.0", "method": "get_account_history_by_operations", "params": ["1.2.3054",[], 1, 10], "id": 1}' http://127.0.0.1:8091/rpc
```
###### response:

```js
{
  "id": 1,
  "jsonrpc": "2.0",
  "result": {
    "total_without_operations": 3, // normally, this number is equal to limit parameter, which means found transactions before operation assay 数组过滤之前，实际查询到了多少笔交易； Already found newest transaction if the number is less than limit.
    "details": [{ //  details returned an array, each item is a transaction
      "memo": "196702323",  // decrypted memo, only sender and receiver could decrypt memo
      "description": "Transfer 3099 GXS from gxb-light to yunbi-gxs -- Memo: 196702323   (Fee: 0.09493 GXS)", //description of the transaction
      "op": {  // operations in transaction
        "id": "1.11.3436",
        "op": [0, {// index0 denotes transfer
          "fee": {
            "amount": 9493,// Transaction fee is(9493/100000) GXS, all the digit should be divided by 100000
            "asset_id": "1.3.1" //  1.3.1 denotes GXS
          },
          "from": "1.2.3054",// account that sending currency
          "to": "1.2.2999",// account that receiving currency
          "amount": {
            "amount": 300000,  // GXS amount is 300000/100000 (3GXS，all the digit should be divided by 100000)
            "asset_id": "1.3.1"
          },
          "memo": {
            "from": "GXC68o9LkFKv5ihSt6z9oTmc7wVALUmT5Kd75BTy9rMp38wSuWU5N",
            "to": "GXC5yRwAseZhPBorMmxXhQvmvsDzoJJbQnau5fFboGr5V2zGZPWsk",
            "nonce": "384187316390066",
            "message": "1c05a47a362361c0cf2dba52d08f3517"  // encrypted memo
          },
          "extensions": []
        }],
        "result": [0, {}],
        "block_num": 1115034, // Height of the block
        "trx_in_block": 0,// index for specific transaction in the block
        "op_in_trx": 0,
        "virtual_op": 3482
      },
      "transaction_id": "f9f8f8359c59ac1341516facdf30c98fd5d57b5b"
    }, {
      "memo": "",
      "description": "Transfer 7100 GXS from yunbi-gxs to gxb-light   (Fee: 1 GXC)",
      "op": {
        "id": "1.11.3331",
        "op": [0, {
          "fee": {
            "amount": 100000,
            "asset_id": "1.3.0"
          },
          "from": "1.2.2999",
          "to": "1.2.3054",
          "amount": {
            "amount": 710000000,
            "asset_id": "1.3.1"
          },
          "extensions": []
        }],
        "result": [0, {}],
        "block_num": 1000766,
        "trx_in_block": 0,
        "op_in_trx": 0,
        "virtual_op": 3377
      },
      "transaction_id": "7c64c51ee931043ca1bc6791efc942e94b8236af"
    }]
  }
}
```
**Attention：For exchange, asset_id must be 1.3.1\(GXS\)。**

#### irreversible block

调用cli\_wallet的get\_dynamic\_global\_properties接口，check current highest irreversible block number. blc小于此区块高度的区块，其包含的交易都是已经被确认不可回退的。可用作用户提现时参考，区块不可回退时再处理用户提现。

```js
curl --data '{"jsonrpc": "2.0", "method": "get_dynamic_global_properties", "params": [], "id": 1}' http://127.0.0.1:8091/rpc
```

return：

```js
{
  "id": 1,
  "result": {
    "id": "2.1.0",
    "head_block_number": 1724515, // block height
    "head_block_id": "001a506300f6717c3e99b6d8d89b264d94f1793c",
    "time": "2017-08-16T14:51:53",
    "current_witness": "1.6.2",
    "next_maintenance_time": "2017-08-16T15:00:00",
    "last_budget_time": "2017-08-16T14:50:00",
    "witness_budget": 0,
    "accounts_registered_this_interval": 0,
    "recently_missed_count": 1,
    "current_aslot": 2426557,
    "recent_slots_filled": "326968992855967788277935148493325729655",
    "dynamic_flags": 0,
    "last_irreversible_block_num": 1724507   // Highest irreversible block number is 1724507
  }
}
```

## 5. Common problems
1. **Error in block synchronization**

   Check: trusted\_node/logs/witness.log, if continue errors occured, e.g."unlinkable block", "block does not link to known chain", which means error in block synchronization.

   Solutions：

   1. Error in block synchronization, local blockchain files may corrupted. Stop witness\_node and delete trusted\_node, then restart witness\_node.

   2. Add --resync-blockchain in startup Commands instead of delete trusted\_node, system will re-synchronize blocks.

2. **How to safely close witness\_node**

   1. If witness\_node is not running at background, press Ctrl + C, then wait the system saves data from the RAM and close automatically.
   2. If witness\_node is running at background, execute `kill -s SIGINT $(pgrep witness_node)`,then wait the system saves data from the RAM and close automatically. One should not use kill -9, or index will be rebuild next, which slow down the startup nexttime.

3. witness\_node重启以后，需要重新启动cli\_wallet。因为cli\_wallet后台运行时，不会自动退出。关闭cli\_wallet的方法：执行  
   `kill -s SIGINT $(pgrep cli_wallet)`

4. 如果异常退出，则重新启动时，很可能需要重建索引，启动比较慢。如果 witness\_node 出现异常，一般先尝试带 --replay-blockchain 参数重启，即手工触发重建索引。

5. witness\_node重启后，需要重启cli\_wallet。如果cli\_wallet是后台运行的，cli\_wallet不会因witness\_node退出而自动退出，也需要重启。
6. **cli_wallet shut down**

   如果遇到cli_wallet后台运行一段时间后退出的情况，可能的原因是终端掉线，建议后台运行成功后，关闭当前终端。或者在启动命令行之前加上nohup:

```
nohup ./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:28090  --enable-rpc-log -r 127.0.0.1:8091 -d >>wallet.out 2>1 &
```
7. 建议使用脚本启动程序(脚本中的帐户id需要做修改)： [witness_nodes startup script](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/script/witness_start.sh) and [cli_wallet startup script](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/script/wallet_start.sh)

## 6 Attention
1. 用户充值。每笔转账可以带一个备注，交易所通过这个备注来区分是哪个用户的充值。具体备注与交易所用户关联关系，请交易所自行设定。备注是加密的，只有转帐双方才可以解密。
2. 转帐手续费问题。转帐的手续费由2部分组成：基本手续费 + memo费用，其中基本手续费为0.05GXS, memo费用按总字节长度收费，0.5GXS/`KB`。一次转帐手续费计算：`0.05 + 0.5*(KBs of memo) GXS ， 取KB时截断取整`。 带备注的转帐，手续费一般在[0.05, 0.15] GXS之间(假设备注长度不超过100字节)。
3. 交易所查询交易历史，处理用户充值时，需要处理用户充值时填入memo的特殊情况。比如交易所提供给用户的memo是1234567，而用户充值填写的memo是"我的memo是1234567!"，类似情况需要考虑。
4. Memo like GXS123456 is not recommended.**There is no limitation for the length and content of memo, but we recommend the length of memo > 10, and start with digit**
5. 钱包的json rpc接口，帐户名需要传入小写，不能有大写字母。用户提现到公信宝钱包时，需要将绑定的帐户名转为小写。
6. .**人工处理用户充值问题**：需要用户提供txID(用户可以通过钱包查看当前转帐信息获得)，txID可以保证用户充值转帐的唯一性。务必注意：**平台保存好人工处理过的txID, 同一txID只处理1次**。如果不同的注册用户使用同一txID提交工单的，属于欺骗！
7. 系统中存在多种资产，其中资产id（asset_id）1.3.1为GXS。监听用户充值时，请务必校验转帐交易中的asset\_id字段为1.3.1。
8. 用户提现。调用transfer/transfer2处理用户提现时，转帐数量请传入字段串，加双引号。如下：
```
curl --data '{"jsonrpc": "2.0", "method": "transfer2", "params": ["from_account", "to_account", "100.01", "GXS", "",  true], "id": 1}' http://127.0.0.1:8091/rpc
```
9. Precision of GXS is 5(smallest unit is0.00001 GXS). There is no decimals in GXChain, all the numbers are amplified by 100000 times, as a result, numbers from get_account_history / get_account_history_by_operations, e.g. transfer amount, should be devided by 100000 to get the real result.
10. Only check 钱包状态为locked状态时只能查询，不能转帐，不能解密转帐备注。如果需要转帐或者查询交易历史，unlock wallet needed.
11. **transfer/transfer2转帐时，第3个参数转帐数量如果包含小数，必须加双引号，否则转帐会失败。建议转帐数量统一加上双引号。**

##### Related Files：

1. [witness_node startup script](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/script/witness_start.sh)
2. [cli_wallet startup script](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/script/wallet_start.sh)
3. [Alternative cli_wallet startup script](http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/script/start_backup_wallet.exp)，脚本提供3个主网接入点，如果本地witness_node暂时不可用，可以执行此脚本，连接主网接入点
4. [GXB cold wallet offline signature tutorial](https://doc.gxb.io/core/gxbleng-qian-bao-li-xian-qian-ming.html)
5. [wallet api documentation](https://doc.gxb.io/core/ming-ling-xing-qian-bao-cli-wallet-api-shuo-ming.html)
