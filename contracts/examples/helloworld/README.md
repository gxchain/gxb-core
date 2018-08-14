helloworld 合约
-------------
helloworld 合约用于简单的演示，提供了一个hi方法，调用该合约会打印一行日志。


#### 编译合约
执行如下命令, 生成wast和wasm文件:
```
gxx -o helloworld/helloworld.wast helloworld/helloworld.cpp
```

生成abi:
```
gxx -g helloworld/helloworld.abi helloworld/helloworld.cpp
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
// 使用nathan帐户，部署名为helloworld的合约
unlocked >>> deploy_contract helloworld nathan 0 0 ./helloworld GXS true

```
其中
```
// helloworld指定将要创建的合约名
// nathan为部署合约的帐户
// 0 0 分别指定了vm类型和版本号
// ./helloworld指定合约文件的路径
// GXS表示使用GXS支付手续费
// true表示执行，发起广播
```

#### 调用合约
调用helloworld合约的hi方法：
```
unlocked >>> call_contract nathan helloworld null hi "{\"user\":\"zhuliting\"}" GXS true
```
其中
```
// nathan指定调用合约的帐户
// helloworld指定要调用的合约帐户名
// null 表示不向合约发送资产
// hi 表示要调用的合约方法
//  "{\"user\":\"zhuliting\"}" 表示合约hi文件的参数，参数以json格式传入
// GXS表示使用GXS支付手续费
// true表示执行，发起广播
```

