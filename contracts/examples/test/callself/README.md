### 用于测试跨合约调用当合约调用自己的情况

```
deploy_contract cs dev 0 0 /Users/sky/gxb_install/callself GXC true

call_contract dev cs null f1 "{\"p1\":100000}" GXC true
```
