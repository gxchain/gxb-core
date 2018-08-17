riddle 合约

------

riddle合约是一个谜题合约，提供2个接口：（issue)发行谜题和(reveal)解密谜题。

任意一个帐户都可以调用issue方法来创建一个谜题，创建时带上问题和加密的答案，问题和答案将会保存在合约里。解谜时调用reveal方法，带上明文答案。

#### 合约编译

1. 生成wast:

```
gxx -o contracts/examples/riddle/riddle.wast contracts/examples/riddle/riddle.cpp 
```

2. 生成abi:

```
gxx -g contracts/examples/riddle/riddle.abi contracts/examples/riddle/riddle.cpp 
```

#### 合约部署和调用

##### 启动cli_wallet

```
./programs/cli_wallet/cli_wallet -sws:///127.0.0.1:28099 --chain-id xxxx
```

##### 部署合约

```shell
// 这里使用nathan帐户部署合约，部署的合约名为riddle
unlocked >>> deploy_contract riddle nathan 0 0 ./contracts/examples/riddle GXS true
```

##### 调用合约

1. 发起谜题, 使用nathan帐户，发起一个谜题，问题："1 + 2 = ?"  答案： "I don't know." 

```shell
字符串生成sha256 命令行：
# echo -n "I don't know." | shasum -a 256
810d7c3ca6aa6411102588f8636833035138a35f32f5e7b2df96d2ab2dc4d660
```

```shell
// 发起谜题
call_contract nathan riddle null issue "{\"question\":\"1 + 2 = ?\", \"hashed_answer\":\"810d7c3ca6aa6411102588f8636833035138a35f32f5e7b2df96d2ab2dc4d660\"}" GXS true
```

2. 查询riddle合约的存储表

```
unlocked >>> get_table_objects riddle record
[{
    "issuer": 17,
    "question": "1 + 2 = ?",
    "hashed_answer": "810d7c3ca6aa6411102588f8636833035138a35f32f5e7b2df96d2ab2dc4d660"
  }
]
```

3. 解开谜题

```
// 使用nathan帐户解谜
unlocked >>> call_contract nathan riddle null reveal "{\"issuer\":\"nathan\", \"answer\":\"I don't know.\"}" GXS true
```

4. 合约存储表

```
unlocked >>> get_table_objects riddle record
[]
```
