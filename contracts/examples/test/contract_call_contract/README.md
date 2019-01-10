### 合约循环调用展示 user(dev) --> contracta --> contractb --> contractc --> contracta
- 用于测试最大跨合约调用次数
- 编译代码前需要修改合约中action中的第一个参数，也就是合约id，用来构成循环调用

#### 编译
```
./bin/gxx -g contract_call_contract/contracta/contracta.abi contract_call_contract/contracta/contracta.cpp
./bin/gxx -o contract_call_contract/contracta/contracta.wast contract_call_contract/contracta/contracta.cpp
./bin/gxx -o contract_call_contract/contractb/contractb.wast contract_call_contract/contractb/contractb.cpp
./bin/gxx -g contract_call_contract/contractb/contractb.abi contract_call_contract/contractb/contractb.cpp
./bin/gxx -o contract_call_contract/contractc/contractc.wast contract_call_contract/contractc/contractc.cpp
./bin/gxx -g contract_call_contract/contractc/contractc.abi contract_call_contract/contractc/contractc.cpp
```

#### 部署合约
deploy_contract ca8 dev 0 0 /Users/sky/gxb_install/contract_call_contract/contracta GXC true

deploy_contract cb8 dev 0 0 /Users/sky/gxb_install/contract_call_contract/contractb GXC true

deploy_contract cc8 dev 0 0 /Users/sky/gxb_install/contract_call_contract/contractc GXC true

#### 调用合约
call_contract dev ca8 null hicontract "{\"act_id\":100000}" GXC true

#### 结果是调用失败
- 提示错误：Assert Exception: trx_context.cross_contract_calling_count <= 8: max cross contract calling can not exceed 8，这里的8是全局参数，可以通过理事会投票修改