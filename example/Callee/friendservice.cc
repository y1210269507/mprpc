#include<iostream>
#include<string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"

class FriendService : public fixbug::FriendServiceRpc{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid){
        std::cout << "do GetFriendsList service! userid:" << userid << std::endl;
        
        std::vector<std::string> vec;
        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");

        return vec;
    }

    void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done){
        uint32_t userid = request->userid();

        std::vector<std::string> friendslist = GetFriendsList(userid);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for(std::string& temp : friendslist){
            response->add_friends(temp);
        }

        done->Run();
    }
};

int main(int argc, char** argv){

    LOG_INFO("first log message!");
    
    LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);


    // 发布服务
    // 1.调用框架的初始化操作
    MprpcApplication::Init(argc,argv); 

    // 2.把UserService对象发布到rpc节点上,即把本地方法转成的rpc方法注册到服务发布者上
    RpcProvider provider;//定义服务发布的对象
    provider.NotifyService(new FriendService());//

    // 3.启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();


}