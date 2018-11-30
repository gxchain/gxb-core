#include "recruitment.hpp"

void recruitment::appyourcom(std::string comname,std::string compub)
{
    //创建认证公司table
    auto owner=get_trx_sender();
    appcommul.emplace(owner,[&](auto &obj){
        obj.comid=appcommul.available_primary_key();
        obj.comname=comname;
        obj.compub=compub;
        obj.owner=owner;
    });
}

void recruitment::createrec(std::string workname,std::string workcontent,uint64_t comid,uint64_t cate)
{

    
    //参数验证,错误则回滚交易
    auto itor=appcommul.find(comid);
    graphene_assert(itor!=appcommul.end(),"错误：发布招聘启事，未发现该公司ID");
    
    auto owner=get_trx_sender();
    recruitmul.emplace(owner,[&](auto &obj){
        obj.workid=recruitmul.available_primary_key();
        obj.workname = workname;
        obj.workcontent = workcontent;
        obj.comid=comid;
        obj.comname = itor->comname;
        obj.compub = itor->compub;
        score temp;
        temp.number=0;
        temp.arvscore=0;
        obj.thescore=temp;
        obj.cate=cate;
    });
}

void recruitment::sendresume(uint64_t workid,std::string resumelink)
{
    //参数验证，错误回滚交易
    auto itor= recruitmul.find(workid);
    graphene_assert(itor!=recruitmul.end(),"错误：没有该职位ID");
    auto owner=get_trx_sender();
    auto resumeid=resumemul.available_primary_key();
    resumemul.emplace(owner,[&](auto &obj){
        obj.resumeid=resumeid;
        obj.resumelink=resumelink;
        obj.owner=owner;
        obj.state=(uint8_t)ResumeState::DELIVERY;
    });
    recruitmul.modify(itor,_self,[&](auto &obj){
        resume objitem;
        objitem.resumeid=resumeid;
        objitem.resumelink=resumelink;
        objitem.owner=owner;
        objitem.state=(uint8_t)ResumeState::DELIVERY;
        obj.reslist.emplace_back(objitem);
    });
}

void recruitment::changstate(uint64_t workid, uint64_t resumeid,uint64_t state)
{
    auto recitor=recruitmul.find(workid);
    graphene_assert(recitor!=recruitmul.end(),"错误：没有该职位ID");
    auto resitor= resumemul.find(resumeid);
    graphene_assert(resitor!=resumemul.end(),"错误：没有该简历ID");
    auto comitor = appcommul.find(recitor->comid);
    graphene_assert(comitor != appcommul.end(),"错误：没有该公司id");
    auto owner=get_trx_sender();
    graphene_assert(comitor->owner ==owner,"错误：该账户没有权限修改简历状态");
    for(auto i=0;i<recitor->reslist.size();i++){
        if(i==recitor->reslist.size()-1&&
                resumeid != recitor->reslist[i].resumeid){
            graphene_assert(true,"错误：没有该简历ID");
        }
    }
    
    recruitmul.modify(recitor,_self,[&](auto &obj){
        for(auto it:obj.reslist){
            if(it.resumeid==resumeid){
                it.state=state;
            }
        }
    });

    resumemul.modify(resitor,_self,[&](auto &obj){
        obj.state=state;
    });
}

void recruitment::closeresume(uint64_t resumeid)
{
    
    auto owner=get_trx_sender();
    auto resitor=resumemul.find(resumeid);
    graphene_assert(resitor != resumemul.end(),"错误：简历不存在");
    graphene_assert(resitor->owner==owner,"错误：该账户没有权限关闭简历");
    resumemul.modify(resitor,_self,[&](auto &obj){
        obj.state=(uint8_t)ResumeState::REJECT;
    });
}

void recruitment::appraise(uint64_t workid,uint64_t score)
{

    auto owner=get_trx_sender();
    auto recitor = recruitmul.find(workid);
    graphene_assert(recitor!=recruitmul.end(),"错误：职位不存在");
    for(auto i=0;i<recitor->reslist.size();i++){
        if(i==recitor->reslist.size()-1&&
                owner != recitor->reslist[i].owner){
            graphene_assert(true,"错误：该职位没有此用户的简历，无法评价");
        }
    }
    auto scope_avg=(recitor->thescore.arvscore *recitor->thescore.number+score)/(recitor->thescore.number+1);
    auto new_number=recitor->thescore.number+1;
    recruitmul.modify(recitor,_self,[&](auto &obj){
        obj.thescore.arvscore=scope_avg;
        obj.thescore.number=new_number;
    });
}