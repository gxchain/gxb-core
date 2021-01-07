## NFT-Contract

### 静态变量
```cpp
static const int64_t BLACKHOLEACCOUNT   = 3;    // 黑洞账户.
```


### 数据结构
#### token表
- 用于记录每一个独特的token的信息
```cpp
struct token {
            uint64_t id;//主键，token的独特id，不能一样
            std::string name;//token的名字(#序号)
            std::string link;//token图片的链接
            std::string desc;//对与token的描述
            std::string series;//token的系列
            uint64_t total;//token发行的总量
            uint64_t owner;//所有者
            uint64_t approve;//授权操作者
            uint64_t primary_key() const { return tokenid; }
            GRAPHENE_SERIALIZE(token, (tokenid)(tokenname)(tokenlink)(tokendes)(tokenseries)(total)(owner)(approve))
        };
        typedef graphene::multi_index<N(token), token> token_index;

```

#### account表
- 用于记录每个用户所拥有的token信息
```cpp
struct account
        {
           uint64_t owner;//主键，用户的id
           std::set<uint64_t> tokenids;//set，记录用户所有的tokenid
           std::set<uint64_t> allowance;//记录用户完全授权的信息
           uint64_t primary_key() const { return owner; }
           GRAPHENE_SERIALIZE(account,(owner)(tokenids)(allowance))
        };
        typedef graphene::multi_index<N(account),account> account_index;
```

### 接口
#### 1.mint创建token
- mint创建一个token,并将其分发给用户
```cpp
//@abi action
void mint(uint64_t to, uint64_t tokenid, std::string tokenuri){
        //检查token是否已经存在
        //检查是否有管理员权限
        //创建token，将其信息入tokens表
        //将token信息加入用户的收藏表中

}
```
#### 2.授权token的交易权
- approve会授权其他的用户token的交易权
```cpp
//@abi action
void approve(uint64_t tokenid, uint64_t to){
        //检查token是否存在
        //检查发送者是否被完全授权或者是否为拥有者
        //更改该token的交易权授权人，默认状态下为3，黑洞账户
}
```
#### 3.将一个用户所有权限授予另一个用户
- approveall允许用户将名下所有收藏的权限赋予另一个用户
```cpp
//@abi action
void approveall(uint64_t to){
        //检查账户是否存在
        //给用户授权
}
```
#### 4.移除所有权限
- appallremove可以让用户移除对另个用户的所有权限的授权
```cpp
//@abi action
void appallremove(uint64_t to){
        //检查是否有权限
        //移除权限
}
```
  
#### 5.交易代币
- transfer可以让拥有权限的用户交易一个代币
```cpp
//@abi action
void transfer(uint64_t tokenid, uint64_t to){
        //检查token是否存在
        //检查发送者是否被完全授权或者是否为拥有者
        //移除前拥有者的授权权限，将其改为默认的3
        //将代币拥有者更改
        //加入现拥有者的收藏
}       
```
#### 6.销毁代币
- burn销毁代币，代币存在，但无法进行任何的操作
```cpp
//@abi action
void burn(uint64_t tokenid){
        //检查代币是否存在
        //检查是否权限修改
        //将代币拥有者改为3黑洞账户
        //将代币权限移除为3黑洞账户
}

```
