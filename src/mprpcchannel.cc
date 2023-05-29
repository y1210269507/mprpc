#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include "zookeeperutil.h"

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
        
        controller->SetFailed("Serialize request error!");
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
        controller->SetFailed("Serialize rpc header error!");
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
        
        char errtxt[512] = {0};
        sprintf(errtxt,"create socket error !errno:%d ",errno);
        controller->SetFailed(errtxt);
        return;
    }
    // 不再使用配置文件读取rpc服务的ip以及port
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    // 现在想要获取rpc服务器的ip和port要根据/服务名/方法名去zk上查询
    ZkClient zkCli;
    zkCli.Start();
    // method_path = /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetDate(method_path.c_str());
    // host_data = 127.0.0.1:8000
    if(host_data == ""){
        controller->SetFailed(method_path + " is not exist!");
        return;
    }

    int idx = host_data.find(":");
    if(idx == -1){
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    
    std::string ip = host_data.substr(0,idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        
        char errtxt[512] = {0};
        sprintf(errtxt,"connect error !errno:%d ",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }
    
    //发送rpc请求
    if(send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0) == -1){
        
        char errtxt[512] = {0};
        sprintf(errtxt,"send error !errno:%d ",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }



    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size =0;
    if((recv_size = recv(clientfd,recv_buf,1024,0) ) == -1){
        
        char errtxt[512] = {0};
        sprintf(errtxt,"recv error !errno:%d ",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    //将响应值写入response，用户端就能感受到响应值了
    //反序列化rpc调用的响应数据
    //std::string response_str(recv_buf,0,recv_size); // 出现问题，recv_buf遇到\0后面数据就丢弃了，导致反序列化失败
    if(!response->ParseFromArray(recv_buf,recv_size)){
        
        char errtxt[512] = {0};
        sprintf(errtxt,"response error ! response_str:%s ",recv_buf);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    close(clientfd);
}