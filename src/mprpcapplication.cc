#include "mprpcapplication.h"
#include<iostream>
#include<unistd.h>
#include<string>

void ShowArgsHelp(){
    std::cout<<"format: command -i <configfile>"<<std::endl;
}

MprpcConfig MprpcApplication::m_config; 
void MprpcApplication::Init(int argc, char** argv){
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

    // 开始加载配置文件了 rpcserver_lp=  rpcserver_port  zookeeper_ip=  zookepper_port=
    m_config.LoadConfigFile(config_file.c_str());
    // std::cout<<"rpcserverip: "<<m_config.Load("rpcserverip")<<std::endl;
    // std::cout<<"rpcserverport: "<<m_config.Load("rpcserverport")<<std::endl;
    // std::cout<<"zookeeperip: "<<m_config.Load("zookeeperip")<<std::endl;
    // std::cout<<"zookeeperport: "<<m_config.Load("zookeeperport")<<std::endl;

}
MprpcApplication& MprpcApplication:: GetInstance(){
    static MprpcApplication app;
    return app;
}
MprpcConfig& MprpcApplication::GetConfig(){
    return m_config;
}