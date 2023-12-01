/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtu104server.cpp
  *Description:
    用于实现dtu间隔单元的104服务
  *History:
    1, 创建, wangjs, 2021-7-1
    2, 加入间隔单元104参数读取, wangjs, 2021-7-8
    3, 修改了104的初始化函数参数, 由外部传入, wangjs, 2021-8-6
    4, 将回调函数从原来的callback文件移到此处, wangjs, 2021-8-6
    5, 实现总召唤命令, wangjs, 2021-8-24
**********************************************************************************/
#include "dtu104server.h"
#include "dturuleshandle.h"
#include "dturulesasdu.h"
#include <lib60870/hal_time.h>
#include <string.h>
#include <dtulog.h>
#include <fstream>
#include <dtustructs.h>
#include <dtusystemconfig.h>
#include <dtucmdcode.h>
#include <dtucommon.h>
#include <functional>
#include <dtusystemconfig.h>
#include "dtutask_dsp.h"

#include "dtuunitcommon.h"
#include "dtuconnect.h"
#include "dtustorage.h"

using namespace DTU;
using namespace DTUCFG;

// asdu接收
bool D104Server::dtu_asduHandler(IMasterConnection connection, CS101_ASDU asdu) {
    //
    auto type = CS101_ASDU_getTypeID(asdu);
    DTULOG(DTU_INFO, (char *)"接收到104请求:%u", type);

    dtuUnitRPCTranser trans;
    trans.setCallback(std::bind(&D104Server::send_asdu, this, std::placeholders::_1, std::placeholders::_2), connection);
    trans.setAlParamer(this->get_applayer_param());

    RemoteCtrlInfo info;
    info.cmdFrom = RC_CMD_104;
    info.conn = (int)connection;
    info.delay = 30;

    switch (type) {
        // 文件传输相关
        case F_FR_NA_2: {
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_FIEL_REQ, info);
            break;
        }
         // 切换定值区
        case C_SR_NA_1: {
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_CHANGE_GROUP, info);
            break;
        }
        // 读取当前区号
        case C_RR_NA_1: {
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_CURRENT_GROUP, info);
            break;
        }
        // 读参数/定值
        case C_RS_NA_1:{
            // 读取定值
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_READ_PARAM, info);
            break;
        }
        // 写入定值相关
        case C_WS_NA_1:{
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_WRITE_PARAM, info);
            break;
        }
        // 遥控命令
        case C_SC_NA_1:
        case C_DC_NA_1:{
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_ROMATE_CTRL, info);
            break;
        }
        // 校时获取时间命令
        case C_CS_NA_1:{
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_TIME_CAPTURE, info);
            break;
        }
        case 108: {
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_READ_PARAM_B, info);
            break;
        }
        case 55: {
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_WRITE_PARAM_B, info);
            break;
        };
        // 未知的类型标识
        default:{
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_UNKNOW_TYPEID);
            break;
        }
    }
    return true;
}

// 104连接消息
bool D104Server::dtu_connectionRequestHandler(const char *ipAddress) 
{
    std::string log = "New connection request from " + std::string(ipAddress);
    connectionMessage(CSLOG,true,104,log.size(),nullptr,log);
    // printf("New connection request from %s\n", ipAddress);
    return true;
}

// 连接状态
void D104Server::dtu_connectionEventHandler(IMasterConnection con, CS104_PeerConnectionEvent event) 
{
    //uint32_t conno = (int)con;
    char conno[64] = {};
    sprintf(conno, "%p", con);
    std::string log = std::string(conno, 64) + ")";
    switch(event)
    {
        case CS104_CON_EVENT_CONNECTION_OPENED: {
            log = "Connection opened (" + log;
        };break;
        case CS104_CON_EVENT_CONNECTION_CLOSED: {
            log = "Connection closed (" + log;
        }break;
        case CS104_CON_EVENT_ACTIVATED: {
            log = "Connection activated (" + log;
        }break;
        case CS104_CON_EVENT_DEACTIVATED: {
            log = "Connection deactivated (" + log;
        }break;
    }
    // 发送消息
    connectionMessage(CSLOG,true,104,log.size(),nullptr,log);

    if (event == CS104_CON_EVENT_CONNECTION_OPENED) 
    {
        DTUConnectTest::instance()->set_cs104_state(true);
        _is104_connect = true;
        // printf("Connection opened (%p)\n", con);
    } else if (event == CS104_CON_EVENT_CONNECTION_CLOSED) {
        _is104_connect = false;
        IMasterConnection_close(con);
        DSTORE::instance().PreSettingFlag = false;
        DSTORE::instance().PreSettingFirstFlag = false;
        // 取消所有预设
        dsptask_execute_cancelrmc();
        DTUConnectTest::instance()->set_cs104_state(false);
        // printf("Connection closed (%p)\n", con);
    } else if (event == CS104_CON_EVENT_ACTIVATED) {
        // printf("Connection activated (%p)\n", con);
    } else if (event == CS104_CON_EVENT_DEACTIVATED) {
        // printf("Connection deactivated (%p)\n", con);
    }
}

void D104Server::dtu_rawMessageHandler(IMasterConnection con, uint8_t* msg, int msgSize, bool sent)
{
    connectionMessage(CSMSG,sent,104,msgSize,msg,std::string(""));

    // if (sent)
    //     printf("SEND: ");
    // else
    //     printf("RCVD: ");

    // int i;
    // for (i = 0; i < msgSize; i++) 
    // {
    //     printf("%02x ", msg[i]);
    // }
    // printf("\n");
}

// 时钟同步
bool D104Server::dtu_clockSyncHandler(IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime) 
{
    // 下发时间
    ClockSynchronization(connection,asdu,newTime);
    return true;
}

// 总召唤
bool D104Server::dtu_interrogationHandler(IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi) 
{
    // 只相应主站召唤
    if (qoi == 20) 
    {
        DTULOG(DTU_INFO, (char *)"104总召唤");
        // 发送总召确认
        IMasterConnection_sendACT_CON(connection, asdu, false);

        dtuUnitRPCTranser trans;
        trans.setCallback(std::bind(&D104Server::send_asdu, this, std::placeholders::_1, std::placeholders::_2), connection);
        trans.IMasterConnect(asdu, dtuUnitRPCTranser::CS104);

        IMasterConnection_sendACT_TERM(connection, asdu);

        return true;
    }
    else
    {
        DTULOG(DTU_WARN,"104未知的QOI=[%u]",qoi);
    }
    return IMasterConnection_sendACT_CON(connection, asdu, true);
}

// 发送ASDU
void D104Server::send_asdu(CS101_ASDU asdu, IMasterConnection con)
{
    if (asdu) {

        if (con) {
            if (F_FR_NA_2 == CS101_ASDU_getTypeID(asdu)) {
                // 在线才传输文件
                if (_is104_connect)
                    IMasterConnection_sendASDU(con, asdu);
            }
            else
                IMasterConnection_sendASDU(con, asdu);
        }
        else{
            CS104_Slave_enqueueASDU(_pSlave, asdu);
        }

        // 回调函数提供的ASDU不需要自动销毁
        // 库会在函数调用结束后自动的销毁
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 静态回调处理函数
// 时钟同步
static bool clockSyncHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime) 
{
    if (!parameter) 
    {
        return false;
    }
    return ((DTU::D104Server *)parameter)->dtu_clockSyncHandler(connection, asdu, newTime);
}

// 总召唤
static bool interrogationHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi) 
{
    if (!parameter) 
    {
        return false;
    }
    return ((DTU::D104Server *)parameter)->dtu_interrogationHandler(connection, asdu, qoi);
}

// ASDU数据处理
static bool asduHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu) 
{
    if (!parameter) 
    {
        return false;
    }
    ((DTU::D104Server *)parameter)->dtu_asduHandler(connection, asdu);
    return true;
}

// 104连接消息
static bool connectionRequestHandler(void *parameter, const char *ipAddress) 
{
    if (!parameter) 
    {
        return false;
    }
    return ((DTU::D104Server *)parameter)->dtu_connectionRequestHandler(ipAddress);
}

// 连接状态
static void connectionEventHandler(void *parameter, IMasterConnection con, CS104_PeerConnectionEvent event) 
{
    if (parameter) 
    {
        ((DTU::D104Server *)parameter)->dtu_connectionEventHandler(con, event);
    }
}

// 收发消息
static void rawMessageHandler(void *parameter, IMasterConnection conneciton, uint8_t *msg, int msgSize, bool sent) 
{
    if (parameter)
    {
        ((DTU::D104Server *)parameter)->dtu_rawMessageHandler(conneciton, msg, msgSize, sent);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 安装回调处理函数
static bool install_hander(CS104_Slave slave, void *param) 
{
    if (!slave || !param) {
        return false;
    }
    /* set the callback handler for the clock synchronization command */
    CS104_Slave_setClockSyncHandler(slave, clockSyncHandler, param);

    /* set the callback handler for the interrogation command */
    CS104_Slave_setInterrogationHandler(slave, interrogationHandler, param);

    /* set handler for other message types */
    CS104_Slave_setASDUHandler(slave, asduHandler, param);

    /* set handler to handle connection requests (optional) */
    CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, param);

    /* set handler to track connection events (optional) */
    CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, param);

    /* uncomment to log messages */
    CS104_Slave_setRawMessageHandler(slave, rawMessageHandler, param);

    return true;
}

bool D104Server::dtu104_init_server() {

    // 如果文件接收不到需要更改后面高优先级队列长度
    /*
    结构体如下
    struct sMessageQueueEntryInfo {
        uint64_t entryId;
        unsigned int entryState : 2;
        unsigned int size : 8;
    };
    内存占用为结构体长度(16字节,注意字节对齐)+256字节 * 第二个参数
    目前载荷能力为2MB
    内存占用为2MB
    */
    
    _pSlave = CS104_Slave_create(10240*2, 10240*2);

    // 绑定IP 0.0.0.0绑定到所有端口
    CS104_Slave_setLocalAddress(_pSlave, DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.ip.c_str());
    // 绑定端口
    CS104_Slave_setLocalPort(_pSlave, DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.port);

    DTULOG(DTU_INFO, "104服务已在[%s][%d]启动", DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.ip.c_str(), 
                                    DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.port);

    // 安装回调函数
    install_hander(_pSlave, this);

    /* get the connection parameters - we need them to create correct ASDUs -
     * you can also modify the parameters here when default parameters are not to be used */
    // 配置APCI参数
    // CS104_APCIParameters apciParams = CS104_Slave_getConnectionParameters(_pSlave);

    // 配置AppLayer参数
    CS101_AppLayerParameters alParam =  CS104_Slave_getAppLayerParameters(_pSlave);
    alParam->sizeOfCOT = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofCOT;
    alParam->sizeOfCA = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofCA;
    alParam->sizeOfIOA = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;

    // 配置模式，允许多个客户端连接，每个客户端单独一个队列
    CS104_Slave_setServerMode(_pSlave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP);

    // 启动104 从站服务
    CS104_Slave_start(_pSlave);
    if (CS104_Slave_isRunning(_pSlave) == false) {
        DTULOG(DTU_ERROR, (char *)"启动104从站服务失败...");
        return false;
    }
    // 发送初始化完成
    CS101_ASDU newAsdu = 
        CS101_ASDU_create(alParam, false, CS101_COT_INITIALIZED, 0, DSYSCFG::instance().ASDU(), false, false);

    InformationObject io = (InformationObject)EndOfInitialization_create(NULL, 0);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    CS104_Slave_enqueueASDU(_pSlave, newAsdu);

    CS101_ASDU_destroy(newAsdu);
    
    is_104_init = true;

    return true;
}

void D104Server::dtu104_run_server() {
    _threadptr = std::make_unique<std::thread>([&]() {

        int nCount = 1;
        while (_run) {
            // to do
            // test_run(nCount);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            //nCount++;
        }
    });
}

void D104Server::dtu104_stop_server() {
    if (_run && _threadptr) {
        _run = false;
        // 等待任务完成
        _threadptr->join();
        // 关闭从站服务
        CS104_Slave_stop(_pSlave);
        CS104_Slave_destroy(_pSlave);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CS101_AppLayerParameters D104Server::get_applayer_param()
{
    return CS104_Slave_getAppLayerParameters(_pSlave);
}

int32_t D104Server::get_address()
{
    return DSYSCFG::instance().ASDU();
}

void D104Server::notify_master(CS101_ASDU asdu)
{
    // asdu如果不为空则发送,否则不发送
    if (asdu == nullptr)
        return;

    if(_pSlave) {
        CS104_Slave_enqueueASDU(_pSlave, asdu);
    }
    if (asdu) {
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
}