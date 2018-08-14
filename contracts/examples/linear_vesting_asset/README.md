
linear_vesting_asset合约
-------------

#### 编译合约
生成wast:
```
gxx -o contracts/examples/linear_vesting_asset/linear_vesting_asset.wast contracts/examples/linear_vesting_asset/linear_vesting_asset.cpp 
```
生成aib
```
gxx -g contracts/examples/linear_vesting_asset/linear_vesting_asset.abi contracts/examples/linear_vesting_asset/linear_vesting_asset.cpp
```


#### 部署合约
```
deploy_contract ch3 nathan 0 0 	./contracts/examples/linear_vesting_asset/ GXS true
```
#### 调用合约

1.  从beita账号转账给nathan(1.2.17)，1000GXS(1.3.1)，锁定30秒后开始根据时间线性释放，300秒全部释放完毕
```
call_contract beita ch3 {"amount":100000000,"asset_id":1.3.1} vestingcreate "{\"to\":17,\"lock_duration\":30,\"release_duration\":300}" GXS true
```

2. 查询合约ch3有哪些表
```
unlocked >>> get_contract_tables ch3
[
  "account",
  "vestingrule"
]
```

3. 查询合约ch3 account表中的记录，最多查询10ms就立即返回结果
```
get_table_objects ch3 account
unlocked >>> get_table_objects ch3 account
[{
    "owner": 17,
    "assets": [{
        "amount": 100000000,
        "asset_id": 1
      }
    ]
  }
]
```

4. 查询合约ch3 vestingrule，最多查询10ms就立即返回结果
```
unlocked >>> get_table_objects ch3 vestingrule
[{
    "id": "73014444033",
    "account_id": 17,
    "lock_time_point": 1533902660,
    "lock_duration": 30,
    "release_time_point": 1533902690,
    "release_duration": 300,
    "asset_id": 1,
    "asset_amount": 100000000,
    "released_amount": 0
  }
]

```

5. 查询合约下有多少资产
```
unlocked >>> list_account_balances ch3
1000 GXS
```

6. 尝试释放ch3中nathan(1.2.17)的GXS(1.3.1)资产，根据当前时间和vestingrule来释放
```
call_contract beita ch3 null vestingclaim "{\"who\":17,\"asset_id\":1}" GXS true
```
