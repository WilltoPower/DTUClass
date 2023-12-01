#include "dturulesnotify.h"
#include "dturulesasdu.h"
#include "dtu104server.h"
#include "dtu101slave.h"
#include "dtu101slave_a.h"
#include <dtucmdcode.h>
#include "dtulog.h"
#include <dtudbmanager.h>
#include <dtusystemconfig.h>

#include "dtuhalasdu.h"

using namespace DTU;
using namespace DTUCFG;

void DRULESNotify::notify(const dtuprotocol& src, uint32_t type, bool isfrombay, int bca)
{
    switch(src._cmd){
        // 突发遥测上送
    case TX_PC_COS_DATA:
        notify_cos(src, type, isfrombay, bca);
        break;
    case TX_PC_SOE_INFO:
        notify_soe(src, type, isfrombay, bca);
        break;
    default: {
        DTULOG(DTU_ERROR,"未知的通知类型[0x%04X]",src._cmd);
    };break;
    }
}

void DRULESNotify::notify_cos(const dtuprotocol& src, uint32_t type, bool isfrombay, int bca)
{
    CS101_AppLayerParameters alParam = nullptr;
    int32_t ca = 0;
    if (type == 104)
    {
        if(!(D104Server::instance().is_104_init))
            return;
        alParam = D104Server::instance().get_applayer_param();
        ca = D104Server::instance().get_address();
    }
    else if (type == 101)
    {
        if(!(D101Slave::instance().is_101_init))
            return;
        alParam = D101Slave::instance().get_applayer_param();
        ca = D101Slave::instance().get_address();
    }
    else
    {
        DTULOG(DTU_ERROR,"notify_cos()错误的协议编号");
        return;
    }
    
    // if(isfrombay)
    //     ca = bca;

    MEASURED_TYPE ttype;

    if(type == 101) {
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType;
    }
    else {
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType;
    }

    CS101_ASDU asdu = dtuHALhandler::createCOS(ca, DSYSCFG::instance().ASDU(), alParam, src._data, type, ttype, true, isfrombay, bca);

    // 指针为空直接返回
    if (asdu == nullptr)
        return;

    if (type == 104)
    {
        // 发送
        D104Server::instance().notify_master(asdu);
    }
    else if (type == 101)
    {
        D101Slave::instance().notify_master(asdu, false);
        D101SlaveSecond::instance().notify_master(asdu);
    }
}

// 发送一条不带时标的软遥信 发送一条带时标的软遥信
void DRULESNotify::notify_soe(const dtuprotocol& src, uint32_t type, bool isfrombay, int bca)
{
    CS101_AppLayerParameters alParam = nullptr;
    int32_t ca = 0;
    if (type == 104)
    {
        if(!(D104Server::instance().is_104_init))
            return;
        alParam = D104Server::instance().get_applayer_param();
        ca = D104Server::instance().get_address();
    }
    else if (type == 101)
    {
        if(!(D101Slave::instance().is_101_init))
            return;
        alParam = D101Slave::instance().get_applayer_param();
        ca = D101Slave::instance().get_address();
    }
    else
    {
        DTULOG(DTU_ERROR,"notify_soe()错误的协议编号");
        return;
    }

    // if(isfrombay)
    //     ca = bca;

    TELEGRAM_TYPE ttype;
    TELEGRAM_TYPE addtype;

    if(type == 101) {
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
    }
    else {
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
    }

    if (ttype == TELE_DOUBLE_POINT)
        addtype = TELE_DOUBLE_POINT_CP56;
    else
        addtype = TELE_SINGLE_POINT_CP56;

    CS101_ASDU asdu = dtuHALhandler::createSOE(ca, DSYSCFG::instance().ASDU(), alParam, src._data, type, ttype, false, isfrombay, bca);
    CS101_ASDU asdu_CP56 = dtuHALhandler::createSOE(ca, DSYSCFG::instance().ASDU(), alParam, src._data, type, addtype, false, isfrombay, bca);

    // 指针为空直接返回
    if (asdu == nullptr || asdu_CP56 == nullptr)
        return;

    if (type == 104)
    {
        // 发送
        D104Server::instance().notify_master(asdu);
        D104Server::instance().notify_master(asdu_CP56);
    }
    else if (type == 101)
    {
        D101Slave::instance().notify_master(asdu, false);
        D101SlaveSecond::instance().notify_master(asdu);
        D101Slave::instance().notify_master(asdu_CP56, false);
        D101SlaveSecond::instance().notify_master(asdu_CP56);
    }
}