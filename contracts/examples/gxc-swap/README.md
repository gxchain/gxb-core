# gxc-swap

### 静态变量

```c++
const int64_t ADMINACCOUNT = 22; // 管理员账号, 可以锁定或开启池.
```
---
### 数据结构
#### 资金池表
- 用于记录每个交易对的当前代币余额, 以及该交易对流动性代币的总量.
```c++
struct pool {
    uint64_t index; // 主键, index = string_to_name(std::to_string(asset_id1) + "x" + std::to_string(asset_id2))
    contract_asset balance1; // 第一种资产的余额.
    contract_asset balance2; // 第二种资产的余额.
    int64_t total_lq; // lq代币的总量.
    bool locked; // 当前交易对是否被锁定, 如果被锁定则无法进行交易, 也无法质押或赎回流动性.

    uint64_t primary_key() const { return index; }
    GRAPHENE_SERIALIZE(pool, (index)(balance1)(balance2))
};

typedef graphene::multi_index<N(pool), pool> pool_index;
```

#### 银行表
- 用于记录用户充值的资产, 方便gxc-swap扣款.
```c++
struct bank {
    uint64_t owner; // 记录拥有者.
    std::map<uint64_t, int64_t> balances; // 各种资产的余额, key是asset_id, value是balance.
    std::map<uint64_t, int64_t> lqs; // 各个交易对流动性代币的数量, key是pool的index, value是balance.

    uint64_t primary_key() const {return owner;}
    GRAPHENE_SERIALIZE(bank, (owner)(balances)(lqs))
};

typedef graphene::multi_index<N(bank),bank> bank_index;
```
---
### 接口
#### 1.质押流动性接口
- 向指定的交易对中质押流动性, gxc-swap将根据用户设置的参数, 从bank中扣除用户余额, 同时用户将获得对应交易对的流动性代币
```c++
//@abi action
void addliquidity(
    std::string coin1, std::string coin2 // 交易对两种代币的名字.
    , int64_t amount1Desire, int64_t amount2Desire // 用户期望质押的两种代币的数量.
    , int64_t amount1Min, int64_t amount2Min // 用户设置的最小质押的两种代币的数量.
    , std::string to // 接受流动性代币的账户.
) {
    // 检查交易对是否存在, 如果不存在则调用create pair创建.
    // 检查pool是否被锁定.
    // 检查用户在bank中持有的代币是否足够.
    // 按当前池中代币的比例计算出可以质押的金额(如果当前池中数量为0, 则以用户余额为准).
    // 检查是否满足用户设置的最小值.
    // 在bank中扣除用户代币余额.
    // 在pool中增加余额.
    // 根据比例计算应该增发的lq代币数量.
    // 在bank中增加用户持有的代币余额.
    // 增加pool中的lq代币总数.
}
```

#### 2.赎回流动性接口
- 赎回指定交易对中指定数量的流动性, gxc-swap将根据用户设置的参数, 扣除用户对应交易对中流动性代币的数量, 同时在bank中增加用户的余额
```c++
//@abi action
void removeliquidity(
    std::string coin1, std::string coin2 // 交易对两种代币的名字.
    , int64_t lq // 希望赎回的流动性代币的数量.
    , int64_t amount1Min, int64_t amount2Min // 用户设置的希望获取的两种代币的最小数量.
    , std::string to // 接受赎回代币的账户.
) {
    // 检查交易对是否存在, 不存在则出错.
    // 检查pool是否被锁定.
    // 检查用户持有的lq代币是否足够.
    // 按比例计算用户可以赎回的两种代币的数量.
    // 检查是否满足用户设置的最小值.
    // 在bank中增加用户对应代币的余额.
    // 在pool中扣除对应代币的余额.
    // 在bank中扣除用户的lq代币.
    // 在pool中扣除lq代币的总量.
}
```

#### 3. 指定输入代币数量进行交易接口
- 指定输入金额以及交换路径, 换取其他代币
```c++
//@abi action
//@abi payable
void swapexacttokensfortokens(
    std::vector<std::string> path // 用户指定的交换路径.
    , int64_t amountOutMin // 用户设置的最小输出金额.
    , std::string to // 接受交换代币的账户.
) {
    // 依次检查path[i], path[i + 1]的交易对是否存在并且没有被锁定.
    // 依次计算每一个交易对的交易结果, 并获取最终的amountOut.
    // 检查amountOut是否满足用户设置的条件.
    // 依次变动每个pool中的金额.
    // 向用户转账.
}
```

#### 4. 指定输出代币数量进行交易接口
- 指定输出金额以及交换路径, 换取其他代币
```c++
//@abi action
void swaptokensforexacttokens(
    std::vector<std::string> path // 用户指定的交换路径.
    , int64_t amountOut // 用户指定的输出的代币的数量.
    , int64_t amountInMax // 用户设置的最大输入金额.
    , std::string to // 接受交换代币的账户.
) {
    // 依次检查path[i], path[i + 1]的交易对是否存在并且没有被锁定.
    // 倒着计算每一个交易对的交易结果, 并获取最终的amountIn.
    // 检查amountIn是否满足用户设置的条件.
    // 检查用户bank中的余额是否大于amountIn.
    // 扣除bank中用户的余额, 增加bank中用户的另一种资产的余额.
    // 倒着变动每个pool中的金额.
}
```

#### 5. 质押接口
- 向bank质押代币
```c++
//@abi action
//@abi payable
void deposit() {
    // 增加bank中用户对应的余额.
}
```

#### 6. 提现接口
- 提现bank中的代币
```c++
//@abi action
void wthdraw(
    std::string coin, // 提现代币类型.
    std::string to, // 首款账户.
    int64_t amount // 提现金额.
) {
    // 检查用户对应的代币资产是否足够.
    // 扣除bank中用户对应的余额.
    // 向用户打款.
}
```

#### 7. 转移流动性代币接口
- 向其他用户转账流动性代币
```c++
//@abi action
void transferliquidity(
    std::string coin1, std::string coin2 // 交易对中的两种代币.
    , std::string to // 首款账户.
    , int64_t amount // 转账金额.
) {
    // 检查交易对是否存在.
    // 检查pool是否被锁定.
    // 检查用户的lq代币是否足够.
    // 向指定用户转移lq代币.
}
```

#### 8. 管理资金池接口
- 管理资金池的状态, 调用者必须为管理员
```c++
//@abi action
void managepool(
    std::string coin1, std::string coin2 // 交易对中的两种代币.
    , bool locked // 资金池状态.
) {
    // 检查调用者是不是管理员账户.
    // 检查交易对是否存在.
    // 改变pool的lock状态.
}
```