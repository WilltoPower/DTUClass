/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtu101slave.cpp
  *Description: 
    用于实现dtu间隔单元的101服务
  *History: 
    1, 创建, wangjs, 2021-7-9
**********************************************************************************/
#include "dtu101slave.h"
#include <dtulog.h>
#include "dturulesasdu.h"
#include "dturuleshandle.h"
#include <dtutask_iec.h>
#include <dtucmdcode.h>
#include <dtuprotocol.h>
#include <dtusystemconfig.h>
#include "dtutask_dsp.h"

#include "dtuunitcommon.h"
#include "dtuconnect.h"

#include <ctime>
#include <lib60870/hal_time.h>

using namespace DTU;
using namespace DTUCFG;

// 数据处理
bool D101Slave::dtu_asduHandler(IMasterConnection connection, CS101_ASDU asdu)
{
   auto type = CS101_ASDU_getTypeID(asdu);
    DTULOG(DTU_INFO, (char *)"主站A 接收到101请求:%u", type);

    dtuUnitRPCTranser trans;
    trans.setCallback(std::bind(&D101Slave::send_asdu, this, std::placeholders::_1, std::placeholders::_2), connection);
    trans.setAlParamer(this->get_applayer_param());

    RemoteCtrlInfo info;
    info.cmdFrom = RC_CMD_101;
    info.conn = (int)connection;
    info.delay = 45;

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
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_TIME_CAPTURE);
            break;
        }
        default:{
            trans.reciver(asdu, CS101_ASDU_getCA(asdu), BAYUC_CMD_UNKNOW_TYPEID);
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
    }
    return true;
}

// 链路状态改变
void D101Slave::dtu_linkLayerStateChanged(int address, LinkLayerState state)
{
    char timeBuffer[32];
    time_t tt;
    struct tm *ttime;
    tt = Hal_getTimeInMs() / 1000;
    ttime = localtime(&tt);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ttime);
    std::string timePre(timeBuffer);

    std::string log = "[" + timePre + "] Link layer state: ";
    switch(state)
    {
        case LL_STATE_IDLE: {
            log = log + "IDLE";
        };break;
        case LL_STATE_ERROR: {
            log = log + "ERROR";
        };break;
        case LL_STATE_BUSY: {
            log = log + "BUSY";
        };break;
        case LL_STATE_AVAILABLE: {
            log = log + "AVAILABLE";
        };break;
    }
    // 发送消息
    connectionMessage(CSLOG,true,101,log.size(),nullptr,log);

    if(state == LL_STATE_ERROR) {
        // 取消所有预设
        dsptask_execute_cancelrmc();
        DTUConnectTest::instance()->set_cs101_state(false);
    }
    else if (state == LL_STATE_AVAILABLE) {
        DTUConnectTest::instance()->set_cs101_state(true);
    }

    // printf("Link layer state: ");
    // switch (state) {
    // case LL_STATE_IDLE:
    //     printf("IDLE\n");
    //     break;
    // case LL_STATE_ERROR:
    //     printf("ERROR\n");
    //     break;
    // case LL_STATE_BUSY:
    //     printf("BUSY\n");
    //     break;
    // case LL_STATE_AVAILABLE:
    //     printf("AVAILABLE\n");
    //     break;
    // }
}

void D101Slave::dtu_rawMessageHandler(uint8_t* msg, int msgSize, bool sent)
{
    connectionMessage(CSMSG,sent,101,msgSize,msg,std::string(""));

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
bool D101Slave::dtu_clockSyncHandler (IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    // 下发时间
    ClockSynchronization(connection,asdu,newTime);
    return true;
}

// 总召唤
bool D101Slave::dtu_interrogationHandler(IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    // 只相应主站召唤
    if (qoi == 20) {
        DTULOG(DTU_INFO, (char *)"主站A 101总召唤");
        // 发送总召确认
        CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
        IMasterConnection_sendACT_CON(connection, asdu, false);

        dtuUnitRPCTranser trans;
        trans.setCallback(std::bind(&D101Slave::send_asdu, this, std::placeholders::_1, std::placeholders::_2), connection);
        trans.IMasterConnect(asdu, dtuUnitRPCTranser::CS101);

        IMasterConnection_sendACT_TERM(connection, asdu);

        return true;
    }
    else
    {
        DTULOG(DTU_WARN,"101未知的QOI=[%u]",qoi);
    }
    return IMasterConnection_sendACT_CON(connection, asdu, true);
}

// 发送ASDU数据
void D101Slave::send_asdu(CS101_ASDU asdu, IMasterConnection con)
{
    if (asdu){
        if (con){
            IMasterConnection_sendASDU(con, asdu);
        }
        else{
            CS101_Slave_enqueueUserDataClass2(_slave, asdu);
        }
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 静态回调处理函数
static bool
clockSyncHandler (void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    if (parameter)
    {
        return ((D101Slave*)parameter)->dtu_clockSyncHandler(connection, asdu, newTime);
    }
    return false;
}

/* Callback handler that is called when an interrogation command is received */
static bool
interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    if (parameter)
    {
        return ((D101Slave*)parameter)->dtu_interrogationHandler(connection, asdu, qoi);
    }
    return false;
}

static bool
asduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu)
{
    if (parameter)
    {
        return ((D101Slave*)parameter)->dtu_asduHandler(connection, asdu);
    }
    return false;
}

static void
resetCUHandler(void* parameter)
{
    CS101_Slave_flushQueues((CS101_Slave) parameter);
}

static void
linkLayerStateChanged(void* parameter, int address, LinkLayerState state)
{
    if (parameter)
    {
        ((D101Slave*)parameter)->dtu_linkLayerStateChanged(address, state);
    }
}

/* Callback handler to log sent or received messages (optional) */
static void
rawMessageHandler (void* parameter, uint8_t* msg, int msgSize, bool sent)
{
    if (parameter)
    {
        ((D101Slave*)parameter)->dtu_rawMessageHandler(msg, msgSize, sent);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 安装回调处理函数
static bool install_hander(CS101_Slave slave, void *param) 
{
    if (!slave || !param) {
        return false;
    }
    
    /* set the callback handler for the clock synchronization command */
    CS101_Slave_setClockSyncHandler(slave, clockSyncHandler, param);

    /* set the callback handler for the interrogation command */
    CS101_Slave_setInterrogationHandler(slave, interrogationHandler, param);

    /* set handler for other message types */
    CS101_Slave_setASDUHandler(slave, asduHandler, param);

    /* set handler for reset CU (reset communication unit) message */
    CS101_Slave_setResetCUHandler(slave, resetCUHandler, (void*) slave);

    /* set timeout for detecting connection loss */
    CS101_Slave_setIdleTimeout(slave, 5000);

    /* set handler for link layer state changes */
    CS101_Slave_setLinkLayerStateChanged(slave, linkLayerStateChanged, param);  

    /* uncomment to log messages */
    CS101_Slave_setRawMessageHandler(slave, rawMessageHandler, param);
}

void D101Slave::dtu101_init_slave()
{
    // 创建串口
    // std::string serialPort = "/dev/" + DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.name;
    std::string serialPort = "/dev/ttyS6";
    _port = SerialPort_create(serialPort.c_str(), 
                                        DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.baudrate,
                                        DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.databits,
                                        DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.pairty,
                                        DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.stopbits);

    // 创建101 slave
    if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.mode == LINK_LAYER_UNBALANCED)
    {
        // 非平衡模式
        DTULOG(DTU_INFO,"主站A 101规约:非平衡模式");
        _slave = CS101_Slave_create(_port, NULL, NULL, IEC60870_LINK_LAYER_UNBALANCED);
    }
    else
    {
        // 平衡模式
        DTULOG(DTU_INFO,"主站A 101规约:平衡模式");
        _slave = CS101_Slave_create(_port, NULL, NULL, IEC60870_LINK_LAYER_BALANCED);
        CS101_Slave_setDIR(_slave, true);
    }

    // 本地链路地址
    DTULOG(DTU_INFO, "主站A 本地链路地址[%d]" , DSYSCFG::instance().linkAddr);
    CS101_Slave_setLinkLayerAddress(_slave, DSYSCFG::instance().linkAddr);
    // 远端链路地址 (主站链路地址)
    CS101_Slave_setLinkLayerAddressOtherStation(_slave, DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.otheraddr);

    // 配置AppLayer参数
    CS101_AppLayerParameters alParameters = CS101_Slave_getAppLayerParameters(_slave);
    alParameters->sizeOfCA = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofCA;
    alParameters->sizeOfIOA = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
    alParameters->sizeOfCOT = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofCOT;

    // 配置链路参数
    LinkLayerParameters llParameters = CS101_Slave_getLinkLayerParameters(_slave);
    llParameters->timeoutForAck = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForACK;
    llParameters->addressLength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.LinkAddrLength;
    llParameters->timeoutRepeat = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForRepeat;
    llParameters->useSingleCharACK = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.SingalCharACK;

    // 安装各回调函数
    install_hander(_slave, this);

    SerialPort_open(_port);

    // 101初始化标志位
    is_101_init = true;
}

void D101Slave::dtu101_run_slave()
{
    //
    _runthread = std::make_unique<std::thread>([&](){
        while(!_bStop){
            CS101_Slave_run(_slave);
            // 
            // to do other thing
            //
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void D101Slave::dtu101_stop_slave()
{
    if (!_bStop && _runthread){
        _bStop = true;
        _runthread->join();
        CS101_Slave_destroy(_slave);
        SerialPort_close(_port);
        SerialPort_destroy(_port);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 获取AP参数
CS101_AppLayerParameters D101Slave::get_applayer_param()
{
    return CS101_Slave_getAppLayerParameters(_slave);
}

// 读取地址
int32_t D101Slave::get_address()
{
    return DSYSCFG::instance().ASDU();
}

void D101Slave::notify_master(CS101_ASDU asdu, bool autodestroy)
{
    // asdu如果不为空则发送,否则不发送
    if (asdu == nullptr)
        return;

    if (_slave)
    {
        DTULOG(DTU_INFO, "101 A主站上送");
        CS101_Slave_enqueueUserDataClass2(_slave, asdu);
    }
    if (asdu != nullptr && autodestroy){
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
}