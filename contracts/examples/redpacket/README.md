redpacket 合约
---------

redpacket 合约是一个口令红包合约，提供了发行红包(issue)、打开红包(open) 和关闭红包(close)方法。

任意一个帐户都可以调用issue方法发行一个红包, 在红包未全部打开之前，可以调用close方法关闭自己发行的红包。

打开红包(open)需要知道红包发行的提供的红包口令。

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
// 使用nathan帐户，发行一个红包， 随机生成的红包口令为GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b， 金额总量为100 GXS(链上为大数，需要乘以10万)， 数量为5个
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
// json为redpacket合约abi约定的参数， 其中issue为发行红包的帐户名，sig为签名(使用红包发起人提供的口令，对自己账号的instanceid(比如nathan的账号id是1.2.17，那么他的instanceid是17)进行签名, cli_wallet提供了sign_string方法)
构造签名，这里红包口令(即共钥)对应的私钥是5J9vj4XiwVQ2HNr22uFrxgaaerqrPN7xZQER9z2hwSPeWdbMKBM:  
sign_string 5J9vj4XiwVQ2HNr22uFrxgaaerqrPN7xZQER9z2hwSPeWdbMKBM 17
unlocked >>> sign_string 5J9vj4XiwVQ2HNr22uFrxgaaerqrPN7xZQER9z2hwSPeWdbMKBM 17
"1f1d104d5750beba9fd4b0637ce69cf54721a57cce91ca81904653307eb72b0a840bd8a80c58df0a7be206a4c5c5b1fa0d96d497667e54579e717d499d0a3498b2"


unlocked >>> call_contract nathan redpacket null open "{\"issuer\":\"nathan\",\"sig\":\"1f1d104d5750beba9fd4b0637ce69cf54721a57cce91ca81904653307eb72b0a840bd8a80c58df0a7be206a4c5c5b1fa0d96d497667e54579e717d499d0a3498b2\"}" GXS true

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
    "encoded_token": "GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b",
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

本次演示使用的口令对
```
pubkey:"GXC81z4c6gEHw57TxHfZyzjA52djZzYGX7KN8sJQcDyg6yitwov5b"
wifkey:"5J9vj4XiwVQ2HNr22uFrxgaaerqrPN7xZQER9z2hwSPeWdbMKBM"
```
