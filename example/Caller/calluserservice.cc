#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char** argv){

    //整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc,argv);

    //演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    
    //rpc方法的响应
    fixbug::LoginResponse response;

    // 发起rpc方法的调用  同步的rpc调用过程  MprpcChannel::callmethod
    stub.Login(nullptr,&request,&response,nullptr); // 实际上是调用RpcChannel->callMethod 所以应该在RpcChannel中集中来做所有Rpc方法调用参数的序列化和网络发送
    
    // 一次rpc调用完成，读调用的结果即读response
    if(response.result().errcode() == 0){
        std::cout<< "rpc login response success:" << response.success() << std::endl;
    }
    else{
        std::cout<< "rpc login response error:" << response.result().errmsg() << std::endl;
    }

    //演示调用远程发布的rpc方法Register
    fixbug::RegisterRequest req;
    req.set_id(2023);
    req.set_name("yangzhan");
    req.set_pwd("6666");
    fixbug::RegisterResponse rsp;

    //以同步方式发起rpc调用请求，等待返回结果
    stub.Register(nullptr,&req,&rsp,nullptr);
    if(rsp.resultcode().errcode() == 0){
        std::cout<< "rpc register rsp success:" << rsp.success() << std::endl;
    }
    else{
        std::cout<< "rpc register rsp error:" << rsp.resultcode().errmsg() << std::endl;
    }
    
    return 0;
}