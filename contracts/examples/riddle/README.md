riddle 合约

------

riddle合约是一个谜题合约，提供2个接口：（issue)发行谜题和(reveal)解密谜题。

任意一个帐户都可以调用issue方法来创建一个谜题，创建时带上问题和加密的答案，问题和答案将会保存在合约里。解谜时调用reveal方法，带上明文答案。

#### 合约编译

1. 生成wast:

```
gxx -o contracts/examples/riddle/riddle.wast contracts/examples/riddle/riddle.cpp 
```

1. 生成abi:

```
gxx -g contracts/examples/riddle/riddle.abi contracts/examples/riddle/riddle.cpp 
```

#### 合约部署和调用

##### 启动cli_wallet

```
./programs/cli_wallet/cli_wallet -sws:///127.0.0.1:28099 --chain-id xxxx
```

##### 部署合约

```
// 这里使用nathan帐户部署合约，部署的合约名为riddle
unlocked >>> deploy_contract redpacket nathan 0 0 ./contracts/examples/riddle GXS true
```

##### 调用合约

1. 发起谜题

```
// 使用nathan帐户，发起一个谜题，问题："1 + 2 = ?"  答案： "I don't know." 答案的sha256为""
unlocked >>> 
```

1. 查询riddle合约的存储表

```
unlocked >>> 
```

1. 解开谜题

```
// 使用nathan帐户解谜

unlocked >>> 

```

1. 列出合约所有的存储表

```
unlocked >>> get_contract_tables redpacket
[
  "packet",
  "record"
]
```



