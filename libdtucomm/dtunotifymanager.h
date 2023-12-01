/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtunotifymanager.h
  *Description: 
    对外通知管理,包括通知主站，配置工具，MCU
  *History: 
    1, 创建, wangjs, 2021-8-10
    2, 加入对101/104的通知, wangjs, 2021-12-23
**********************************************************************************/
#ifndef _DTU_NOTIFY_MANAGER_H
#define _DTU_NOTIFY_MANAGER_H
#include "dtuprotocol.h"
namespace DTU
{
    class DTUNotifyMgr
    {
    public:
        static DTUNotifyMgr& instance(){
            static DTUNotifyMgr mgr;
            return mgr;
        }
    private:
        DTUNotifyMgr(){}
    public:
        // 通知规约工具
        void notify_cstool(uint8_t cmd,const dtuprotocol& data);
        // 通知配置工具
        void notify_dtutools(const dtuprotocol& data);
        // 通知MCU
        void notify_mcu(const dtuprotocol& data);
        // 通知公共单元
        void notify_public(const dtuprotocol& data);
        // 通知101
        void notify_101(const dtuprotocol& data);
        // 通知104
        void notify_104(const dtuprotocol& data);
    };
};
#endif