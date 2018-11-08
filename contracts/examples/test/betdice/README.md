betdice 合约
---------

##### 部署合约
```
deploy_contract b7 nathan 0 0 /Users/sky/gxb_install/betdice GXC true
```

##### 调用合约

1. 初始化合约
```
call_contract nathan b7 null init "{}" GXC true
```

2. 向合约充值
```
call_contract nathan b7 {"amount":1000000000,"asset_id":1.3.0} deposit "{}" GXC true
```

3. roll
```
call_contract beita b7 {"amount":20000000,"asset_id":1.3.0} roll "{\"roll_under\":35,\"roll_seed\":\"gooaadd\"}" GXC true
```

4. 从合约提取资金
```
call_contract beita b7 null withdraw "{\"amt\":100000000,\"to\":18}" GXC true
```

##### 查询合约

```
get_table_objects b7 globalvar 1 111 1111

get_table_objects b7 bet 1 111 1111
```

