### 用于测试跨合约转账
- user(dev) [4GXC] --> contracta [1GXC] --> contractb

#### case1
```
用户dev调用合约contracta同时向合约转账4GXC，合约contracta调用contractb同时向contractb转账1GXC
contracta,contractb初始余额为0，最终调用失败，原因是跟函数一样，在dev调用contracta的hi方法时，hi方法会调用contractb，此时contracta的hi方法没有执行完毕，所以此时余额是0，相当于函数执行过程中压栈，之后执行向合约contractb转账1GXC由于余额不够最终失败
```

#### case2
```
用户dev调用合约contracta同时向合约转账4GXC，合约contracta调用contractb同时向contractb转账1GXC
contracta,contractb初始余额为10GXC，最终contracta余额13GXC，contractb余额11GXC
```