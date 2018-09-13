#### 调用合约

##### 初始化bancor：需要调用2次，使用2个资产创建2个connector， 之后这2个资产就可以基于bancor的buy方法进行互相兑换

```
call_contract beita bancor20 {"amount":10000000,"asset_id":1.3.1} issue "{}" GXC true
call_contract beita bancor20 {"amount":100000000,"asset_id":1.3.2} issue "{}" GXC true
```

##### 开始执行asset_id(1.3.1(GXS))和asset_id(1.3.2(TXT))两种资产的互相兑换

用1000个TXT兑换一定量的GXS

```
call_contract beita bancor20 {"amount":100000000,"asset_id":1.3.2} buy "{}" GXC true
```
用1000个GXS兑换一定量的TXT

```
call_contract beita bancor20 {"amount":100000000,"asset_id":1.3.1} buy "{}" GXC true
```

查询合约币的状态，合约币并不真正存在这种资产，只是作为状态记录在合约中，用于根据supply和cw进行计算价格等

```
get_table_objects bancor coinmarket
```

[{
    "bancorid": 1016,
    "supply": 100000000,
    "base": {
      "balance": {
        "amount": 9695770,
        "asset_id": 1
      },
      "weight": "0.50000000000000000"
    },
    "quote": {
      "balance": {
        "amount": 103226653,
        "asset_id": 2
      },
      "weight": "0.50000000000000000"
    }
  }
]
