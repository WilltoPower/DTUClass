/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturpcmanager.h
  *Description: 
    RPC连接客户端管理
  *History: 
    1, 创建, wangjs, 2021-8-4
    2, 修改了对RPC客户端的管理, 
    加入了唯一识别ID,确保一个客户端只能连接一次, wangjs, 2021-8-9
**********************************************************************************/
#ifndef _DTU_PRC_MANAGER_H
#define _DTU_RPC_MANAGER_H

#include <dtuprotocol.h>
#include "dturpcserver.h"
#include <mutex>
#include <map>
#include <dtusystemconfig.h>

namespace DTU
{
    class DTURPCMgr
    {
    private:
        DTURPCMgr(){}
        DTURPCMgr(const DTURPCMgr&) = delete;
    public:
        static DTURPCMgr& instance(){
            static DTURPCMgr mgr;
            return mgr;
        }
    public:
        template<typename T>
        void registerfunc(const std::string& funcName, T func)
        {
            DTURPC::instance().registerfunc(funcName, func);
        }
        
        // 启动RPC服务
        bool start_dtu_rpc(uint16_t port);
        // 停止RPC服务
        void stop_dtu_rpc();

        // 数据发布到到配置工具
        void notify_dtu_client(const dtuprotocol& proto);    
        // 规约内容发布到到工具
        void notify_cs_client(uint8_t cmd,const dtuprotocol& proto);
        // GOOSE状态发布到工具
        void notify_goose_client(const dtuprotocol& proto);
    private:
        std::mutex _lock;
    };
};
#endif