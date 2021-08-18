# voting

### 静态变量

```c++
static const uint64_t ENDLINEBLOCK = 37064083;//投票截止区块，通过更新合约更改
```
---
### 数据结构
#### 用户投标表
- 用于记录用户投票
```c++
struct users {
    uint64_t uid; //用户id
    bool approve; //是否支持，true 或者 false

    uint64_t primary_key() const { return uid; }
    GRAPHENE_SERIALIZE(users, (uid) (approve))
};
typedef multi_index<N(users), users> users_index;
```
---
### 接口
#### vote 投票接口
- 用户对提案是否支持进行投票，在截止前可以更改投票
```c++
// @abi action
    void vote(bool approve) //是否支持,false or true
    {
        //检查是否到截止区块
        //检查用户是否投过票，若没有，向表中添加用户和投票信息，如果投过，更改投票信息
    }
```

