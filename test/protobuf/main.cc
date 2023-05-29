#include"test.pb.h"
#include<iostream>
#include<string>


int main1(){

    fixbug::LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123465");

    std::string send_str;
    if(req.SerializeToString(&send_str)){
        std::cout<< send_str << std::endl;
    }


    fixbug::LoginRequest reqB;
    if(reqB.ParseFromString(send_str)){
        std::cout<< reqB.name() <<std::endl;
        std::cout<< reqB.pwd() <<std::endl;
    }
    return 0;
}
int main(){
    // fixbug::LoginResponse rsp;
    // fixbug::ResultCode* rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("登陆处理失败了");
    fixbug::GetFriendListsResponse rsp;
    fixbug::ResultCode* rc = rsp.mutable_result();
    rc->set_errcode(0);
    fixbug::User* user1 = rsp.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(fixbug::User::MAN);
    
}   