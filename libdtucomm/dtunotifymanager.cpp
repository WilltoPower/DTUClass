/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtunotifymanager.cpp
  *Description: 
    对外通知管理,包括通知主站，配置工具，MCU
  *History: 
    1, 创建, wangjs, 2021-8-10
**********************************************************************************/
#include "dtunotifymanager.h"
#include "dturpcmanager.h"
#include "dtuserial.h"
#include <dturulesnotify.h>
#include <dtusystemconfig.h>
#include <dtulog.h>
#include <dtusystemconfig.h>
#include <future>

using namespace DTU;
using namespace DTUCFG;

// 通知规约工具
void DTUNotifyMgr::notify_cstool(uint8_t cmd,const dtuprotocol& data)
{
    DTURPCMgr::instance().notify_cs_client(cmd,data);
}

// 通知配置工具
void DTUNotifyMgr::notify_dtutools(const dtuprotocol& data)
{
    DTURPCMgr::instance().notify_dtu_client(data);
}

// 通知MCU
void DTUNotifyMgr::notify_mcu(const dtuprotocol& data)
{
    dtuserial::instance().notify_serial(data);
}

// 通知公共单元
void DTUNotifyMgr::notify_public(const dtuprotocol& data)
{
    // 屏蔽公共单元上送
    return;

    // 如果当前单元不是间隔单元直接返回
    if(DSYSCFG::instance().isPublic())
        return;

    try
    {
        std::future<void> asyncTask = std::async(std::launch::async, [](const dtuprotocol pdata) {
            auto PCFG = DSYSCFG::instance().GetPublicCFG();

            rest_rpc::rpc_client PublicSender(PCFG.ProtoRPC.ip, PCFG.ProtoRPC.port);
            PublicSender.set_connect_timeout(50);
            if(PublicSender.connect())
            {
                int ret = PublicSender.call<int>("rpc_bay_notify", pdata.package(), DSYSCFG::instance().devno);
            }
            else
                DTULOG(DTU_WARN,"公共单元未连接");
	    }, data);
    }
    catch(std::exception &e)
    {
        DTULOG(DTU_ERROR,"通知公共单元发生错误:%s",e.what());
    }

}

// 通知101
void DTUNotifyMgr::notify_101(const dtuprotocol& data)
{
    if(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use)
    {
        DRULESNotify::instance().notify(data, 101, false, DSYSCFG::instance().devno);
    }
}

// 通知104
void DTUNotifyMgr::notify_104(const dtuprotocol& data)
{
    if(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use)
    {
        DRULESNotify::instance().notify(data, 104, false, DSYSCFG::instance().devno);
    }
}