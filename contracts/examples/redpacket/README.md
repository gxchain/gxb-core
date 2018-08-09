redpacket 合约
---------




#### 合约编译

1. 生成wast:
```
gxx -o contracts/examples/redpacket/redpacket.wast contracts/examples/redpacket/redpacket.cpp 
```

2. 生成abi:
```
gxx -g contracts/examples/redpacket/redpacket.abi contracts/examples/redpacket/redpacket.cpp 
```

#### 合约部署和调用

##### 启动cli_wallet
```
./programs/cli_wallet/cli_wallet -sws:///127.0.0.1:28099 --chain-id xxxx
```

##### 部署合约
```
// 这里使用nathan帐户部署合约，部署的合约名为redpacket
unlocked >>> deploy_contract redpacket nathan 0 0 ./contracts/examples/redpacket GXS true


```

##### 调用合约

1. 发行红包
```
// 使用nathan帐户，发行一个红包， 红包公钥为GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b， 金额总量为100 GXS(链上为大数，需要乘以10万)， 数量为5个
unlocked >>> call_contract nathan redpacket {"amount":10000000,"asset_id":1.3.1} issue "{\"pubkey\":\"GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b\",\"number\":5}" GXS true
```

2. 发行红包后，查询redpacket合约的帐户余额
```
unlocked >>> list_account_balances redpacket
```

3. 打开红包
```
// 使用nathan帐户打开一个红包
// 合约名redpacket
// 打开红包的合约方法名 open
// json为redpacket合约abi约定的参数， 其中issue为发行红包的帐户名, timestamp为当前时间，sig为签名(使用红包发起人提供的私钥，对timestamp进行签名, cli_wallet提供了sign_string方法)
unlocked >>> call_contract nathan redpacket null open "{\"issuer\":\"nathan\",\"sig\":\"20217c7a2c9f00b886ba029c65bb0338407a292db97eb0937b7e8a50b19f70fbfd1dd1bec9f861f616a850a6a8cb955ba870e8750ef145ae33b9c05cced8b033eb\",\"timestamp\":456}" GXS true

```

4. 列出合约所有的存储表
```
unlocked >>> get_contract_tables redpacket
[
  "packet",
  "record"
]
```

5. 查询合约的packet表，该表记录了所有的红包
```
unlocked >>> get_table_objects redpacket packet
[{
    "issuer": 17,
    "pub_key": "GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b",
    "total_amount": {
      "amount": 10000000,
      "asset_id": 0
    },
    "number": 5,
    "subpackets": [
      3112947,
      1611570,
      1170798,
      1363639
    ]
  }
]
```

6. 查询合约的record表，该表记录了所有的开红包记录
```
unlocked >>> get_table_objects redpacket record
[{
    "packet_issuer": 17,
    "accounts": [{
        "account_id": 17,
        "amount": 2741046
      }
    ]
  }
]

```

7. 如果红包未抢完，发行者可以关闭红包，余额退还

```
// 只有发行红包的帐户，才可以关闭红包
// 此处为nathan帐户
// 关闭红包的合约方法名为close
unlocked >>> call_contract nathan redpacket null close "{}" GXS true
```

本次演示使用的公私钥对
```
pubkey:"GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b"
wifkey:"5J9vj4XiwVQ2HNr22uFrxgaaerqrPN7xZQER9z2hwSPeWdbMKBM"
```
