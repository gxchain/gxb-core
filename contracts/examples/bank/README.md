bank 合约
---------
bank 合约提供了充值(deposit)和提现(withdraw)方法，可以向合约充值、从合约中提取资产。


#### 合约编译

1. 生成wast:
```
gxx -o contracts/examples/bank/bank.wast contracts/examples/bank/bank.cpp 
```

2. 生成abi:
```
gxx -g contracts/examples/bank/bank.abi contracts/examples/bank/bank.cpp 
```

#### 合约部署和调用

##### 启动cli_wallet
```
./programs/cli_wallet/cli_wallet -sws:///127.0.0.1:28099 --chain-id xxxx
```

##### 部署合约
```
// 这里使用nathan帐户部署合约，部署的合约名为bank
unlocked >>> deploy_contract bank nathan 0 0 ./contracts/examples/bank GXS true

```

##### 调用合约

1. 调用合约的deposit方法
```
// 使用nathan帐户，调用bank合约，方法名为deposit,  发送资产总量为100 GXS(链上为大数，需要乘以10万)
unlocked >>> call_contract nathan bank {"amount":10000000,"asset_id":1.3.1} deposit "{}" GXS true
```

2. 查询bank合约的帐户余额
```
unlocked >>> list_account_balances bank
100 GXS
```

3. 列出合约所有的存储表
```
unlocked >>> get_contract_tables bank
[
  "account"
]
```
该bank合约只有一个account表

4. 查询合约的account表，该表记录了帐户在合约中的余额
```
unlocked >>> get_table_objects bank account
[{
    "owner": 17,
    "balances": [{
        "amount": 10000000,
        "asset_id": 1
      }
    ]
  }
]
```

5. 调用合约的withdraw方法提现

```
// 提现到nathan帐户
unlocked >>> call_contract nathan bank null withdraw "{\"to_account\":\"nathan\", \"amount\":{\"asset_id\": 1, \"amount\":100000}}" GXS true

// 提现到init0帐户
unlocked >>> call_contract nathan bank null withdraw "{\"to_account\":\"init0\", \"amount\":{\"asset_id\": 1, \"amount\":100000}}" GXS true

```

