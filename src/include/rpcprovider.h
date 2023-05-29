#pragma once
#include "google/protobuf/service.h"
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/InetAddress.h>
#include<muduo/net/TcpConnection.h>
#include<string>
#include<functional>
#include<google/protobuf/descriptor.h>
#include<unordered_map>
#include "logger.h"

//框架提供的专门负责发布rpc服务的网络对象类
//提供者可以包含多个服务，而每个服务又包含多个服务方法
class RpcProvider{
public:
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service* service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();

private:
    //因为provider对象是服务器，所以需要tcp连接,需要epoll
    //本来这里应该写一个TCPServer，但是tcp连接只需要在服务启动的时候创建，这个变量写到Run()方法中就好
    //组合EventLoop，即epoll
    muduo::net::EventLoop m_eventLoop;

    //新的socket连接回调函数
    void OnConnection(const muduo::net::TcpConnectionPtr& );

    //service服务类型信息
    struct ServiceInfo
    {
        //记录服务对象，根据服务对象可以获取服务描述器，服务描述器可以获得服务名字、服务方法
        google::protobuf::Service* m_service;//保存服务对象
        //记录服务方法对象，服务方法名对应服务方法的描述器
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;//保存服务方法
    };
    //存储注册成功的服务对象和器服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    //已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*,muduo::Timestamp);

    // Closure回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};