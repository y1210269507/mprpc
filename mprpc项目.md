# mprpc分布式网络通信框架项目

mp = 基于muduo高性能网络库 + Protobuf开发

rpc = 远程过程调用

## 集群、分布式以及mprpc项目做了什么事

> 单机服务器的限制是什么

![image-20230522140334851](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522140334851.png)

1. 集群解决的问题，比如说一台服务器的内存是4g，一条连接占用的内存是4k，那么一台服务器最多能承载100w个连接；如果超出这个限制就需要添加新的服务器
2. 分布式解决的问题，比如说用户管理模块里面出了个小bug，目前整个项目规模很大，编译部署时间需要很多小时；如果要修改用户管理模块的话，就需要将整个项目下线，修改bug后再重新编译、部署，非常费时间。如果对项目分模块部署，则可以下线用户管理模块，修改bug，再上线用户管理模块，其他模块不用修改
3. 根据项目中模块的业务，应该选择更符合的硬件资源。CPU密集型是计算量大的模块，应该放到高性能CPU机器上；I/O密集型经常处理输入输出，需要放到内存、带宽较大的机器上。如果全部放到同一台机器上，那么这台机器的硬件资源就需要对CPU密集和I/O密集做权衡。

> 集群服务器的好处和限制是什么

![image-20230522142048145](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522142048145.png)

每台服务器独立运行一个项目的所有模块

需要一个负载均衡器，将请求分发给集群中的各个机器

好处：改进了问题1，而且**很简单**

坏处：问题2、问题3没有解决

对于后台管理模块，并没有那么大的并发量，所以后台管理模块是不需要部署到多台服务器上的。但由于集群还是将项目中各个模块看作整体，所以每台机器上都有一个后台管理模块，这就导致了资源的浪费

> 分布式的好处和限制是什么

一个项目拆分了很多个模块，每个模块独立部署在一个服务器主机上，所有服务器协同工作才能实现项目的全部功能，每一台服务器称作分布式的一个节点，根据节点的并发要求，对一个节点再进行集群部署

![image-20230522150051349](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522150051349.png)

1. 所有的节点加起来才算实现了一个项目；集群是每台机器就是一个项目
2. 对于需要高并发的节点，可以再做集群部署；问题1，2，3都能解决

 存在的问题：

1. 怎么把系统拆分成一个个模块
   1. 各模块可能会实现大量重复的代码，软件设计师来解决
2. 各模块之间该怎么交互信息
   1. **各模块都运行在不同机器的进程里面**（网络通信）
   2. 各模块在同一个机器的不同进程里面（进程间通信）

> 分布式网络通信框架做了什么

![image-20230522152711524](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522152711524.png)

分布式需要把模块运行在不同的主机上，但模块需要协同工作才能实现整个项目的功能，协同过程中避免不了模块与模块之间的调用。比如用户管理模块登录了一个用户之后，想要调用好友管理模块来获取用户的好友列表，由于这两个模块在不同的主机上，所以需要使用网络通信。

而将这个网络通信过程封装起来，让远程调用透明化，让用户能够像在同一个进程中模块互相调用那样使用跨主机的模块调用，就是分布式通信框架要做的事情。

## RPC通信原理

RPC（Remote Procedure Call Protocol）**远程过程调用协议**

分布式通信也就是RPC通信

RPC通信也就是不同机器上模块之间互相调用，需要传递信息这个过程

举个例子：server1上的用户管理模块想要调用server2上的好友管理模块里的``GetUserFriendLists``方法，首先需要把函数名和参数打包（序列化过程），然后通过muduo网络发送给server2，server2网络接收后解包（反序列化）得到函数名和参数，进行方法调用得到一个返回结果。对返回结果序列化，通过muduo传递给server1，server1解包就得到了结果。

![image-20230522155419559](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522155419559.png)

用户在红色部分调用RPC方法

由于RPC通信过程对用户是透明的，所以需要在User-stub和RPCRuntime中实现RPC通信过程

黄色部分：涉及rpc方法、参数的打包和解析的实现，也就是数据的序列化和反序列化，使用Protobuf

> Protobuf比json的好处
>
> 1. Protobuf是二进制存储，json是文本存储，前者占用空间小
> 2. Protobuf不存储额外信息，json是key-value对
>    1. json。name:"zhang san"，pwd:"123456"
>    2. Protobuf。"zhang san"，"123456"

绿色部分：网络通信部分，包括寻找rpc服务主机，发起rpc调用请求和响应rpc调用结果，使用muduo网络库和zookeeper服务配置中心（专门做服务发现，即知道A调用的方法是B，C，D中谁的）。

mprpc实现的主要是黄色和绿色部分

## 环境配置

### 项目代码工程目录

![image-20230522222412388](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230522222412388.png)

### 需要的库

1. boost库
   1. 出错时可以更新python版本
   2. [(66条消息) C++网络编程 - Boost::asio异步网络编程 - 01- boost库源码编译安装_大秦坑王的博客-CSDN博客](https://blog.csdn.net/QIANGWEIYUAN/article/details/88792874)
2. Muduo库
   1. 高版本Muduo库会有函数的弃用，老版本代码不能用太新版本的Muduo
   2. [(66条消息) C++ muduo网络库知识分享01 - Linux平台下muduo网络库源码编译安装_大秦坑王的博客-CSDN博客](https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980)

3. protobuf
   1. linux下连接不上github问题
   2. 旧版本的protobuf使用``./autogen.sh``，高版本的直接使用``./configure``
   3. 使用的时候没法动态链接protobuf输入命令``export LD_LIBRARY_PATH=/usr/local/lib``

## Protobuf的使用

Protobuf是用来序列化和反序列化的

序列化是指将函数名，函数的参数都转成二进制存储发送到网络

反序列化是指将二进制存储还原成函数名，函数的参数

### 在test.proto文件中声明消息类型

```protobuf
syntax = "proto3";//声明protobuf的版本

package fixbug;//声明了代码所在的包（即namespace fixbug）
//所有的类型可以分为：数据、列表、映射表
//数据就是一个单独的数据
//列表就是一个list
//映射表是一个map
//定义登陆请求消息类型 name pwd
message LoginRequest{
    bytes name = 1;//第一个字段
    bytes pwd = 2;//第二个字段
}

//定义登陆响应消息类型
message LoginResponse{
    int32 errcode = 1;
    bytes errmsg = 2;
    bool success = 3;
}
```

然后调用命令``protoc test.proto --cpp_out ./``

就会把test.proto文件转成.cc和.h文件，这样就可以使用C++直接调用，对于``message``类型，会生成对应的消息类，继承于父类Message

![image-20230523214415793](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230523214415793.png)

### 如何序列化

```c++
//封装了login请求对象的数据
LoginRequest req;
req.set_name("zhang san");
req.set_pwd("123465");

//序列化成String类型需要有一个回调参数 
std::string send_str;
if(req.SerializeToString(&send_str)){
    std::cout<< send_str << std::endl;
}

//结果：zhang san123465
```

### 如何反序列化

```c++
//用一个新对象接收反序列后的信息，需要传入的参数是序列化后的序列send_str
LoginRequest reqB;
if(reqB.ParseFromString(send_str)){
    std::cout<< reqB.name() <<std::endl;
    std::cout<< reqB.pwd() <<std::endl;
}
//结果：
//zhang san
//123465
```

### 注意的点

一般string类型直接写成bytes，这样可以避免string再进行转码

```protobuf
message LoginRequest{
    bytes name = 1;//第一个字段
    bytes pwd = 2;//第二个字段
}
```



如果是一个对象里面引用另一个对象，那么想要改变需要用mutable方法

```protobuf
message ResultCode{
    int32 errcode = 1;
    bytes errmsg = 2;
}
message LoginResponse{
    ResultCode result = 1;
    bool success = 2;
}

fixbug::LoginResponse rsp;
fixbug::ResultCode* rc = rsp.mutable_result();//使用mutable方法
rc->set_errcode(1);
rc->set_errmsg("登陆处理失败了");
```



### 列表举例

```protobuf
message GetFriendListsRequest{
    uint32 userid = 1;
}

message User{
    bytes name = 1;
    uint32 age = 2;
    enum Sex{
        MAN = 0;
        WOMEN = 1;
    }
    Sex sex = 3;
}

message GetFriendListsResponse{
    ResultCode result = 1;
    //只要带了repeated就表示是个列表
    repeated User friend_list = 2;
}

fixbug::GetFriendListsResponse rsp;
fixbug::ResultCode* rc = rsp.mutable_result();
rc->set_errcode(0);
fixbug::User* user1 = rsp.add_friend_list();//想要添加就调用add方法，得到一个指针
//通过指针对列表中新添加的对象进行修改
user1->set_name("zhang san");
user1->set_age(20);
user1->set_sex(fixbug::User::MAN);
```

### protobuf怎么定义描述rpc方法的类型 - service

目前定义的消息类型只有传递的实参（Request）、返回值（Response）

rpc方法是给**用户**使用的，由三部分组成：**方法名**、**实参**、**返回值**

我们怎么把方法名与实参、返回值一一对应起来呢

使用protobuf的service，这个描述**只是个描述**，**本身没有实现rpc方法**，虽然本身没有实现rpc方法，但可以**通过指定**实现rpc方法实参的序列化和反序列化以及网络通信

 ```protobuf
 // 定义下面的选项，表示生成service服务类和rpc方法描述，默认不生成
 option cc_generic_services = true;
 
 service UserServiceRpc{
 	//将Login方法名与实参LoginRequest与返回值LoginResponse消息类型联系起来
     rpc Login(LoginRequest) returns(LoginResponse);
     
     rpc GetFreindLists(GetFriendListsRequest) returns(GetFriendListsResponse);
 }
 ```

protobuf中的service会被转换成C++中的类，继承父类Service

service里定义的rpc方法也会成为对应的类成员函数，此外还会生成一个获取服务描述器的方法

**服务描述器可以告知服务的名字和服务包含的方法**

![image-20230523215233017](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230523215233017.png)

### 总结

protobuf中**message**类型主要用来**参数**和**返回值的序列化和反序列化**

protobuf中**service**类型是**rpc方法的服务类**，会生成两个类

1. ServiceRPC，是Callee部分中真正执行rpc方法的类，是rpc服务的提供者
2. ServiceRPC_Stub，是Caller部分中执行的代理类，负责实现底层的细节（序列化，反序列化，网络通信）；该类构造函数需要传入一个RpcChannel，我们在派生类的CallMethod方法实现底层细节  

## 本地方法怎么发布成rpc服务

简单来说就是怎么让这个本地方法能够被远程调用。

在rpc框架中，Caller是rpc方法的请求者，Callee是rpc方法的提供者

在Callee下实现一个Login方法，这时Login方法是本地方法，如果我们想要把Login变成rpc方法，就需要使用rpc框架。

远程调用时需要传递rpc方法名、函数、返回值，使用protobuf进行序列化和反序列化，protobuf中message生成参数和返回值的消息类型、service负责描述rpc方法。

本地服务变成rpc服务主要经过两步：

1. 在proto文件中对rpc方法进行描述
   1. 使用message，生成参数消息类型和返回值消息类型
   2. 使用service，将rpc方法名与参数消息类型、返回值消息类型绑定起来；这样就生成了对rpc方法的描述
   3. 这样rpc的服务者和rpc的消费者就相当于达成了“协议”，rpc方法的发送和接收就按照rpc方法的描述来执行

```protobuf
syntax = "proto3";

package fixbug;

option cc_generic_services = true;
//这一步是标识rpc方法的参数和返回值
message ResultCode{
    int32 errcode = 1;
    bytes errmsg = 2;
}

message LoginRequest{
    bytes name = 1;
    bytes pwd = 2;
}

message LoginResponse{
    ResultCode result = 1;
    bool success = 2;
}

//这一步是标识rpc方法名
service UserServiceRpc{
    rpc Login (LoginRequest) returns (LoginResponse);
}
```

2. 重写proto文件生成的服务类的rpc方法

下列代码角色是本地，还在调用框架的阶段，而不是实现框架的阶段

```c++
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
    //重写基类UserServiceRpc的虚函数，下面这些方法都是框架直接调用的
    //1.caller   ===>  Login(LoginRequest)  ===> muduo  ===>  callee
    //2.callee 把Login(LoginRequest)   ===> 交给下面重写的Login方法
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done){
        //框架给业务上报了请求参数LoginRequest，本地获取相关数据做业务处理
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool login_result = Login(name,pwd);//本地业务

        //把响应写入  包括错误码、错误消息、返回值
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调函数  执行响应对象数据的序列化和网络发送（由框架完成）
        done->Run();
    }
};
//怎么使用呢？
int main(int argc, char** argv){
    // 发布服务
    // 1.调用框架的初始化操作
    MprpcApplication: :Init(argc,argv); 

    // 2.把UserService对象发布到rpc节点上,即把本地方法转成的rpc方法注册到服务发布者上
    RpcProvider provider;//定义服务发布的对象
    provider.NotifyService(new UserService());//

    // 3.启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();
}
```

> 为什么要重写框架中的Login方法

远端想要调用本地的Login方法，这个请求是**由框架来接收**，框架收到请求解析传来的参数（request），然后调用本地服务进行业务操作，**将结果写入到返回值**（response）再通过框架传回远端（done->Run()，涉及到把结果序列化，然后传到网络等等操作），远端就得到了结果

也就是说框架是远端和本地之间沟通的桥梁，是一个中介

## mprpc框架基础类设计

```c++
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
```

根据使用框架时提到的类，我们要去实现它们

> MprpcApplication

```c++
//mprpcapplication.h
#pragma once

//mprpc框架的基础类 负责框架的初始化操作
//框架只需要一个，所以使用单例
class MprpcApplication{
public:
    static void Init(int argc, char** argv);
    static MprpcApplication& GetInstance();
private:
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
};
```

```c++
//mprpcapplication.cc
#include "mprpcapplication.h"

//框架初始化时，一般就是读取写好的配置文件，所以需要在命令行中指明，因此需要以下两步
//1. 传入终端输入的参数列表
//2. 加载配置文件
void MprpcApplication::Init(int argc, char** argv){
    //对参数列表进行判断
    if(argc < 2){
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
    std::string config_file;
    int c = 0;
    //从参数列表中获得i，因为我们的命令格式是：command -i <configfile>
    while((c = getopt(argc, argv, "i:")) != -1){
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }     
    }

    // 开始加载配置文件了 rpcserver_lp=  rpcserver_port=  zookeeper_ip=  zookepper_port=
    
}
MprpcApplication& MprpcApplication:: GetInstance(){
    static MprpcApplication app;
    return app;
}
```

> RpcProvider

```c++
//rpcprovider.h
#pragma once
#include "google/protobuf/service.h"

//框架提供的专门负责发布rpc服务的网络对象类
class RpcProvider{
public:
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service* service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();
};
```

```c++
//rpcprovider.cc
#include "rpcprovider.h"

// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service){

}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run(){
    
}
```

 

## mprpc框架的配置文件加载

根据任务驱动，我们首先要实现框架的初始化功能：即读取配置文件初始化

> mprpc框架需要的配置文件内容是什么呢

四个部分：rpcserverip、rpcserverport、zookeeperip、zookeeperport

> 实现对这四个部分的读取

首先需要创建一个类，将方法封装在里面，面向对象的思想

要实现两个功能：

1. 给定一个配置文件的名字，能够读取里面的内容
2. 根据key获得value

```c++
//mprpcconfig.h
// rpcserverip rpcserverport  zookeeperip  zookeeperport
// 框架读取配置文件类
class MprpcConfig{
public:
    //负责解析加载配置文件
    void LoadConfigFile(const char* config_file);
    // 查询配置项信息
    std::string Load(std::string key);
private:
    std::unordered_map<std::string,std::string> m_configMap;
    // 去掉字符串多余的空格
    void Trim(std::string &src_buf);
};
```

```c++
//mprpcconfig.cc
void MprpcConfig::Trim(std::string &src_buf){
    int idx = src_buf.find_first_not_of(' ');
    if(idx != -1){
        //说明字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }

    //去掉字符串后面的空格
    idx = src_buf.find_last_not_of(' ');
    if(idx != -1){
        src_buf = src_buf.substr(0,idx + 1);
    }
        
}

//负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char* config_file){
    
    FILE* pf = fopen(config_file,"r");
    if(pf == nullptr){
        std::cout<< config_file << "is not exist!"<<std::endl;
        exit(EXIT_FAILURE);
    }

    //1.注释   2.正确的配置项 =  3.去掉开头多余的空格
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf,512,pf);
        
        //去掉字符串前面的空格
        std::string read_buf(buf);
        
        Trim(read_buf);
        // 判断#的注释
        if(read_buf[0] == '#' || read_buf.empty()){
            //std::cout<<"1"<<std::endl;
            continue;
        }

        // 解析配置项
        int idx = read_buf.find('=');
        if(idx == -1){//千万不能把==写成=
            //配置项不合法
            //std::cout<<"2"<<std::endl;
            continue;
        }

        std::string key;
        std::string value;
        key = read_buf.substr(0,idx);
        Trim(key);
        //rpcserverip=127.0.0.1\n
        int end_idx = read_buf.find('\n',idx);
        value = read_buf.substr(idx + 1,end_idx -1 - idx);
        Trim(value);
        m_configMap.insert({key,value});
        
    }
    
}
// 查询配置项信息
std::string MprpcConfig::Load(std::string key){
    auto it = m_configMap.find(key);
    if(it == m_configMap.end()){
        
        return "";
    }
    return it->second;
}
```



> 遇到的问题

1. 使用时链接不上protobuf动态库：将动态库所在的目录写入库配置文件

```bash
1.将用户用到的库统一放到一个目录，如 /usr/loca/lib
# cp libXXX.so.X /usr/loca/lib/           

2.向库配置文件中，写入库文件所在目录
# vim /etc/ld.so.conf.d/usr-libs.conf    
  /usr/local/lib  

3.更新/etc/ld.so.cache文件
# ldconfig  
```

2. 在.cc中引入.h文件失败：如果include路径没问题，就是vscode抽风了，cmake一下

3. 在静态方法中不能访问非静态成员变量：将成员变量改成静态的

4. 一个类中成员变量未定义的引用：.h中的静态成员变量要在.cc中初始化

5. 类中成员函数在链接时未定义的引用：因为CMakeLists没有改变，导致没有重新生成makefile，新添加的.h和.cpp文件就没有添加进去，所以查询不到。使用新版本cmake就好/删除build下的内容 
6. char* 类型转成string会把``\n``也给带上

## 开发rpcprovider的网络服务

框架初始化后，需要将rpc服务注册到provider里，供请求方远程调用，这需要网络服务

同时provider作为服务器，可以接受多个连接，那么就需要考虑高并发

于是采用Muduo库来实现，我们首先把provider实现成一个服务器，即实现``provider.Run()``方法

> 首先在rpcprovider.h中声明需要实现的方法

因为要使用muduo库将provider作为服务器，所以需要有TCP连接和Epoll

Epoll中连接上发生的事件有1、连接请求事件 2、读写事件；所以需要有对应的回调函数

这是Reactor的事件处理模式

```c++
//框架提供的专门负责发布rpc服务的网络对象类
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


    //已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*,muduo::Timestamp);
};
```

> 在rpcprovider.cc中具体实现

我们点进去TcpServer类发现需要三个参数，分别是EventLoop、InetAddress、string服务器的名字

InetAddress要根据配置文件中规定的服务器ip和port来创建对象，需要调用框架获取





```c++
void RpcProvider::Run(){
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port);
    
    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");

    //绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    //muduo库使用的是C方法，而不是对象的方法，如果想要使用的话，那么需要用bind()来把对象方法改成C方法
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));

    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    //设置muduo库的线程数量
    server.setThreadNum(4);

    std::cout<<"RpcProvider start service at ip:"<<ip<<" port:"<<port<<std::endl;
    // 启动网络服务
    server.start();

    //相当于epoll.wait()，等待epoll中连接上有事件发生
    m_eventLoop.loop();
}

```

## rpcprovider发布服务方法

> 理论部分

视频18理论详解

目前我们已经实现了框架的网络通信部分（通过Muduo），也就是让服务提供者作为服务器，服务请求者请求远程调用rpc方法时向服务器发起请求。

下面我们来实现根据服务请求者发起的请求，框架去调用请求的rpc方法

![image-20230525194435659](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230525194435659.png)

我们知道在UserService里面重写了一个Login方法，这个是给框架用的，当框架收到远端要调用这个rpc方法时，框架就会调用这个Login方法。那么我们怎么在框架中注册这个方法呢？

> 实现NotifyService（Service*）

NotifyService就是把服务给注册到服务提供者里，举个例子：现在有一个UserService服务，服务提供者是RpcProvider，我们现在要在RpcProvider里记录下UserService这个服务，以及服务里包含的服务方法

```c++
void RpcProvider::NotifyService(google::protobuf::Service* service){
    
    //记录当前传入的服务的相关信息，如服务对象，服务对象的方法
    ServiceInfo service_info;
    service_info.m_service = service;
    
    //获取了服务对象的描述信息       可以知道这个服务叫什么名字，里面包含的服务对象方法有什么     eg: 服务名：UserService，  方法：Login
    const google::protobuf::ServiceDescriptor* pserviceDesc = service->GetDescriptor();

    //获取服务的名字
    std::string service_name = pserviceDesc->name();
    std::cout<<"当前登记的服务是: "<<service_name<<std::endl;
    std::cout<<"登记"<<service_name<<"服务下包含的服务方法："<<std::endl;
    //获取服务对象的方法的数量
    int methodCnt = pserviceDesc->method_count();

    for(int i = 0; i < methodCnt; i++){
        //获取了服务对象中指定下标i的服务方法的描述
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name,pmethodDesc});
        std::cout<<"当前登记的方法是："<<method_name<<std::endl;
    }
    //将当前服务注册到服务提供者中
    m_serviceMap.insert({service_name,service_info});
}
```

> 遇到的问题

const类型值不能初始化非const的左值：左值接收const的值，该左值需要加const

不允许指针指向不完整的类类型：这说明类只是声明了，**但没有定义**，也就是说找不到这个类。引入相关的头文件即可

## RpcProvider分发rpc服务

现在已经在Provider中注册了rpc方法，也实现了网络的通信，现在该处理接收到rpc请求后怎么调用注册好的rpc方法；这一步应该在处理读写事件中完成

> 接收请求端发来的rpc请求

rpc请求应该包含的是服务名 服务中的方法名 参数

我们已经在Callee中写好了参数的proto文件，解决了参数的序列化和反序列化问题

现在为服务名和服务中的方法名来实现一个proto文件

```protobuf
syntax = "proto3";

package mprpc;

message RpcHeader{
    bytes service_name = 1;
    bytes method_name = 2;
    uint32 args_size = 3;
}
```

因为序列化后，不包含额外的信息，内容都挤在一起``UserServiceLoginzhang san123456``

所以我们要解析出来谁是服务名，谁是方法名，谁是参数

```c++
/*
在框架内部，RpcProvider和RpcConsumer协商好通信用的protobuf消息类型，就是数据该以什么样形式传输
service_name  method_name  args，传服务名->根据服务名得到所有的method->传method名->调用指定的method  对于args，使用已有的message类型进行数据的序列化和反序列化；对于service_name和method_name重新创建message

新建立的message包含 service_name  method_name  args_size(用来处理TCP粘包问题)

16UserServiceLoginzhang san123456

header_size(4个字节) + header_str + args_str
*/
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr&conn, muduo::net::Buffer* buffer,muduo::Timestamp){
    //第一部分：收到请求，解析请求携带的数据

    //网络上接收的远程rpc调用请求的字符流  Login  args
    std::string recv_buf = buffer->retrieveAllAsString();

    //问题：如果按照十进制存储的话，我们不知道4个字节存储的数在string类型中占了几位：10占了2位，100占了3位；   解决方法：二进制存储，使用string的insert和copy方法，针对固定内存很好用
    //从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size,4,0);
    
    //根据header_size读取数据头的原始字符流,反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str)){
        //数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else{
        //数据头反序列化失败
        std::cout<<"rpc_header_str: "<<rpc_header_str<<" parse error!"<<std::endl;
        return;
    }

    //获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    //打印调试信息
    std::cout<<"======================================================================="<<std::endl;
    std::cout<<"header_size: "<<header_size<<std::endl;
    std::cout<<"rpc_header_str: "<<rpc_header_str<<std::endl;
    std::cout<<"service_name: "<<service_name<<std::endl;
    std::cout<<"method_name: "<<method_name<<std::endl;
    std::cout<<"args_str: "<<args_str<<std::endl;
    std::cout<<"======================================================================="<<std::endl;
}
```

解析出来后，就需要根据请求端请求的服务，方法，参数来调用本地写好的业务方法

```c++
//第二部分 获取service对象和method对象，生成返回值返回给请求方
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end()){
        std::cout<<service_name<<"is not exist!"<<std::endl;
        return;
    }

    
    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end()){
        std::cout<<service_name<<" : "<<method_name<<" is not exist!"<<std::endl;
    }

    google::protobuf::Service* service = it->second.m_service;//获取service对象 new UserService

    const google::protobuf::MethodDescriptor* method = mit->second;//获取method对象 Login方法

    // rpc方法调用需要request和response参数，详见userservice.cc中Login方法；生成的请求request和响应response
    //这一步从抽象中获得了具体的方法所需要的request和response，即通过Login方法获得了LoginRequest和LoginResponse，看user.proto
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str)){
        std::cout<<"request parse error! content:"<< args_str<<std::endl;
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    //给下面method方法调用绑定一个Closure的回调函数, 当框架处理完业务后，还需要把返回值发送给请求端，这需要网络通信功能以及把返回值序列化

    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr& ,google::protobuf::Message*>(this, &RpcProvider::SendRpcResponse, conn, response);
    //现在框架可以根据远程rpc请求，调用当前节点发布的rpc方法; 即调用UserService里的Login方法
    service->CallMethod(method, nullptr,request,response, done);
```

> rpc响应回调的实现

上文中实现的NewCallback会生成一个Closure对象，传入的是SendRpcResponse方法。CallMethod方法会调用Login方法，Login方法会调用这个Closure对象的Run方法即SendRpcResponse方法

因此我们需要在SenRpcResponse方法中实现1、网络通信2、返回值序列化

```c++


void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response){
    std::string response_str;
    //将返回值序列化
    if(response->SerializeToString(&response_str)){
        //序列化成功后 通过网络把rpc方法执行的结果发送回rpc请求方
        conn->send(response_str);
    }
    else{
        std::cout<< "Serialize response_str error!"<<std::endl;
    }
    conn->shutdown();// 模拟http短连接服务, 由rpcprovider主动断开连接
}
```

>出现的问题

调用NewCallBack函数时，如果不指定传入的类型，靠读取参数自动获取会出错：通过指定传入的类型来解决该问题

## RpcChannel的调用过程

我们已经完成了rpc服务提供的代码，下面进行rpc服务调用的代码开发

proto文件中生成两个服务类ServiceRpc（生产者用）、ServiceRpc_Stub（消费者用）；在ServiceRpc_Stub中调用Login函数，其实是传入的RpcChannel调用callMethod方法。

我们知道请求方实际上是把rpc方法的描述传给提供方，由提供方得到结果后再返回过来。

首先在用户代码里发出rpc请求

```c++
//calluserservice.cc
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
    return 0;
}
```

我们知道调用sub.Login其实是调用RPCChannel->callMethod，而RpcChannel是一个抽象类，我们需要实现一个派生类来重写callMethod方法

callMethod方法实现的内容主要有:1、将请求序列化 2、网络通信

```c++
//约定好的消息格式: header_size + service_name method_name args_size + args
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done){
    
    //获取 service_name method_name
    const google::protobuf::ServiceDescriptor* sd =  method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    //获取参数的序列化字符串长度 args_size
    std::string args_str;
    uint32_t args_size = 0;
    if(request->SerializeToString(&args_str)){
        args_size = args_str.size();
    }
    else{
        std::cout<< "Serialize request error!"<<std::endl;
        return;
    }

    //定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str)){
        header_size = rpc_header_str.size();
    }
    else{
        std::cout<< "Serialize rpc header error!"<<std::endl;
        return;
    }

    //组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0,std::string((char*)&header_size,4));// 前4个字节放二进制的header_size
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    //打印调试信息
    std::cout<<"======================================================================="<<std::endl;
    std::cout<<"header_size: "<<header_size<<std::endl;
    std::cout<<"rpc_header_str: "<<rpc_header_str<<std::endl;
    std::cout<<"service_name: "<<service_name<<std::endl;
    std::cout<<"method_name: "<<method_name<<std::endl;
    std::cout<<"args_str: "<<args_str<<std::endl;
    std::cout<<"======================================================================="<<std::endl;

    // 使用TCP编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);

    if(clientfd == -1){
        std::cout<<"create socket error !errno: "<<errno <<std::endl;
        exit(EXIT_FAILURE);
    }
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        std::cout<<"connect error !errno: "<<errno <<std::endl;
        close(clientfd);
        exit(EXIT_FAILURE);
    }
    
    //发送rpc请求
    if(send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0) == -1){
        std::cout<<"send error !errno: "<<errno <<std::endl;
        close(clientfd);
        return;
    }



    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size =0;
    if((recv_size = recv(clientfd,recv_buf,1024,0) ) == -1){
        std::cout<<"recv error !errno: "<<errno <<std::endl;
        close(clientfd);
        return;
    }

    //将响应值写入response，用户端就能感受到响应值了
    //反序列化rpc调用的响应数据
    std::string response_str(recv_buf,0,recv_size);
    if(response->ParseFromString(response_str)){
        std::cout<<"response error !errno: "<< response_str <<std::endl;
        close(clientfd);
        return;
    }

    close(clientfd);
}	
```

调用流程如下图，在调用时是使用stub.Login，但其实内部实现是RpcChannel->callMethod方法

因此会去调用我们自己写的MprpcChannel的callMethod方法

callMethod方法实现了rpc请求的序列化、通过网络发送rpc请求、接收rpc的响应、将响应反序列化![image-20230527131157033](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230527131157033.png)

> 存在问题

代码逻辑问题：std::string response_str(recv_buf,0,recv_size); // 出现问题，recv_buf遇到\0后面数据就丢弃了，导致反序列化失败

## RpcController实现

RpcController是在用户端调用Login方法时传入的，我们知道Login实际上是调用RpcChannel的callMethod方法，在这个方法过程中有很多可能出错导致方法提前结束的地方。RpcController就可以**记录**过程中的**错误信息**

```c++
//mprpccontroller.h
class MprpcController : public google::protobuf::RpcController{
public:
    MprpcController();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void SetFailed(const std::string& reason);

    //目前未实现功能
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);
private:
    bool m_failed;// RPC方法执行过程中的状态
    std::string m_errText; // RPC方法执行过程中的错误信息
};
```

```c++
//mprpccontroller.cc
MprpcController::MprpcController(){
    m_failed = false;
    m_errText = "";
}

void MprpcController::Reset(){
    m_failed = false;
    m_errText = "";
}

bool MprpcController::Failed() const{
    return m_failed;
}

std::string MprpcController::ErrorText() const{
    return m_errText;
}

void MprpcController::SetFailed(const std::string& reason){
    m_failed = true;
    m_errText = reason;
}

//目前未实现功能
void MprpcController::StartCancel(){}
bool MprpcController::IsCanceled() const{return false;}
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback){}
```

在mprpcchannel.cc中使用时，如果有错误就可以把错误记录到controller中

在callfriendsservice.cc中，如果controller中记录有错误信息，那么就说明rpc远程调用失败；如果没有错误信息说明执行成功

## 项目中添加日志模块

日志中记录框架运行时产生的信息

因为日志是记录在磁盘上，所以如果直接把信息写入日志就相当于磁盘I/O操作，会很慢

采用**日志缓冲区**（可以使用消息队列中间件kafka），把日志内容写到日志缓冲区队列上（这是在内存中进行，很快）；**另开辟一个线程**，该线程负责把队列中内容写到磁盘上

> 需要考虑的问题

1、由于Mprpc框架中服务器是epoll+多线程的，所以会出现多个线程同时写队列日志的情况，因此要把队列设置为**线程安全的**；

2、如果单纯采用互斥锁的话，写入queue需要抢锁，从queue读日志写入log也需要抢锁，**如果queue为空**，那么写日志线程抢锁就没有意义，还会拖慢写queue线程的效率；所以引入了**线程间通信**

![image-20230527170639548](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230527170639548.png)

> 首先创建日志类logger

因为日志系统只需要一个，所以采用单例模式，日志系统中需要有一个队列缓冲区，能够往队列缓冲区写内容，同时还要创建一个线程用来把缓冲区内容写到磁盘上

```c++
//logger.h
enum LogLevel{
    INFO,//普通信息
    ERROR,//错误信息
};
//Mprpc框架提供的日志系统
class Logger{
public:
    // 获取日志的单例
    static Logger& GetInstance();
    // 设置日志级别
    void SetLogLevel(LogLevel level);
    // 写日志
    void Log(std::string msg);
private:
    int m_loglevel; //记录日志级别
    LockQueue<std::string> m_lckQue; //日志缓冲队列

    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&& ) = delete;
};

//定义宏
#define LOG_INFO(logmsgformat, ...) \
    do \
    {  \
        Logger& logger = Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char c[1024] = {0}; \
        snprintf(c,1024,logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while({0});

#define LOG_ERR(logmsgformat, ...) \
    do \
    {  \
        Logger& logger = Logger::GetInstance(); \
        logger.SetLogLevel(ERROR); \
        char c[1024] = {0}; \
        snprintf(c,1024,logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while({0});
```

```c++
//logger.cc
// 获取日志的单例
Logger& Logger::GetInstance(){
    static Logger logger;
    return logger;
}

Logger::Logger(){
    // 启动专门的写日志线程，将缓冲区内容写到磁盘上
    std::thread writeLogTask([&](){
        for(;;){
            // 获取当前的日期，然后取日志信息写入相应的日志文件中
            time_t now = time(nullptr);
            tm* nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name,"%d-%d-%d-log.txt",nowtm->tm_year + 1900, nowtm->tm_mon+1,nowtm->tm_mday);

            FILE* pf = fopen(file_name,"a+");
            if( pf == nullptr){
                std::cout << "logger file: "<< file_name << "open error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lckQue.Pop();

            char time_buf[128] = {0};
            sprintf(time_buf, "%d:%d:%d => ", nowtm->tm_hour , nowtm->tm_min, nowtm->tm_sec);
            msg.insert(0,time_buf);

            fputs(msg.c_str(),pf);
            fclose(pf);
        }
    });
    //设置分离线程
    writeLogTask.detach();


}
// 设置日志级别
void Logger::SetLogLevel(LogLevel level){
    m_loglevel = level;
}
// 写日志，把日志信息写入lockqueue缓冲区当中，给外部调用的，只是写入到内存中的缓冲区上
void Logger::Log(std::string msg){
    m_lckQue.push(msg);
}
```

> 缓冲队列类的实现

在日志系统类中我们发现有一个LockQueue类，需要实现一下，借助于Queue类

注意：需要学习一下lock_guard这个类似智能指针的锁

因为缓冲区是共享的，所以需要加锁操作；

通过条件变量来进行线程通信：如果队列为空则写文件线程wait，队列不为空则通知解除wait

```c++

//异步写日志的日志队列
template<typename T>
class LockQueue{
public:
    // 多个工作线程都会写日志queue
    void push(const T& data){
        //这个锁在遇到}会自动释放
        std::lock_guard<std::mutex> lock(m_mutex);
        //拿到锁后才进行操作
        m_queue.push(data);
        //条件变量通知
        m_condvar.notify_one();
    }

    // 一个线程读日志queue，写日志文件
    T Pop(){
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty()){
            //日志队列为空 线程进入wait状态 释放当前持有的锁
            m_condvar.wait(lock);
        }

        T data = m_queue.front();
        m_queue.pop();
        return data;
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
};
```

## zookeeper分布式协调服务

[ZooKeeper原理及介绍 - 鹿泉 - 博客园 (cnblogs.com)](https://www.cnblogs.com/xinyonghu/p/11031729.html)

zookeeper可以理解为一个类似linux的结构，它的数据存储在znode上，znode就类似linux中的文件，znode下可以有znode节点。我们rpc服务就可以作为一个znode，然后rpc服务下的方法作为rpc服务znode下的子znode

eg: /UserService/Login、/UserService/Register

如果想要查询哪个方法就去找服务znode下对应的方法znode中保存的ip地址和port



我们希望通过zk解决的问题：

1. 每一个服务器和客户端都要存储一个配置文件，如果请求服务需要读取配置文件内容找到服务器。不再通过配置文件记录服务节点的ip和port，因为这是静态的，如果新增服务必须修改配置文件。这是一个紧耦合的结构，修改配置文件就要修改所有的配置文件

2. 通过zk作为中间层，可以动态更新服务节点的ip和port；当需要查找UserServiceRpc的Login方法时，会把这拼成一个路径，到zookeeper上按照路径查找到Login节点，Login节点下存放着该方法所在服务器的ip地址和端口号

![image-20230529131940435](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230529131940435.png)

服务节点与zookeeper之间通过建立session会话，要求服务节点发送**心跳消息**来判断该服务是否可用

+ 心跳消息就是，我有一个心跳计数，每隔一秒增加一，增加到5我就认为你断开连接；你发送心跳消息，我的心跳计数就会减一。
+ 永久性节点和临时性节点
  + 永久性节点断开连接，zookeeper不删除这个节点
  + 临时性节点断开连接，zookeeper会删除这个节点

>zk的数据是怎么组织的？ znode节点

![image-20230528190412461](../../../../../AppData/Roaming/Typora/typora-user-images/image-20230528190412461.png)

zk的数据存储在节点上，每个节点有名字和数据

常用命令：ls、get、create、set、delete

> zk的watcher机制

客户端对zookeeper上某个节点放置一个watcher，用来监听该节点的变化

如果该节点有变化，则会通过watcher来告诉客户端变化发生

> zk的使用

先到zookeeper下的bin中``./zkServer.sh start``，现在打开了zk的服务器；

如果我们想查看zk服务器上的内容，需要运行``./zkCli.sh``，然后使用ls、get等命令获得其中内容
