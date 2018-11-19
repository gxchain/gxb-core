#include <graphenelib/graphene.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/global.h>

using namespace graphene;

enum class ResumeState{
    DELIVERY,           //投递中
    REJECT,             //拒绝或者取消
    ACCEPT,             //接受
    PASS,               //通过
    UNPASS              //未通过
};

class recruitment:public contract
{
public:

    typedef std::string mystring;

    recruitment(uint64_t id):contract(id),resumemul(_self,_self),appcommul(_self,_self),recruitmul(_self,_self){}

    // @abi action
    void appyourcom(mystring comname,std::string compub);

    // @abi action
    void createrec(std::string workname,std::string workcontent,uint64_t comid,uint64_t cate);

    // @abi action
    void sendresume(uint64_t workid, std::string resumelink);

    // @abi action
    void changstate(uint64_t workid,uint64_t resumeid,uint64_t state);

    // @abi action
    void closeresume(uint64_t resumeid);

    // @abi action
    void appraise(uint64_t workid,uint64_t score);

private:

    // @abi table resume i64
    struct resume{
        uint64_t resumeid;
        std::string resumelink;             //简历的可访问哈希值
        uint64_t owner;                     //二级索引
        uint8_t state;

        uint64_t primary_key() const{return resumeid;}
        uint64_t by_owner() const{return owner;}

        GRAPHENE_SERIALIZE(resume,(resumeid)(resumelink)(owner)(state))
    };
    typedef graphene::multi_index<N(resume),resume,
        indexed_by<N(owner),const_mem_fun<resume,uint64_t,&resume::by_owner>>> resume_index;
    resume_index resumemul;

    // @abi table approvecom i64
    struct approvecom{
        uint64_t comid;
        std::string comname;                //所属公司名称
        std::string compub;                 //所属公司公钥
        uint64_t owner;
        //std::vector<uint64_t> accnamelist;  //可以用该公司发布招聘的用户列表

        uint64_t primary_key() const{ return comid;}
        uint64_t by_owner() const{ return owner;}

        GRAPHENE_SERIALIZE(approvecom,(comid)(comname)(compub)(owner))
    };
    typedef graphene::multi_index<N(approvecom),approvecom,
        indexed_by<N(owner),const_mem_fun<approvecom,uint64_t,&approvecom::by_owner>>> approvecom_index;
    approvecom_index appcommul;

    struct score{
        uint64_t number;
        uint64_t arvscore;
        //score(uint64_t number,uint64_t arvscore):number(number),arvscore(arvscore){}
    };
    // @abi table recruit i64
    struct recruit{
        uint64_t workid;
        std::string workname;
        std::string workcontent;             //职位详细应该为一个可以获取内容的哈希值
        uint64_t comid;                      //所属公司id （二级索引）
        std::string comname;                 //所属公司名称
        std::string compub;                  //所属公司公钥
        uint64_t cate;                       //职位所属分类（三级索引）
        score thescore;                      //职位得到的评价分数
        std::vector<resume> reslist;         //该职位接收到的简历列表

        uint64_t primary_key()const {return workid;}
        uint64_t by_comid()const {return comid;}
        uint64_t by_cate()const {return cate;}

        GRAPHENE_SERIALIZE(recruit,(workid)(workname)(workcontent)(comid)(comname)(compub)(cate)(thescore)(reslist))
    };

    typedef graphene::multi_index<N(recruit),recruit,
        indexed_by<N(comid),const_mem_fun<recruit,uint64_t,&recruit::by_comid>>,
        indexed_by<N(cate),const_mem_fun<recruit,uint64_t,&recruit::by_cate>>> recruit_index;
    recruit_index recruitmul;    
};
GRAPHENE_ABI(recruitment,(appyourcom)(createrec)(sendresume)(changstate)(closeresume)(appraise))