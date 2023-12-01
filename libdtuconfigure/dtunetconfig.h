#ifndef _NET_CFG_H
#define _NET_CFG_H

#include <vector>
#include <string>
#include <dtuprotocol.h>
#include <json/json.h>

//是否应用
#define NET_USE_Default 1

namespace DTUCFG
{
    class netconfigure
    {
        public:
            static netconfigure& instance() {
                static netconfigure cfg;
                return cfg;
            }
        public:
            void load(const std::string& file);
            int save_net(DTU::buffer context);
            //获取网络参数
            DTU::buffer get_net_param();
            //设置网络参数
            int set_net_param(const DTU::buffer& data, int net_use = NET_USE_Default);
            DTU::buffer get_net_param_from_file();
        private:
            int _NETnum = 0;//网卡数量
            Json::Value _net_info;
            // 是否使用 IP 网卡名 子网掩码 网关
            std::vector<std::tuple<uint16_t,std::string,std::string,std::string,std::string>> NetParam;
        private:
            netconfigure(){};

    };
}

#endif