relay 合约
-----------------
relay合约提供了充值(deposit)和提现(withdraw)方法，可以向合约充值、从合约中提取资产。充值的合约将变为指定链上的资产发放给指定的账户。

### 静态变量（Static Parameters）

静态变量在合约初始化时定义，会在合约中参与计算
```c++
const uint64_t                 adminAccount = 1234;//合约的权限拥有者
const std::vector<std::string> TARGETS = {"ETH"};//合约支持的跨链种类
const uint64_t                 MIN_DEPOSIT = 50000;//最小的deposit数量(带精度)
const uint64_t                 MIN_WITHDRAW = 50000;//最小的withdraw数量(带精度)
const uint64_t                 TXID_LIST_LIMIT = 10000;//合约中缓存的txid最大数量
const int64_t                  TIME_GAP = 86400;//withdraw取回等待时间
const uint64_t                 NUMBER_LIMIT = 10;//comfirmw一次允许取回的最大数量
```

### 数据结构
#### fundin表
deposit存入表，记录用户已经deposit而没有被确认的跨链转化条目。
```C++
struct fundin{
           uint64_t id;             //索引
           uint64_t sender;         //发起人
           uint64_t asset_id;       //存入资产
           int64_t amount;          //存入数目
           std::string coinkind;    //跨链类别
           std::string addr;        //跨链地址
           std::string txid;        //交易id
           uint64_t state;          //确认状态
           }
```
#### ctxids表
缓存一定量的txid，用于判断confirmd传入的txid是否被使用过
```c++
 struct ctxids
    {
        uint64_t id;                //索引
        std::string txid;           //交易id

    }
```
#### wtxids表
缓存一定量的txid，用于判断withdraw传入的txid是否被使用过
```c++
 struct wtxids
    {
        uint64_t id;                //索引
        std::string txid;           //交易id

    }
```
#### fundout表
withdraw请求记录表，用于记录用户的withdraw请求。
```cpp
struct fundout{
        uint64_t id;                //索引
        uint64_t to_account;        //取回账户
        uint64_t asset_id;          //取回资产
        int64_t amount;             //取回数量
        std::string from_target;    //取回目标链
        std::string txid;           //取回证明txid
        std::string from_account;   //取回源于账户
        int64_t block_time;         //请求时的区块时间
    }
```
### 接口
#### 1.用户充值
- 用户通过指定目标链和目标地址，转入相应资产，操作会被记入表中，待确认。
```C++
PAYABLE deposit(std::string target,std::string addr){
  // 判断是否存在所需的跨链种类
  // 判断账户是否存在
  // 判断资产是否存在
  // 判断余额是否充足
  // 存入合约，记录操作
}
```

#### 2.管理员确认
- 管理员确认从目标链中传回的消息，确认deposit成功
```C++
ACTION confirmd(
    uint64_t order_id, 
    std::string target,
    std::string addr, 
    contract_asset amout, 
    std::string txid){
  //判断调用者是否为管理者
  //判断order_id 是否存在
  //修改确认后的状态，txid
  //判断传入的参数与表中是否一致
  //将确认完的记录擦除
}
```
#### 3.取回资产请求
- 管理员收到目标链上的销毁请求，将withdraw请求入表。
```c++
ACTION withdraw(std::string to_account,
contract_asset amount, 
std::string from_target, 
std::string txid){
    //判断调用者是否为管理者
    //判断是否存在所需的跨链种类
    //判断所提账户和资产是否合法
    //判断合约内部的余额是否充足
    //请求信息入表
}
```
##### 4.释放已经满足等待时间的withdraw
- 管理员每隔一段时间，去确认已经满足等待时间的withdraw请求，将抵押返还。
```cpp
confirmw(){
    //判断时间是否到达
    //将质押返回
}
```
