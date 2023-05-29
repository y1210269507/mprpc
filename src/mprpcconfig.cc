#include "mprpcconfig.h"
#include<iostream>
#include<string>
//如果遇到检测不出来mprpcconfig.h的错误，说明vscode抽风了，cmake一下就好

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