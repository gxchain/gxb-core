# stackpool

### 静态变量

```c++
static const uint64_t POOLASSETID       = 1; // stackpool发放的奖励资产的id.
static const uint64_t ADMINACCOUNT    = 22; // 管理员账号id.
static const uint64_t DURATION          = 7 * 24 * 60 * 60; // 每次收益期的长度.
```
---
### 数据结构
#### 质押池表
- 用于记录每种代币的信息.
```c++
struct pool {
    uint64_t asset_id; // 质押池质押的资产id.
    bool locked; // 是否被锁定.
    int64_t total_amount; // 总质押金额.
    int64_t reward_rate; // 单位时间可获得增发奖励的数量.
    int64_t reward_per_token; // 单位质押资产可获得的增发奖励的数量.
    int64_t start_time; // 质押池开始运行的时间.
    int64_t last_update_time; // 上一次更新reward_per_token的时间.
    int64_t period_finish; // 此次收益期结束的时间.
    std::map<uint64_t, int64_t> last_reward_per_token; // 每个用户上次用来计算收益的reward_per_token, key为userid, value为reward_per_token.
    std::map<uint64_t, int64_t> reward; // 每个用户获得的增发奖励, key为userid, value为奖励的数量.
    std::map<uint64_t, int64_t> stake; // 每个用户质押资金的数量, key为userid, value为质押数量.

    uint64_t primary_key() const { return asset_id; }
    GRAPHENE_SERIALIZE(pool, (asset_id)(locked)(total_amount)(reward_rate)(reward_per_token)(start_time)(last_update_time)(period_finish)(last_reward_per_token)(reward)(stake))
};
```

---
### 接口
#### 1.质押资金接口
- 向指定质押池中质押资金
```c++
// @abi action
// @abi payable
void stack() {
    // 获取用户转入的资产的信息.
    // 检查资产是否存在并且是否没有被锁定.
    // 检查当前时间是否大于池子的开始时间.
    // 调用_update_reward_per_token更新reward_per_token.
    // 增加用户在stack中的余额.
    // 增加池子的total_amount.
}
```

#### 2.提现资产接口
- 从指定质押池中提现资金
```c++
// @abi action
void withdraw(contract_asset asset) {
    // 检查资产是否存在.
    // 检查当前时间是否大于池子的开始时间.
    // 调用_update_reward_per_token更新reward_per_token.
    // 判断用户stack中的余额是否充足.
    // 减少用户在stack中的余额.
    // 减少池子的total_amount.
    // 向用户转相应数量的资产.
}
```

#### 3.获取奖励接口
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

#### 4.退出接口
- 从指定质押池中提现所有质押的金额, 同时领取所有增发的奖励
```c++
// @abi action
void exit(uint64_t asset_id) {
    // 检查传入的资产id是否存在.
    // 调用withdraw, 输入参数的asset_id为用户传入的值, amount为stack中用户的余额.
    // 调用get_reward, 传入参数为asset_id.
}
```

#### 5.设置增发奖励接口
- 设置指定质押池, 接口会在内部判断, 可能开启新的一轮收益期, 可能增加现在的收益期的增发金额, 也可能只是进行初始化
```c++
// @abi action
// @abi payable
void notifyreward(uint64_t asset_id) {
    // 检查调用者是否为ADMINACCOUNT.
    // 获取转进来的资产的信息.
    // 检查池子是否存在.
    // 检查转账进来的资产是否是POOLASSETID.
    // 调用_update_reward_per_token更新reward_per_token.
    // 如果当前时间大于池子开始的时间
    //   如果当前的时间大于等于池子本次收益结束的时间, 则开启一个新的收益周期.
    //     以当前时间为起点, 开启一个新的收益周期, 时间长度为DURATION.
    //     将period_finish变为新的收益周期的结束时间.
    //     根据转进来的资产数量及DURATION计算单位时间内可以增发的数量reward_rate.
    //   如果当前时间小于池子本次收益结束的时间, 则增加此次收益周期的增发数量.
    //     根据当前的时间和本次收益结束的时间计算出剩下的时间.
    //     根据剩下的时间和上一次计算得到的reward_rate得出还没有增发的数量.
    //     根据还没增发的数量和此次转入的金额计算出剩余时间中应该增发的总量.
    //     根据剩余时间中应该增发的总量和剩下的时间计算出新的reward_rate.
    //   记录当前时间为last_update_time.
    // 如果当前时间小于等于池子开始的时间, 则认为这是一次初始化调用.
    //   将收益期结束时间设置为start_time + DURATION.
    //   记录start_time为last_update_time.
    //   根据转入的金额及DURATION计算reward_rate.
}
```

#### 6.新建质押池接口
- 对指定资产新建一个质押池
```c++
// @abi action
void newpool(uint64_t asset_id, int64_t start_time) {
    // 检查调用者是否是ADMINACCOUNT.
    // 检查资产是否不存在.
    // 向pool中插入一条新的记录.
}
```

#### 7.管理质押池接口
- 管理员锁定或开启质押池, 锁定后新的资产不能进入, 但是可以提现及领取增发奖励
```c++
// @abi action
void managepool(uint64_t asset_id, bool locked) {
    // 检查调用者是否是ADMINACCOUNT.
    // 检查资产是否存在.
    // 修改pool中记录的状态.
}
```