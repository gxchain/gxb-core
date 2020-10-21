# stakepool

### 静态变量

```c++
static const uint64_t POOLASSETID   = 1; // stakepool发放的奖励资产的id.
static const uint64_t ADMINACCOUNT  = 22; // 管理员账号id.
static const uint64_t SWAPACCOUNT   = 100; // swap账号地址.
```
---
### 数据结构
#### 质押池表
- 用于记录每种代币的信息.
```c++
struct pool {
    uint64_t asset_id; // 质押池质押的资产id或swap中池子的id. (理论上两者会重复, 但这里不考虑)
    bool locked; // 是否被锁定.
    bool is_lq; // 质押的是否是流动性.
    std::string coin1; // 第一种代币的名称.
    std::string coin2; // 第二种代币的名称.
    int64_t total_amount; // 总质押金额.
    int64_t reward_rate; // 单位时间可获得增发奖励的数量.
    int64_t reward_per_token; // 单位质押资产可获得的增发奖励的数量.
    int64_t start_time; // 质押池开始运行的时间.
    int64_t last_update_time; // 上一次更新reward_per_token的时间.
    int64_t period_finish; // 此次收益期结束的时间.
    int64_t duration; // 单次收益期的长度, 可修改.
    std::map<uint64_t, int64_t> last_reward_per_token; // 每个用户上次用来计算收益的reward_per_token, key为userid, value为reward_per_token.
    std::map<uint64_t, int64_t> reward; // 每个用户获得的增发奖励, key为userid, value为奖励的数量.
    std::map<uint64_t, int64_t> stake; // 每个用户质押资金的数量, key为userid, value为质押数量.

    uint64_t primary_key() const { return asset_id; }
    GRAPHENE_SERIALIZE(pool, (asset_id)(locked)(is_lq)(coin1)(coin2)(total_amount)(reward_rate)(reward_per_token)(start_time)(last_update_time)(period_finish)(duration)(last_reward_per_token)(reward)(stake))
};
```

---
### 接口
#### 1.质押资金接口
- 向指定质押池中质押资金
```c++
// @abi action
// @abi payable
void stake() {
    // 获取用户转入的资产的信息.
    // 检查资产是否存在并且是否没有被锁定.
    // 检查当前时间是否大于池子的开始时间.
    // 调用_update_reward_per_token更新reward_per_token.
    // 增加用户在stake中的余额.
    // 增加池子的total_amount.
}
```

#### 2.质押接口(流动性代币)
- 向指定质押池中质押流动性
```c++
// @abi action
void stakelq(uint64_t asset_id, int64_t amount) {
    // 调用swap的transferlqb方法完成转账.
    // 根据coin1和coin2计算获得pool_id.
    // 之后的逻辑和stake一致, 可以复用.
}
```

#### 3.提现接口
- 从指定质押池中提现资金
```c++
// @abi action
void withdraw(uint64_t asset_id, int64_t amount) {
    // 检查资产是否存在.
    // 检查当前时间是否大于池子的开始时间.
    // 调用_update_reward_per_token更新reward_per_token.
    // 判断用户stake中的余额是否充足.
    // 减少用户在stake中的余额.
    // 减少池子的total_amount.
    // 如果池子是流动性质押池则使用swap的transferlqa方法向用户转账, 否则直接withdraw.
}
```

#### 4.获取奖励接口
- 从指定质押池中领取增发的奖励
```c++
// @abi action
void getreward(uint64_t asset_id) {
    // 检查传入的资产id是否存在.
    // 检查当前时间是否大于池子的开始时间.
    // 调用_update_reward_per_token更新reward_per_token.
    // 检查用户reward中的余额是否大于0.
    // 向用户转移数量为reward, id为POOLASSETID的资产.
    // 将用户的reward变为0.
}
```

#### 5.退出接口
- 从指定质押池中提现所有质押的金额, 同时领取所有增发的奖励
```c++
// @abi action
void exit(uint64_t asset_id) {
    // 检查传入的资产id是否存在.
    // 调用withdraw, 输入参数的asset_id为用户传入的值, amount为stake中用户的余额.
    // 调用getreward, 传入参数为asset_id.
}
```

#### 6.设置增发奖励接口
- 设置指定质押池, 接口会在内部判断, 可能开启新的一轮收益期, 可能增加现在的收益期的增发金额, 也可能只是进行初始化
```c++
// @abi action
// @abi payable
void notifyreward(uint64_t asset_id, int64_t new_duration) {
    // 检查调用者是否为ADMINACCOUNT.
    // 获取转进来的资产的信息.
    // 检查池子是否存在.
    // 检查转账进来的资产是否是POOLASSETID.
    // 调用_update_reward_per_token更新reward_per_token.
    // 如果当前时间大于池子开始的时间
    //   如果当前的时间大于等于池子本次收益结束的时间, 则开启一个新的收益周期.
    //     以当前时间为起点, 开启一个新的收益周期, 时间长度为new_duration.
    //     将period_finish变为新的收益周期的结束时间.
    //     根据转进来的资产数量及new_duration计算单位时间内可以增发的数量reward_rate.
    //     将池子的duration设置为new_duration.
    //   如果当前时间小于池子本次收益结束的时间, 则增加此次收益周期的增发数量.
    //     根据当前的时间和本次收益结束的时间计算出剩下的时间.
    //     根据剩下的时间和上一次计算得到的reward_rate得出还没有增发的数量.
    //     根据还没增发的数量和此次转入的金额计算出剩余时间中应该增发的总量.
    //     根据剩余时间中应该增发的总量和剩下的时间计算出新的reward_rate.
    //   记录当前时间为last_update_time.
    // 如果当前时间小于等于池子开始的时间, 则认为这是一次初始化调用.
    //   将池子的duration设置为new_duration.
    //   将收益期结束时间设置为start_time + new_duration.
    //   记录start_time为last_update_time.
    //   根据转入的金额及new_duration计算reward_rate.
}
```

#### 7.新建质押池接口
- 对指定资产新建一个质押池
```c++
// @abi action
void newpool(uint64_t asset_id, int64_t start_time, int64_t duration) {
    // 检查调用者是否是ADMINACCOUNT.
    // 检查资产是否不存在.
    // 判断asset_id是否大于2 ^ 32, 如果是则说明是流动性质押池, 还需要查询coin1及coin2.
    // 向pool中插入一条新的记录, 并记录所有信息.
}
```

#### 8.管理质押池接口
- 管理员锁定或开启质押池, 锁定后新的资产不能进入, 但是可以提现及领取增发奖励
```c++
// @abi action
void managepool(uint64_t asset_id, bool locked) {
    // 检查调用者是否是ADMINACCOUNT.
    // 检查资产是否存在.
    // 修改pool中记录的状态.
}
```