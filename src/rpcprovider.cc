#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

/*
service_name => service描述器对象
                        => 描述器对象可以描述该服务的名字以及其中包含的方法
                        method_name => method方法描述器对象
                        
*/
// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
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

// 启动rpc服务节点，开始提供rpc远程网络调用服务
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

    // 把当前rpc节点要发布的服务全部注册到zk上，让rpc client可以从zk上发现服务
    // session timeout 3s    zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkClie;
    zkClie.Start();
    // service_name 为永久性节点，  method_name为临时性节点
    for(auto& sp: m_serviceMap){
        // /service_name 相当于创建了/FriendServiceRpc
        std::string service_path = "/" + sp.first;
        zkClie.Create(service_path.c_str(),nullptr,0);
        for(auto& mp:sp.second.m_methodMap){
            // /service_name/method_name   /UserServiceRpc/Login 存储当前rpc节点的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkClie.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }

    }
    std::cout<<"RpcProvider start service at ip:"<<ip<<" port:"<<port<<std::endl;
    // 启动网络服务
    server.start();

    //相当于epoll.wait()，等待epoll中连接上有事件发生
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn->connected()){
        // 和rpc client连接断开了
        conn->shutdown();
    }
}
/*
在框架内部，RpcProvider和RpcConsumer协商好通信用的protobuf消息类型，就是数据该以什么样形式传输
service_name  method_name  args，传服务名->根据服务名得到所有的method->传method名->调用指定的method  对于args，使用已有的message类型进行数据的序列化和反序列化；对于service_name和method_name重新创建message

新建立的message包含 service_name  method_name  args_size(用来处理TCP粘包问题)

16UserServiceLogin16zhang san123456

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
        LOG_INFO("rpc_header_str: %s parse error!", rpc_header_str)
        //std::cout<<"rpc_header_str: "<<rpc_header_str<<" parse error!"<<std::endl;
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

    //给下面method方法调用绑定一个发送的回调函数, 当框架处理完业务后，还需要把返回值发送给请求端，这需要网络通信功能以及把返回值序列化

    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr& ,google::protobuf::Message*>(this, &RpcProvider::SendRpcResponse, conn, response);
    //现在框架可以根据远程rpc请求，调用当前节点发布的rpc方法; 即调用UserService里的Login方法
    service->CallMethod(method, nullptr,request,response, done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response){
    std::string response_str;
    if(response->SerializeToString(&response_str)){
        //序列化成功后 通过网络把rpc方法执行的结果发送回rpc请求方
        conn->send(response_str);
    }
    else{
        std::cout<< "Serialize response_str error!"<<std::endl;
    }
    conn->shutdown();// 模拟http短连接服务, 由rpcprovider主动断开连接
}