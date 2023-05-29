#include<iostream>
#include<string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists

而框架做的事情就是把这个本地方法变成一个rpc远程方法
*/
//一定要记得加上fixbug，因为protobuf生成的都是fixbug的命名空间
class UserService : public fixbug::UserServiceRpc{ //使用在rpc服务的发布端，是rpc服务的提供者

public:
    bool Login(std::string name, std::string pwd){
        std::cout<< "doing local service: Login"<<std::endl;
        std::cout<< "name: "<<name << " pwd:"<<pwd <<std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd){
        std::cout<< "doing local service: Register"<<std::endl;
        std::cout<< "id:" << id << "name: "<<name << " pwd:"<<pwd <<std::endl;
        return true;
    }

    //重写基类UserServiceRpc的虚函数，下面这些方法都是框架直接调用的
    //1.caller   ===>  Login(LoginRequest)  ===> muduo  ===>  callee
    //2.callee 把Login(LoginRequest)   ===> 交给下面重写的Login方法
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done){
        //框架给业务上报了请求参数LoginRequest，本地获取相关数据做业务处理;例如本地需要name和pwd，解析出来就能调用本地函数
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool login_result = Login(name,pwd);//本地业务

        //把响应写入  包括错误码、错误消息、返回值
        fixbug::ResultCode* code = response->mutable_result();
        response->set_success(login_result);
        if(login_result){
            code->set_errcode(0);
            code->set_errmsg("");
        }
        else{
            code->set_errcode(1);
            code->set_errmsg("Login do error!");
        }


        //执行回调函数  执行响应对象数据的序列化和网络发送（由框架完成）
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done){
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id,name,pwd);

        response->mutable_resultcode()->set_errcode(0);
        response->mutable_resultcode()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }
};

int main(int argc, char** argv){
    // 发布服务
    // 1.调用框架的初始化操作
    MprpcApplication::Init(argc,argv); 

    // 2.把UserService对象发布到rpc节点上,即把本地方法转成的rpc方法注册到服务发布者上
    RpcProvider provider;//定义服务发布的对象
    provider.NotifyService(new UserService());//

    // 3.启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();
}