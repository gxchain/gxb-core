
#### 编译合约
执行如下命令, 生成wast和wasm文件:
```
gxx -o dice/dice.wast dice/dice.cpp
```

生成abi:
```
gxx -g dice/dice.abi dice/dice.cpp
```

#### 部署合约
以开发环境为例，使用cli_wallet部署合约。

执行cli_wallet，连接到本地节点：
```
// --chain-id 填写本地链的chain-id
./programs/cli_wallet/cli_wallet -sws://127.0.0.1:28090 --chain-id bc59e6e7f500fa56504ce7101f7df8eb74151398f62167567adcf18a026928d1

```

如果之前没有导入过私钥，需要设置钱包密码、导入私钥，如下：
```
new >>> set_password caochong


locked >>> unlock caochong

// 导入nathan帐户私钥
unlocked >>> import_key nathan 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

```

上述步骤完成后，使用deploy_contract方法部署合约：

```
// 使用nathan帐户，部署名为dice的合约
unlocked >>> deploy_contract dice nathan 0 0 ./dice GXS true

```
其中
```
// dice指定将要创建的合约名
// nathan为部署合约的帐户
// 0 0 分别指定了vm类型和版本号
// ./dice指定合约文件的路径
// GXS表示使用GXS支付手续费
// true表示执行，发起广播
```

#### 调用合约
调用dice合约的deposit方法,beita/nathan向合约存入100个GXC用于dice游戏：
```
call_contract beita dice {"amount":10000000,"asset_id":1.3.0} deposit "{}" GXC true
call_contract nathan dice {"amount":10000000,"asset_id":1.3.0} deposit "{}" GXC true
```
其中
```
// beita/nathan指定调用合约的帐户
// dice指定要调用的合约帐户名
// {"amount":10000000,"asset_id":1.3.0} 表示不向合约发送资产100GXC
// deposit 表示要调用的合约方法
// "{}" 表示合约deposit的参数，参数以json格式传入
// GXC表示使用GXC支付手续费
// true表示执行，发起广播
```
调用dice合约的offerbet方法,beita/nathan分别押注10GXC并指定自己的commitment，commitment的生成方式详见后面：
```
call_contract beita d11421 null offerbet "{\"bet\":{\"amount\":1000000,\"asset_id\":0},\"commitment\":\"492b184fdcb41f442d539e792600a8db67c47eb16c0265e5d1e553473f462486\"}" GXS true

call_contract nathan d11421 null offerbet "{\"bet\":{\"amount\":1000000,\"asset_id\":0},\"commitment\":\"c807cc6c0b9121f6b151f39dcf5fd819e3cb9a6d18f46c7a635f1015148bcc76\"}" GXS true
```

调用dice合约的reveal方法,beita/nathan分别亮牌：
```
call_contract beita d11421 null reveal "{\"commitment\":\"492b184fdcb41f442d539e792600a8db67c47eb16c0265e5d1e553473f462486\",\"source\":\"a80b8a6d1bf2cf798b045366d920531b2ce5f13082c4b2b08d53cfeab5d24b7a\"}" GXS true
call_contract nathan d11421 null reveal "{\"commitment\":\"c807cc6c0b9121f6b151f39dcf5fd819e3cb9a6d18f46c7a635f1015148bcc76\",\"source\":\"5387ec637ac4bdbcf9577451cdc61f4d11ba600ba5540a27065a15aa3f8d45d1\"}" GXS true
```
commitment及相应的resource生成方式如下(mac下):
生成resource：
```
➜  gxb-core git:(dev_master) openssl rand 32 -hex
7d65a071c3f8b38f94e11be1fda97109ea41dfd305e06e677a55edd957327fab
```
根据resource生成commitment：
```
➜  gxb-core git:(dev_master) ✗ echo -n '7d65a071c3f8b38f94e11be1fda97109ea41dfd305e06e677a55edd957327fab' | xxd -r -p | shasum -a 256 -b
24948ec739cd9963bae8805ae18a4a2f4d58689b8d48dc20f289c3ded47ef600 *-
```

亮牌后系统根据2位玩家的commitment和resource计算1个hash，比较hash值的第一个和第二个字节的大小来决定胜负，并完成account表的更新操作，查看account表
```
get_table_objects d11421 account

unlocked >>> get_table_objects dice account
[{
    "owner": 17,
    "balance": {
      "amount": 11000000,
      "asset_id": 0
    },
    "open_offers": 0,
    "open_games": 0
  },{
    "owner": 18,
    "balance": {
      "amount": 9000000,
      "asset_id": 0
    },
    "open_offers": 0,
    "open_games": 0
  }
]
```


