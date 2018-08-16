
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

1.  从beita账号转账给nathan，100GXC(1.3.0)，锁定30秒后开始根据时间线性释放，120秒全部释放完毕
```
call_contract beita t81611 {"amount":10000000,"asset_id":1.3.0} vestingcreate "{\"to\":\"nathan\",\"lock_duration\":30,\"release_duration\":120}" GXC true
```

2. 查询合约ch3有哪些表
```
unlocked >>> get_contract_tables ch3
[
  "vestingrule"
]
```

3. 查询合约ch3 vestingrule，最多查询10ms就立即返回结果
```
unlocked >>> get_table_objects ch3 vestingrule
[{
    "account_id": 17,
    "asset_id": 0,
    "vesting_amount": 10000000,
    "vested_amount": 0,
    "lock_time_point": 1534422875,
    "lock_duration": 30,
    "release_time_point": 1534422905,
    "release_duration": 120
  }
]

```

4. 查询合约下有多少资产

```
unlocked >>> list_account_balances ch3
100 GXC
```

5. 尝试释放ch3中nathan的GXC(1.3.0)资产，根据当前时间和vestingrule来释放
```
call_contract beita t81611 null vestingclaim "{\"who\":\"nathan\"}" GXC true
```
