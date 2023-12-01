#include "dtuhalasdu.h"

#include <time.h>
#include <stdio.h>

#include <lib60870/cs101_information_objects.h>

#include <dtutask_iec.h>
#include "dturulehandle.h"
#include "dtudbmanager.h"
#include "dtuioamap.h"
#include "dtutask_dsp.h"
#include "dtulog.h"
#include "dtusoemap.h"
#include "dtuelecyt.h"
#include "dtuparamconfig.h"
#include "dtustorage.h"

using namespace DTU;
using namespace DTUCFG;


// ASDU最大帧长度,104为254 101为249,这里是使用最小的以保证兼容性
#define MAX_ASDU_LENGTH 235

#define HYX_LENGTH 4
#define SYX_LENGTH 32

// 遥信数据长度
#define YX_LENGTH 1024

// 临时CA OA AlParam,本文件中为创建ASDU需要用到,但实际并不会使用ASDU,仅使用payload部分
#define TEMP_CA 1
#define TEMP_OA 0

#define FPRINT(val) printf("float [%f]\n", val)
#define UPRINT(val) printf("uint32 [%u]\n", val)

// 二次值转一次值
static void transECToYC(DTU::buffer& param, HIOA hioa, float& value, bool isCloseMutiple = false)
{
    // 直接返回
    return;

    std::map<HIOA, float> ptmap;

    if (param.size() != 96) {
        DTULOG(DTU_INFO, "公共定值长度有误,不进行一二次值变换,以二次值进行上送");
        return;
    }

    // printf("HIOA in [%f] ", value);
    uint32_t CTONE = param.get(0, 4).value<uint32_t>();    // CT额定一次值              0
    uint32_t CTTWO = param.get(4, 4).value<uint32_t>();    // CT额定二次值              1
    uint32_t ZCTONE = param.get(8, 4).value<uint32_t>();   // 零序CT额定一次值          2
    uint32_t ZCTTWO = param.get(12, 4).value<uint32_t>();  // 零序CT额定二次值          3
    float PPTONE = param.get(16, 4).value<float>();        // 电源侧PT额定一次值        4
    float PPTTWO = param.get(20, 4).value<float>();        // 电源侧PT额定二次值        5
    float FPTONE = param.get(24, 4).value<float>();        // 负荷侧PT额定一次值        6
    float FPTTWO = param.get(28, 4).value<float>();        // 负荷侧PT额定二次值        7
    float PZPTONE = param.get(32, 4).value<float>();       // 电源侧零序PT额定一次值    8
    float PZPTTWO = param.get(36, 4).value<float>();       // 电源侧零序PT额定二次值    9
    float FZPTONE = param.get(40, 4).value<float>();       // 负荷侧零序PT额定一次值    A
    float FZPTTWO = param.get(44, 4).value<float>();       // 负荷侧零序PT额定二次值    B

    int mutiple = 1000;
    if (isCloseMutiple) {
        mutiple = 1;
    }

    // UPRINT(CTONE);
    // UPRINT(CTTWO);
    // UPRINT(ZCTONE);
    // UPRINT(ZCTTWO);

    // FPRINT(PPTONE);
    // FPRINT(PPTTWO);
    // FPRINT(FPTONE);
    // FPRINT(FPTTWO);
    // FPRINT(PZPTONE);
    // FPRINT(PZPTTWO);
    // FPRINT(FZPTONE);
    // FPRINT(FZPTTWO);

    switch(hioa)
    {
        case 0x4001:value = value * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break;      // 母线侧A相电压
        case 0x4002:value = value * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break;      // 母线侧B相电压
        case 0x4003:value = value * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break;      // 母线侧C相电压
        case 0x4004:value = value * ((float)PZPTONE / (float)PZPTTWO) / (float)mutiple;break;    // 母线侧零序电压
        case 0x4005:value = value * ((float)CTONE / (float)CTTWO);break;        // 母线侧A相电流
        case 0x4006:value = value * ((float)CTONE / (float)CTTWO);break;        // 母线侧B相电流
        case 0x4007:value = value * ((float)CTONE / (float)CTTWO);break;        // 母线侧C相电流
        case 0x4008:value = value * ((float)ZCTONE / (float)ZCTTWO);break;      // 母线侧零序电流
        case 0x4011:value = value * ((float)FPTONE / (float)FPTTWO) / (float)mutiple;break;      // 线路侧A相电压
        case 0x4012:value = value * ((float)FPTONE / (float)FPTTWO) / (float)mutiple;break;      // 线路侧B相电压
        case 0x4013:value = value * ((float)FPTONE / (float)FPTTWO) / (float)mutiple;break;      // 线路侧C相电压
        case 0x4014:value = value * ((float)FZPTONE / (float)FZPTTWO) / (float)mutiple;break;    // 线路侧零序电压
        case 0x401F:value = value * ((float)CTONE / (float)CTTWO) * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break; // 有功功率
        case 0x4020:value = value * ((float)CTONE / (float)CTTWO) * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break; // 无功功率
        case 0x4021:value = value * ((float)CTONE / (float)CTTWO) * ((float)PPTONE / (float)PPTTWO);break; // 视在功率
        case 0x4027:value = value * ((float)FPTONE / (float)FZPTTWO) / (float)mutiple;break; // 线路侧电压Uab
        case 0x4028:value = value * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break;  // 母线侧电压Uab
        case 0x4029:value = value * ((float)PPTONE / (float)PPTTWO) / (float)mutiple;break;  // 母线侧电压Ubc
    }

    // printf("out [%f] \n", value);
}

// 注:一个本文件中因返回情况过多会使用goto语句,每个goto标签仅在一个函数中使用

std::map<int, int> dtuHALhandler::devStaticSave;

std::vector<DTU::buffer> dtuHALhandler::readParam(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo &rinfo)
{
    std::vector<DTU::buffer> functionResult;

    int elementNum = info.elementNumber;

    // 外部定值 使用时需要转换成内部定值
    std::vector<std::uint32_t> vecfix;
    std::vector<DTU::buffer> result;
    
    uint16_t group = 0;
    uint8_t tag = 0;

    // 解析ASDU载荷部分数据
    ParseReadParam(payload, group, tag, vecfix, rinfo);

    ////////////////////////////////////////////////////////////////////////////////////////
    // 是否在本机执行该函数
    bool CurDevExec = false;
    // 远方调度结果集合
    std::vector<DTU::buffer> remoteResult;
    // 远方调度(这里会判断是否在本机执行函数)
    buffer btemp;
    btemp.append((char*)&info, sizeof(info));
    btemp.append(payload);
    RemoteDispatchingDevice(CurDevExec, remoteResult, vecfix, "rpc_proto_read_param", btemp, rinfo);
    ////////////////////////////////////////////////////////////////////////////////////////
    if (CurDevExec || vecfix.size() == 0)
    {
        dtuRuleHandler::readParam(group, vecfix, result, rinfo);

        CS101_AppLayerParameters alParams = createAlParams();

        CS101_ASDU newAsdu = nullptr;
        int nCount = 0;
        int nCountsize = 12;//帧总长度计数
        for(auto& item : result)
        {
            if (!newAsdu) {
                newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA,false, false);
                CS101_ASDU_addPayload(newAsdu, (uint8_t*)&group, sizeof(uint16_t));
                CS101_ASDU_addPayload(newAsdu, &tag, sizeof(tag));
                nCount = 0;
            }

            if(nCountsize + item.size() >= MAX_ASDU_LENGTH)
            {
                /* 超过帧长 */
                if (nCount == result.size() - 1) {
                    tag = set_bit(tag, 1, false);
                } else {
                    tag = set_bit(tag, 1, true);
                }
                CS101_ASDU_getPayload(newAsdu)[2] = tag;
                CS101_ASDU_setNumberOfElements(newAsdu, nCount);
                CS101_ASDU_setTypeID(newAsdu,C_RS_NA_1);
                // 完成 添加到结果
                addpayload(functionResult, newAsdu);

                nCount = 0;
                nCountsize = 12;

                //将此次结果重新添加到新的帧
                newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA,false, false);

                CS101_ASDU_addPayload(newAsdu, (uint8_t*)&group, sizeof(uint16_t));
                tag = set_bit(tag, 1, false);
                CS101_ASDU_addPayload(newAsdu, &tag, sizeof(tag));
                CS101_ASDU_addPayload(newAsdu, (uint8_t*)item.data(), item.size());
                nCount++;
                nCountsize = nCountsize + item.size();
                //////////////////////////////
                continue;
            }
            else
            {/* 未超过帧长 添加元素 */
                CS101_ASDU_addPayload(newAsdu, (uint8_t*)item.data(), item.size());
                nCountsize = nCountsize + item.size();
            }
            nCount++;
        }
        
        // 所有循环结束,但还有一帧没有添加到结果集,将最后一帧添加到结果集
        if(newAsdu) {
            tag = set_bit(tag, 1, false);
            CS101_ASDU_getPayload(newAsdu)[2] = tag;
            CS101_ASDU_setNumberOfElements(newAsdu, nCount);
            CS101_ASDU_setTypeID(newAsdu,C_RS_NA_1);

            addpayload(functionResult, newAsdu);
        }
        
        // 销毁临时AlParams
        deleteAlParams(alParams);
    }

    // 如果远方调用有内容则追加到结果集
    if(remoteResult.size() > 0)
        functionResult.insert(functionResult.end(), remoteResult.begin(), remoteResult.end());

    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::writeParam(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo &rinfo)
{
    if (payload.size() == 3)
        return operateParam(info, payload, rinfo);    // 固化或者撤销
    else
        return presetParam(info, payload, rinfo);     // 携带数据,认为是预设
}

std::vector<DTU::buffer> dtuHALhandler::fileRequest(transerInfomation info, DTU::buffer &payload)
{
    std::vector<DTU::buffer> functionResult;
    CS101_AppLayerParameters alParams = createAlParams();
    bool haveQuestion = false;
    uint8_t opt = 0;
    CS101_ASDU newAsdu = nullptr;

    // 读取数据
    if (info.payloadSize == 0) {
        DTULOG(DTU_ERROR, (char *)"fileRequest() 文件请求目录为空");
        haveQuestion = true;
        goto LABEL_FILE_REQ_EXIT;
    }

    payload.dump(0, payload.size());

    payload.remove(0, 1);

    // 获取tag
    opt = payload.get(3, sizeof(uint8_t)).value<uint8_t>();  // payload[3]
    DTULOG(DTU_INFO, (char *)"文件处理请求[%u]",opt);
    switch (opt) 
    {
        // 主站远程读取文件目录
        case F_OPT_DIR: {
            functionResult = fileReqDirectory(info, payload, alParams);break;
        }
        // 主站远程从装置读文件激活
        case F_OPT_RFILE_ACT: {
            functionResult = fileReqContent(info, payload, alParams);break;
        }
        // 主站远程读文件完成确认
        case F_OPT_RFILE_CON: {
            fileTransOK(info, payload, alParams);break;
        }
        // 主站远程向装置写文件激活
        case F_OPT_WFILE_ACT: {
            functionResult = FileRecvComfirm(info, payload, alParams);break;
        }
        // 传文件内容
        case F_OPT_WFILE_DATA: {
            functionResult = FileRecvContent(info, payload, alParams);break;
        }
        default:
            DTULOG(DTU_WARN, (char *)"未知文件处理请求[%u]",opt);break;
    }

LABEL_FILE_REQ_EXIT:

    if(haveQuestion) {
        newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_TERMINATION, TEMP_CA, TEMP_OA, false, false);
        CS101_ASDU_setTypeID(newAsdu, static_cast<IEC60870_5_TypeID>(info.Ti));
        addpayload(functionResult, newAsdu);
    }

    deleteAlParams(alParams);
    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::changeCurrentGroup(transerInfomation info, DTU::buffer &payload)
{
    std::vector<DTU::buffer> functionResult;
    CS101_AppLayerParameters alParams = createAlParams();

    uint32_t group = payload.get(3, sizeof(uint16_t)).value<uint16_t>();   // *((uint16_t*)(payload+3));

    CS101_ASDU newAsdu = nullptr;
    if (DTU_SUCCESS != dtuRuleHandler::changeCurrentGroup(group)) {
       newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_UNKNOWN_IOA, TEMP_CA, TEMP_OA, false, false);
    }
    else {
       newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA, false, false);
    }

    // 添加fix(0)
    uint32_t fix = 0;
    int IOALength = 0;
    if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use) {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
    }
    else {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
    }
    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&fix, IOALength);
    uint16_t rgroup = DBManager::instance().GetCurrGroup();
    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&rgroup, sizeof(uint16_t));

    CS101_ASDU_setTypeID(newAsdu, C_SR_NA_1);
    addpayload(functionResult, newAsdu);

    deleteAlParams(alParams);
    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::readCurrentGroup(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo rinfo)
{
    std::vector<DTU::buffer> functionResult;
    CS101_AppLayerParameters alParams = createAlParams();

    DTU::buffer result;
    dtuRuleHandler::readCurrentGroup(result, rinfo);

    CS101_ASDU groupAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA, false, false);
    CS101_ASDU_addPayload(groupAsdu, (uint8_t*)result.data(), result.size());
    CS101_ASDU_setTypeID(groupAsdu, C_RR_NA_1);

    addpayload(functionResult, groupAsdu);

    deleteAlParams(alParams);
    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::remoteCtrl(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo rinfo)
{
    std::vector<DTU::buffer> functionResult;
    CS101_AppLayerParameters alParams = createAlParams();

    uint32_t fix = 0;
    uint16_t operate = 0;
    // 在分闸操作中是否执行
    bool exec = false;
    // 需要执行分闸还是合闸  1是合闸 2是分闸
    int fixopt = 1;
    uint32_t originfix = 0;
    int retCOT = ParseRemoteCtrl(info, payload, operate, fix,  originfix, fixopt, exec);

    auto Ti = info.Ti;

    std::vector<uint32_t> vecfix;
    vecfix.emplace_back(originfix);
    ////////////////////////////////////////////////////////////////////////////////////////
    // 是否在本机执行该函数
    bool CurDevExec = false;
    // 远方调度结果集合
    std::vector<DTU::buffer> remoteResult;
    // 远方调度(这里会判断是否在本机执行函数)
    buffer btemp;
    btemp.append((char*)&info, sizeof(info));
    btemp.append(payload);
    RemoteDispatchingDevice(CurDevExec, remoteResult, vecfix, "rpc_proto_romate_ctrl", btemp, rinfo);
    ////////////////////////////////////////////////////////////////////////////////////////

    if (CurDevExec)
    {
        DTULOG(DTU_INFO, "HIOA[0x%04X]本机执行遥控命令 操作FLAG[%02d] OPT[%02d]", fix, fixopt, operate);
        // 如果exec为false(即只有执行命令)且下发的是合闸命令才下发
        int execret = DTU_SUCCESS;
        if(fixopt == 1) {
            // 合闸
            execret = dtuRuleHandler::remoteControl(fix, operate, rinfo);
        }
        else if(fixopt == 2) {
            //分闸
            if(exec)
                execret = dtuRuleHandler::remoteControl(fix, operate, rinfo);
        }

        int payloadsize = info.payloadSize;
        uint8_t payloadBuff[8] = {};
        memcpy(payloadBuff, payload.const_data(), payload.size());
        
        if(execret != DTU_SUCCESS)
            retCOT = CS101_COT_UNKNOWN_COT; // 遥控失败返回

        // 返回发送确认命令
        CS101_ASDU newAsdu = nullptr;
        newAsdu = CS101_ASDU_create(alParams, false, static_cast<CS101_CauseOfTransmission>(retCOT), TEMP_CA, TEMP_OA, false, false);
        CS101_ASDU_addPayload(newAsdu, payloadBuff, payloadsize);
        CS101_ASDU_setTypeID(newAsdu, static_cast<IEC60870_5_TypeID>(Ti));
        CS101_ASDU_setNumberOfElements(newAsdu, 1);
        addpayload(functionResult, newAsdu);

        // 如果是执行命令 返回确认命令 且成功的时候
        if(operate == RC_CMD_EXE && execret == DTU_SUCCESS) {
            newAsdu = nullptr;
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_TERMINATION, TEMP_CA, TEMP_OA, false, false);
            CS101_ASDU_addPayload(newAsdu, payloadBuff, payloadsize);
            CS101_ASDU_setTypeID(newAsdu,static_cast<IEC60870_5_TypeID>(Ti));
            CS101_ASDU_setNumberOfElements(newAsdu, 1);
            addpayload(functionResult, newAsdu);
        }

        deleteAlParams(alParams);
    }

    // 如果远方调用有内容则追加到结果集
    if (remoteResult.size() > 0)
        functionResult.insert(functionResult.end(), remoteResult.begin(), remoteResult.end());

    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::timeCapture(transerInfomation info, DTU::buffer &payload)
{
    std::vector<DTU::buffer> functionalResult;
    int COT = info.COT;

    if(COT != CS101_COT_REQUEST)
        return functionalResult;

    // 获取时间
    DTU::buffer result;
    dtuRuleHandler::queryTime(result);
    uint64_t dtime = ParseCurrentTime(result);

    // 获取通信参数
    CS101_AppLayerParameters alParams = createAlParams();

    CS101_ASDU newAsdu = nullptr;
    newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_REQUEST, TEMP_CA, TEMP_OA, false, false);
    CS101_ASDU_setTypeID(newAsdu,C_CS_NA_1);

    CP56Time2a snewTime = new sCP56Time2a;
    CP56Time2a_setFromMsTimestamp(snewTime, dtime);
    uint16_t infofix = 0;
    // 添加信息体地址点表
    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&infofix, 3);
    // 添加时间点表
    CS101_ASDU_addPayload(newAsdu,(uint8_t*)snewTime,sizeof(sCP56Time2a));
    addpayload(functionalResult, newAsdu);

    delete snewTime;
    deleteAlParams(alParams);

    return functionalResult;
}

std::vector<DTU::buffer> dtuHALhandler::unknownTypeID(transerInfomation info, DTU::buffer &payload)
{
    std::vector<DTU::buffer> functionResult;
    CS101_AppLayerParameters alParams = createAlParams();
    CS101_ASDU newAsdu = nullptr;

    newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_UNKNOWN_TYPE_ID, TEMP_CA, TEMP_OA, false, false);
    CS101_ASDU_setTypeID(newAsdu, static_cast<IEC60870_5_TypeID>(info.Ti));
    addpayload(functionResult, newAsdu);

    deleteAlParams(alParams);
    return functionResult;
}

// curdev:是否在本机运行
std::vector<DTU::buffer> dtuHALhandler::IMasterConnect(int type, bool curdev)
{
    std::vector<DTU::buffer> functionResult;

    CS101_AppLayerParameters alParams = createAlParams();

    DTU::buffer HYXBuffer;  // 硬遥信缓存
    DTU::buffer SYXBuffer;  // 软遥信缓存
    DTU::buffer YCBuffer;   // 遥测缓存

    iectask_execute_query(PC_R_HYX_DATA, HYXBuffer);
    iectask_execute_query(PC_R_SYX_INFO, SYXBuffer);

    auto ASDU_yx = dtuHALhandler::createYX(TEMP_OA, TEMP_CA, alParams, HYXBuffer, SYXBuffer, static_cast<int>(type));
    for(const auto &item : ASDU_yx)
        addpayload(functionResult, item);

    // 上送遥测信息
    iectask_execute_query(PC_R_YC_DATA, YCBuffer);

    DTU::buffer publicParam;
    dsptask_execute_read(PC_R_PUB_FIX, publicParam);

    auto ASDU_yc = dtuHALhandler::createYC(TEMP_OA, TEMP_CA, alParams, YCBuffer, publicParam, static_cast<int>(type));
    for(const auto &item : ASDU_yc)
        addpayload(functionResult, item);

    // if (curdev) {
    //     // 上送校验值(遥测)
    //     CS101_ASDU newAsdu = dtuHALhandler::createCRC(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams);
    //     addpayload(functionResult, newAsdu);
    //     resultvalue.remove();
    // }

    deleteAlParams(alParams);
    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::NewIMasterConnect(int csfrom, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> functionResult;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // 前置准备工作
    // ASDU发送队列
    std::vector<CS101_ASDU> ASDUAttr;

    InformationObject io = nullptr;
    QualityDescriptor quality = IEC60870_QUALITY_INVALID;
    QualityDescriptor back = IEC60870_QUALITY_INVALID;//是否备用
    bool bvalue = false;
    float fvalue = 0.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // 获取规约配置

    // 是否采用压缩格式(默认 采用压缩格式) 获取遥信值类型 如果遇到未知来源,默认按照104的配置来处理
    bool isSequence = true;
    // 获取时间
    uint64_t currTime = get_current_mills();

    TELEGRAM_TYPE ttype;
    MEASURED_TYPE mtype;

    if(csfrom == 101) {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
        mtype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType;
    }
    else {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
        ttype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
        mtype = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType;
    }

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION, TEMP_OA, TEMP_CA, false, false);
    ASDUAttr.emplace_back(newAsdu);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // 获取所有遥信
    // auto YXGroup = GetYXValue();

    auto YXGroup = GetYXValueEx();

    // 如果是公共单元则进行远端读取
    if (DSYSCFG::instance().isPublic()) {
        for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
        {
            if (!item.second.use)
                continue;

            auto temp = dtuHALhandler::call<std::map<IOA, bool>>(item.second, "rpc_proto_get_yx");

            // 追加结果
            for (const auto& item : temp)
            {
                YXGroup.insert({item.first, item.second});
            }
        }
    }

    // 填充备用点
    for (const auto& item : IOAMap::instance()->findIOASpareWithType(IMAPT_YX))
    {
        YXGroup.insert({item, false});
    }

    for (const auto& item : YXGroup)
    {
        if (IOAMap::instance()->isIOASpare(item.first)) {
            quality = IEC60870_QUALITY_GOOD;
            back = IEC60870_QUALITY_INVALID;
            bvalue = false;
        }
        else {
            quality = IEC60870_QUALITY_GOOD;
            back = IEC60870_QUALITY_GOOD;
            bvalue = item.second;
        }
    
        printf("遥信 IOA[0x%04X] 状态[%s] 备用[%s]\n", item.first, bvalue ? "1" : "0", (back==IEC60870_QUALITY_INVALID) ? "Y" : "N");
        io = createTelegram(ttype, item.first, bvalue, currTime, quality);

        /*
            添加失败原因:
            1.发送帧超过长度
            2.压缩格式下信息体地址不连续
        */
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {

            DTULOG(DTU_WARN,"遥信IOA[0x%04X]添加数据失败,构造新io发送", item.first);
    
            newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION, TEMP_OA, TEMP_CA, false, false);

            ASDUAttr.emplace_back(newAsdu);

            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"遥信IOA[0x%04X]重构后添加数据失败", item.first);
            }

        }

        if (io) {
            InformationObject_destroy(io);
            io = nullptr;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // 获取所有遥测
    auto YCGroup = GetYCValue();

    // 如果是公共单元则进行远端读取
    if (DSYSCFG::instance().isPublic()) {
        for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
        {
            if (!item.second.use)
                continue;

            auto temp = dtuHALhandler::call<std::map<IOA, float>>(item.second, "rpc_proto_get_yc");
            // 追加结果
            for (const auto& item : temp)
            {
                YCGroup.insert({item.first, item.second});
            }
        }
    }

    // 填充备用点
    for (const auto& item : IOAMap::instance()->findIOASpareWithType(DSYSCFG::instance().devno, IMAPT_YC))
    {
        YCGroup.insert({item, false});
    }

    for (const auto& item : YCGroup)
    {

        if (IOAMap::instance()->isIOASpare(item.first)) {
            quality = IEC60870_QUALITY_GOOD;
            back = IEC60870_QUALITY_INVALID;
            fvalue = false;
        }
        else {
            quality = IEC60870_QUALITY_GOOD;
            back = IEC60870_QUALITY_GOOD;
            fvalue = item.second;
        }
    
        printf("遥测 IOA[0x%04X] 值[%.6f] 备用[%s]\n", item.first, fvalue, (back==IEC60870_QUALITY_INVALID) ? "Y" : "N");
        io = createMeasured(mtype, item.first, fvalue, currTime, quality);

        /*
            添加失败原因:
            1.发送帧超过长度
            2.压缩格式下信息体地址不连续
        */
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {

            DTULOG(DTU_WARN, "遥测IOA[0x%04X]添加数据失败,构造新io发送", item.first);
    
            newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION, TEMP_OA, TEMP_CA, false, false);

            ASDUAttr.emplace_back(newAsdu);

            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"遥测IOA[0x%04X]重构后添加数据失败", item.first);
            }

        }

        if (io) {
            InformationObject_destroy(io);
            io = nullptr;
        }
    }

    std::string acheck = DTU::DBManager::instance().GetParamAttributeCheckBuff();
    std::string check = DTU::DBManager::instance().GetParamCheckBuff();
    quality = IEC60870_QUALITY_GOOD;
    
    io = createMeasured(mtype, 0x4012, 26.0, currTime, quality);
    CS101_ASDU_addInformationObject(newAsdu, io);
    if (io) {
        InformationObject_destroy(io);
        io = nullptr;
    }
    io = createMeasured(mtype, 0x4013, 1.1, currTime, quality);
    CS101_ASDU_addInformationObject(newAsdu, io);
    if (io) {
        InformationObject_destroy(io);
        io = nullptr;
    }
    io = createMeasured(mtype, 0x4014, static_cast<float>(crc16((uint8_t*)(acheck.c_str()),acheck.size())), currTime, quality);
    CS101_ASDU_addInformationObject(newAsdu, io);
    if (io) {
        InformationObject_destroy(io);
        io = nullptr;
    }
    io = createMeasured(mtype, 0x4015, static_cast<float>(crc16((uint8_t*)(check.c_str()),check.size())), currTime, quality);
    CS101_ASDU_addInformationObject(newAsdu, io);
    if (io) {
        InformationObject_destroy(io);
        io = nullptr;
    }

    for (const auto& item : ASDUAttr)
    {
        addpayload(functionResult, item);
    }

    return functionResult;
}

std::map<IOA, bool> dtuHALhandler::GetYXValue()
{
    std::map<IOA, bool> functionResult;

    // 硬遥信
    DTU::buffer HYXBuffer;
    iectask_execute_query(PC_R_HYX_DATA, HYXBuffer);

    auto HYXAttr = DTU::DBManager::instance().GetYXIndex(DTU::YX_HYX);

    int hparamsize = HYX_LENGTH;
    if (HYXBuffer.size() != hparamsize){
        DTULOG(DTU_WARN,"硬遥信数据长度有误%u,应该为%d,已重新进行分配", HYXBuffer.size(), hparamsize);
        HYXBuffer.resize(hparamsize);
    }

    uint32_t hyx = HYXBuffer.get(0, hparamsize).value<uint32_t>();
    std::bitset<32> HYXBits(hyx);

    for(const auto& item : HYXAttr)
    {
        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.devno);

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        // printf("HYX devno [0x%04X] outfixid [0x%04X] ioa [0x%04X] USE [%s]\n", item.second.devno, outfixid, ioa, item.second.use ? "yes" : "no");

        functionResult.insert({ioa, HYXBits[item.second.offset]});
    }

    // 软遥信
    for (const auto& item : SOEMap::instance()->getAllState())
    {
        IOA ioa;
        if (IOAMap::instance()->mapHIOAtoIOA(item.first, ioa, DSYSCFG::instance().devno)) {
            functionResult.insert({ioa, item.second});
        }
    }

    return functionResult;
}

std::map<IOA, bool> dtuHALhandler::GetYXValueEx()
{
    std::map<IOA, bool> functionResult;

    // 硬遥信
    DTU::buffer YXBuffer;

    iectask_execute_query(PC_R_XY, YXBuffer);

    auto YXAttr = DTU::DBManager::instance().GetYXIndex(DTU::YX_ALL);

    int paramsize = YX_LENGTH;
    if (YXBuffer.size() != paramsize) {
        DTULOG(DTU_WARN,"遥信数据长度有误%u,应该为%d,已重新进行分配", YXBuffer.size(), paramsize);
        YXBuffer.resize(paramsize);
    }

    for(const auto& item : YXAttr)
    {

        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.devno);

        if (item.second.offset >= YX_LENGTH)
            continue;

        // printf("YX devno [0x%04X] outfixid [0x%04X] USE [%s] VALUE [%s]\n", 
        // item.second.devno, outfixid, item.second.use ? "yes" : "no",
        // (YXBuffer.get(item.second.offset, sizeof(uint8_t)).value<uint8_t>() == 1) ? "1" : "0");

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        printf("YX devno [0x%04X] outfixid [0x%04X] ioa [0x%04X] USE [%s] VALUE [%s]\n", 
        item.second.devno, outfixid, ioa, item.second.use ? "yes" : "no",
        (YXBuffer.get(item.second.offset, sizeof(uint8_t)).value<uint8_t>() == 1) ? "1" : "0");

        functionResult.insert({ ioa, (YXBuffer.get(item.second.offset, sizeof(uint8_t)).value<uint8_t>() == 1) });
    }

    return functionResult;
}

std::map<IOA, float> dtuHALhandler::GetYCValue()
{
    std::map<IOA, float> functionResult;

    // 读取公共定值
    DTU::buffer PublicParam;
    dsptask_execute_read(PC_R_PUB_FIX, PublicParam);

    // 读取遥测信息
    DTU::buffer YCBuffer;
    iectask_execute_query(PC_R_YC_DATA, YCBuffer);

    // 获取所有遥测值信息
    auto &attr = DTU::DBManager::instance().GetInfomationTableByIndex(DTU::InfomTelemetry);

    int paramsize = attr.size;
    if (YCBuffer.size() != paramsize) {
        DTULOG(DTU_WARN,"遥测数据长度有误%u,应该为%d", YCBuffer.size(), paramsize);
        YCBuffer.resize(paramsize);
    }

    for(const auto& oneItem : attr.info)
    {
        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YC, oneItem.second.fixid);

        // 未找到该点表
        if (outfixid == 0x00)
            continue;

        IOA ioa = 0x0000;
        if(!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        float value = YCBuffer.get(oneItem.second.offset, oneItem.second.size).value<float>();
        transECToYC(PublicParam, outfixid, value);
    
        // printf("添加遥测[0x%04X] OFFSET[%d] SIZE[%d] VALUE[%f]\n", ioa, oneItem.second.offset, oneItem.second.size, value);
        functionResult.insert({ioa, value});
    }

    return functionResult;
}

InformationObject dtuHALhandler::createTelegram(TELEGRAM_TYPE type, int32_t ioa, bool value, uint64_t time, QualityDescriptor q)
{
    InformationObject io;
    switch(type)
    {
        // 单点遥信
        case TELE_SINGLE_POINT:
        {
            io = (InformationObject)SinglePointInformation_create(NULL, ioa, value, q);
            break;
        }
        // 单点遥信 CP24时标
        case TELE_SINGLE_POINT_CP24:
        {
            CP24Time2a cptime_24 = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)SinglePointWithCP24Time2a_create(NULL, ioa, value, q, cptime_24);
            break;
        }
        // 单点遥信 CP56时标
        case TELE_SINGLE_POINT_CP56:
        {
            CP56Time2a cptime_56 = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)SinglePointWithCP56Time2a_create(NULL, ioa, value, q, cptime_56);
            break;
        }
        // 双点遥信
        case TELE_DOUBLE_POINT:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            io = (InformationObject)DoublePointInformation_create(NULL, ioa, dpvalue, q);
            break;
        }
        // 双点遥信 带CP24时标
        case TELE_DOUBLE_POINT_CP24:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            CP24Time2a cptime_24 = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)DoublePointWithCP24Time2a_create(NULL, ioa, dpvalue, q, cptime_24);
            break;
        }
        // 双点遥信 CP56时标
        case TELE_DOUBLE_POINT_CP56:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            CP56Time2a cptime_56 = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)DoublePointWithCP56Time2a_create(NULL, ioa, dpvalue, q, cptime_56);
            break;
        }
        default:
            DTULOG(DTU_ERROR,"错误的遥信值类型type=%u,已使用默认值[单点遥信]创建",static_cast<int>(type));
            io = (InformationObject)SinglePointInformation_create(NULL, ioa, value, q);
            break;
    }
    return io;
}

std::vector<CS101_ASDU> dtuHALhandler::createHYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom)
{
    // 是否采用压缩格式(默认 采用压缩格式)
    bool isSequence = true;
    if(csfrom == 101) {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
    }
    else {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
    }
    // ASDU发送队列
    std::vector<CS101_ASDU> ASDUAttr;

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
    ASDUAttr.emplace_back(newAsdu);

    auto attr = DTU::DBManager::instance().GetYXIndex(DTU::YX_HYX);

    int paramsize = HYX_LENGTH;
    if (yxresult.size() != paramsize){
        DTULOG(DTU_WARN,"硬遥信数据长度有误%u,应该为%d",yxresult.size(),paramsize);
        yxresult.resize(paramsize);
    }
    uint32_t hyx = yxresult.get(0, paramsize).value<uint32_t>();
    std::bitset<32> yxbits(hyx);

    // 获取遥信值类型 如果遇到未知来源,默认按照104的配置来处理
    TELEGRAM_TYPE type;
    if(csfrom == 101) {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
    }
    else {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
    }
    
    // 获取时间
    uint64_t currTime = get_current_mills();

    std::map<IOA, uint16_t> ioamap;
    // offset = 0xFFFF 为备用点表

    for(const auto& item : attr)
    {
        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.devno);

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        ioamap.insert({ioa, item.second.offset});
    }

    for (const auto& item : ioamap)
    {
        // printf("硬遥信 主站点表[0x%04X] 点表状态[%s]\n", outfixid, yxbits[item.second.offset] ? "true" : "false");
        InformationObject io = createTelegram(type, item.first, yxbits[item.second], currTime, IEC60870_QUALITY_GOOD);
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            // 发送失败原因:
            // 1.发送帧超过长度
            // 2.压缩格式下信息体地址不连续
            DTULOG(DTU_WARN,"硬遥信Fix[0x%04X]添加数据失败,构造新io发送", item.first);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"硬遥信Fix[0x%04X]重构后添加数据失败", item.first);
            }
        }

        InformationObject_destroy(io);
    }

    return ASDUAttr;
}

std::vector<CS101_ASDU> dtuHALhandler::createSYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom)
{
    // 是否采用压缩格式(默认 采用压缩格式)
    bool isSequence = true;
    if(csfrom == 101) {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
    }
    else {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
    }
    // ASDU发送队列
    std::vector<CS101_ASDU> ASDUAttr;

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
    ASDUAttr.emplace_back(newAsdu);

    auto attr = DTU::DBManager::instance().GetYXIndex(DTU::YX_SYX);

    int paramsize = SYX_LENGTH;
    if (yxresult.size() != paramsize) {
        DTULOG(DTU_WARN,"软遥信数据长度有误%u,应该为%d",yxresult.size(),paramsize);
        yxresult.resize(paramsize);
    }

    std::bitset<256> syxbits;
    uint32_t nsetsize = 0;
    array_2_bitset<256>(yxresult.get(0, paramsize).data(), 
        yxresult.get(0, paramsize).size(), syxbits, nsetsize);

    // 获取遥信值类型 如果遇到未知来源,默认按照104的配置来处理
    TELEGRAM_TYPE type;
    if(csfrom == 101) {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
    }
    else {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
    }

    // 获取时间
    uint64_t currTime = get_current_mills();

    std::map<IOA, uint16_t> ioamap;

    for(const auto& item : attr)
    {
        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.fixno);

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        ioamap.insert({ioa, item.second.offset});
    }

    for (const auto& item : ioamap)
    {
        printf("IOA 0x%04X OFFSET %u\n", item.first, item.second);
    }

    for (const auto& item : ioamap)
    {
        // printf("软遥信 主站点表[0x%04X] 点表状态[%s]\n", outfixid, syxbits[item.second.offset] ? "true" : "false");
        InformationObject io = createTelegram(type, item.first, syxbits[item.second], currTime, IEC60870_QUALITY_GOOD);
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            DTULOG(DTU_WARN,"软遥信IOA[0x%04X]添加数据失败,构造新io发送", item.first);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"软遥信IOA[0x%04X]重构后添加数据失败", item.first);
            }
        }
        InformationObject_destroy(io);
    }

    return ASDUAttr;
}

// 备用点表
#define SDL_YX_SPARE 0xFFFF
#define SDL_YX_HYX 0
#define SDL_YX_SYX 1
// 备用
#define SDL_YX_BACK 2

std::vector<CS101_ASDU> dtuHALhandler::createYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, buffer& hyxresult, buffer& syxresult, int csfrom)
{
    // 是否采用压缩格式(默认 采用压缩格式) 获取遥信值类型 如果遇到未知来源,默认按照104的配置来处理
    bool isSequence = true;
    TELEGRAM_TYPE type;
    // IOA:信息体地址 uint16_t 偏移  int 0:硬遥信 1:软遥信  || offset = 0xFFFF 为备用点表
    std::map<IOA, std::tuple<uint16_t, int>> ioamap;

    if(csfrom == 101) {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
    }
    else {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
    }

    // 获取时间
    uint64_t currTime = get_current_mills();

    // ASDU发送队列
    std::vector<CS101_ASDU> ASDUAttr;

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
    ASDUAttr.emplace_back(newAsdu);

    /////////////////// 构建硬遥信
    auto HYXAttr = DTU::DBManager::instance().GetYXIndex(DTU::YX_HYX);

    int hparamsize = HYX_LENGTH;
    if (hyxresult.size() != hparamsize){
        DTULOG(DTU_WARN,"硬遥信数据长度有误%u,应该为%d,已重新进行分配", hyxresult.size(), hparamsize);
        hyxresult.resize(hparamsize);
    }

    uint32_t hyx = hyxresult.get(0, hparamsize).value<uint32_t>();
    std::bitset<32> hyxbits(hyx);

    for(const auto& item : HYXAttr)
    {
        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.devno);

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        // printf("HYX devno [0x%04X] outfixid [0x%04X] ioa [0x%04X] USE [%s]\n", item.second.devno, outfixid, ioa, item.second.use ? "yes" : "no");

        ioamap.insert({ioa, {item.second.offset, SDL_YX_HYX}});  // 来自于硬遥信
    }

    /////////////////// 构建软遥信
    auto SYXAttr = DTU::DBManager::instance().GetYXIndex(DTU::YX_SYX);

    int sparamsize = SYX_LENGTH;
    if (syxresult.size() != sparamsize) {
        DTULOG(DTU_WARN,"软遥信数据长度有误%u,应该为%d,已重新进行分配", syxresult.size(), sparamsize);
        syxresult.resize(sparamsize);
    }

    std::bitset<256> syxbits;
    uint32_t nsetsize = 0;
    array_2_bitset<256>(syxresult.get(0, sparamsize).data(), syxresult.get(0, sparamsize).size(), syxbits, nsetsize);

    for(const auto& item : SYXAttr)
    {
        if (!item.second.use)
            continue;

        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YX, item.second.devno);

        IOA ioa = 0x0000;
        if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }

        // printf("SYX devno [0x%04X] outfixid [0x%04X] ioa [0x%04X] USE [%s]\n", item.second.devno, outfixid, ioa, item.second.use ? "yes" : "no");

        ioamap.insert({ioa, {item.second.offset, SDL_YX_SYX}});  // 来自于软遥信
    }

    /////////////////// 查找备用并添加IOA
    auto spareVec = IOAMap::instance()->findIOASpareWithType(DSYSCFG::instance().devno, IMAPT_YX);
    for (const auto& item : spareVec)
    {
        ioamap.insert({item, {SDL_YX_SPARE, SDL_YX_BACK}}); // 来自于备用
    }

    // for (const auto& item : ioamap)
    // {
    //     printf("IOA [0x%04X]\n", item.first);
    // }

    /////////////////// 添加信息体地址
    for (const auto& item : ioamap)
    {
        InformationObject io = nullptr;
        QualityDescriptor quality = IEC60870_QUALITY_INVALID;
        bool value = false;

        // 判断是否为备用点表
        if (std::get<0>(item.second) == SDL_YX_SPARE) {
            quality = IEC60870_QUALITY_INVALID;
            value = false;
        }
        else {
            
            quality = IEC60870_QUALITY_GOOD;
            // 判断需要从哪个数据集取数据
            if (std::get<1>(item.second) == SDL_YX_HYX)
                value = hyxbits[std::get<0>(item.second)];
            else
                value = syxbits[std::get<0>(item.second)];
        }
        // printf("遥信 IOA[0x%04X] 状态[%s] 备用[%s]\n", item.first, value ? "1" : "0", (quality==IEC60870_QUALITY_INVALID) ? "YES" : "NO");
        io = createTelegram(type, item.first, value, currTime, quality);

        /*
            添加失败原因:
            1.发送帧超过长度
            2.压缩格式下信息体地址不连续
        */
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {

            DTULOG(DTU_WARN,"遥信IOA[0x%04X]添加数据失败,构造新io发送", item.first);
    
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);

            ASDUAttr.emplace_back(newAsdu);

            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"遥信IOA[0x%04X]重构后添加数据失败", item.first);
            }

        }

        if (io)
            InformationObject_destroy(io);
    }

    return ASDUAttr;
}

InformationObject dtuHALhandler::createMeasured(MEASURED_TYPE type, int32_t ioa, float value, uint64_t time, QualityDescriptor q)
{
   InformationObject io = nullptr;
    switch(type)
    {
        // 短浮点数
        case MEAS_MEASURED_SHORT:
        {
            io = (InformationObject) MeasuredValueShort_create(NULL, ioa, value, q);
            break;
        }
        // 短浮点数 带CP24时标
        case MEAS_MEASURED_SHORT_CP24:
        {
            CP24Time2a cptime = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueShortWithCP24Time2a_create(NULL, ioa, value, q, cptime);
            break;
        }
        // 短浮点数 带CP56时标
        case MEAS_MEASURED_SHORT_CP56:
        {
            CP56Time2a cptime = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueShortWithCP56Time2a_create(NULL, ioa, value, q, cptime);
            break;
        }
        // 归一化值
        case MEAS_MEASURED_NORMALIZED:
        {
            io = (InformationObject)MeasuredValueNormalized_create(NULL, ioa, value, q);
            break;
        }
        // 归一化值 带CP24时标
        case MEAS_MEASURED_NORMALIZED_CP24:
        {
            CP24Time2a cptime = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueNormalizedWithCP24Time2a_create(NULL, ioa, value, q, cptime);
            break;
        }
        // 归一化值 带CP56时标
        case MEAS_MEASURED_NORMALIZED_CP56:
        {
            CP56Time2a cptime = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueNormalizedWithCP56Time2a_create(NULL, ioa, value, q, cptime);
            break;
        }
        // 标度化值
        case MEAS_MEASURED_SCALED:
        {
            io = (InformationObject)MeasuredValueScaled_create(NULL, ioa, (int)value, q);
            break;
        }
        // 标度化值 带CP24时标
        case MEAS_MEASURED_SCALED_CP24:
        {
            CP24Time2a cptime = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueScaledWithCP24Time2a_create(NULL, ioa, (int)value, q, cptime);
            break;
        }
        // 标度化值 带CP56时标
        case MEAS_MEASURED_SCALED_CP56:
        {
            CP56Time2a cptime = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)MeasuredValueScaledWithCP56Time2a_create(NULL, ioa, (int)value, q, cptime);
            break;
        }
        default:
            DTULOG(DTU_ERROR,"错误的测量值类型type=%u,已使用默认值[归一化值]创建",static_cast<int>(type));
            io = (InformationObject)MeasuredValueNormalized_create(NULL, ioa, value, q);
            break;
        
    };
    return io;
}

std::vector<CS101_ASDU> dtuHALhandler::createYC(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& ycresult, DTU::buffer& PublicParam, int csfrom)
{
    // 是否采用压缩格式(默认 采用压缩格式)
    bool isSequence = true;
    if(csfrom == 101) {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
    }
    else {
        isSequence = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
    }

    // ASDU发送队列
    std::vector<CS101_ASDU> ASDUAttr;
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
    ASDUAttr.emplace_back(newAsdu);

    auto &attr = DTU::DBManager::instance().GetInfomationTableByIndex(DTU::InfomTelemetry);

    int paramsize = attr.size;
    if (ycresult.size() != paramsize) {
        DTULOG(DTU_WARN,"遥测数据长度有误%u,应该为%d",ycresult.size(),paramsize);
        ycresult.resize(paramsize);
    }

    // 获取遥测值类型 如果遇到未知来源,默认按照104的配置来处理
    MEASURED_TYPE type;
    if(csfrom == 101) {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType;
    }
    else {
        type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType;
    }

    uint64_t currTime = get_current_mills();
    std::map<IOA, std::tuple<int, int>> ioamap;

    for(const auto& oneItem : attr.info)
    {
        auto outfixid = DBManager::instance().FixidMapIntoout(MAP_YC, oneItem.second.fixid);

        printf("YC in 0x%04X out 0x%04X\n", oneItem.second.fixid, outfixid);

        // 未找到该点表
        if (outfixid == 0x00)
            continue;

        IOA ioa = 0x0000;
        if(!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
            continue;
        }
        printf("添加遥测 [0x%04X]\n", ioa);
        ioamap.insert({ioa, {oneItem.second.offset, oneItem.second.size}});
    }

    for (const auto& item : ioamap)
    {
        float value = ycresult.get(std::get<0>(item.second), std::get<1>(item.second)).value<float>();
        printf("遥测 主站点表[0x%04X] 遥测值[%f] offset[%d] size [%d]\n", item.first, value, std::get<0>(item.second), std::get<1>(item.second));

        HIOA hioa;
        IOAMap::instance()->mapIOAtoHIOA(item.first, hioa, DSYSCFG::instance().devno);
        transECToYC(PublicParam, hioa, value);
        printf("遥测 主站点表[0x%04X] 遥测值[%f] offset[%d] size [%d]\n", item.first, value, std::get<0>(item.second), std::get<1>(item.second));

        InformationObject io = createMeasured(type, item.first, value, currTime, IEC60870_QUALITY_GOOD);

        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            DTULOG(DTU_WARN,"遥测IOA[0x%04X]添加数据失败,构造新io发送", item.first);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"遥测IOA[0x%04X]重构后添加数据失败", item.first);
            }
        }

        InformationObject_destroy(io);
    }

    return ASDUAttr;
}

CS101_ASDU dtuHALhandler::createCRC(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam)
{
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, true, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);

    // 使用短浮点数创建
    MEASURED_TYPE type = MEAS_MEASURED_SHORT;

    uint64_t currTime = get_current_mills();

    std::string acheck = DTU::DBManager::instance().GetParamAttributeCheckBuff();
    std::string check = DTU::DBManager::instance().GetParamCheckBuff();
    DTU::dtuParamCheckIndex attr = DTU::DBManager::instance().GetParamCheckTable();

    std::get<2>(attr[0]) = 1.0;    //  版本号
    std::get<2>(attr[1]) = static_cast<float>(crc16((uint8_t*)(acheck.c_str()),acheck.size()));    // 定值属性校验码
    std::get<2>(attr[2]) = static_cast<float>(crc16((uint8_t*)(check.c_str()),check.size()));      //  定值校验码

    for(const auto& item : attr)
    {
        InformationObject io = createMeasured(type, std::get<0>(item.second), std::get<2>(item.second), currTime, IEC60870_QUALITY_GOOD);
        CS101_ASDU_addInformationObject(newAsdu, io);
        InformationObject_destroy(io);
    }
    return newAsdu;
}

CS101_ASDU dtuHALhandler::createSOE(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, TELEGRAM_TYPE type, bool isDefault
                                        , bool isFrombay, int devno)
{
    bool isadd = true;
    int offset = 0;
    int addCount = 0;
    // 
    CS101_ASDU asdu = CS101_ASDU_create(alParam, false, CS101_COT_SPONTANEOUS, oa, ca, false, false);

    // 如果设置isDefault为true即使用默认配置,从获取配置文件遥信值类型
    if(isDefault) {
        // 如果遇到未知来源,默认按照104的配置来处理
        if(csfrom == 101) {
            type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType;
        }
        else {
            type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType;
        }
    }

    while(offset < data.size())
    {
        // 秒数
        uint32_t seconds = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
        offset += sizeof(uint32_t);

        // 微妙数
        uint32_t micosec = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
        offset += sizeof(uint32_t);
        // 闰秒标志位
        offset += sizeof(uint16_t);
        // SOSE类别
        uint16_t SOEtype = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
        offset += sizeof(uint16_t);
        // 原始点表值
        uint16_t fix = data.get(offset, sizeof(uint16_t)).value<uint16_t>();

        // 硬件点表值
        HIOA hioa = DBManager::instance().FixidMapIntoout(MAP_YX, fix);
        printf("SOE [0x%04X]\n", hioa);
        
        if (hioa == 0)
            isadd = false;
        else
            isadd = true;

        bool istoMaster = true;//DBManager::instance().isDevIDneedToMaster(MAP_YX, data.get(offset, sizeof(uint16_t)).value<uint16_t>());
        
        isadd = isadd && istoMaster;

        offset += sizeof(uint16_t);
        // 变化状态 (之前)
        uint8_t statuschange_before = data.get(14, sizeof(uint8_t)).value<uint8_t>();
        // 变化状态 (之后)
        uint8_t statuschange_after = data.get(15, sizeof(uint8_t)).value<uint8_t>();

        uint16_t statuschange = data.get(14, sizeof(uint16_t)).value<uint16_t>();
        offset += sizeof(uint16_t);
        // 保留
        offset += sizeof(uint32_t);

        uint16_t LOW = 0x00FF;
        uint16_t LOW8Bit = statuschange & LOW;
        bool flag = false;
        if(statuschange_after)
        {
            flag = true;
        }
        // 总毫秒数 + 从1970年时间戳 +UTC时间偏移
        uint64_t millsec = (uint64_t)seconds * (uint64_t)1000 + (uint64_t)micosec / (uint64_t)1000 + (uint64_t)946656000000 + (uint64_t)28800000;
        
        if (isadd)
        {
            IOA ioa = 0x0000;
            if (!IOAMap::instance()->mapHIOAtoIOA(hioa, ioa, devno)) {
                printf("SOE [0x%04X] devno [%d] break;\n", hioa, devno);
                continue;
            }

            addCount++;
            // 创建单SOE信息
            InformationObject io = createTelegram(type, ioa, flag, millsec, IEC60870_QUALITY_GOOD);
            CS101_ASDU_addInformationObject(asdu, io);
            InformationObject_destroy(io);

            SOEMap::instance()->updateYXStateByHIOA(hioa, flag);
        }
    }

    if (addCount == 0) {
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }

    return asdu;
}

CS101_ASDU dtuHALhandler::createCOS(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, MEASURED_TYPE type, bool isDefault
                                        , bool isFrombay, int devno)
{
    int offset = 0;
    bool isadd = true;
    int addCount = 0;
    // 
    CS101_ASDU asdu = CS101_ASDU_create(alParam, false, CS101_COT_SPONTANEOUS, oa, ca, false, false);

    DTU::buffer publicParam;
    dsptask_execute_read(PC_R_PUB_FIX, publicParam);

    std::map<IOA, std::tuple<float, uint64_t>> cosmap;

    while(offset < data.size())
    {
        // 秒数
        uint32_t secs = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
        offset+=sizeof(uint32_t);
        // 微妙
        uint32_t mirco = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
        offset+=sizeof(uint32_t);
        // 突变值
        float value = data.get(offset, sizeof(float)).value<float>();
        offset+=sizeof(float);
        // 原始硬件点表
        uint16_t fix = data.get(offset, sizeof(uint16_t)).value<uint16_t>();

        HIOA hioa = DBManager::instance().FixidMapIntoout(MAP_YC, fix);
        // 如果点表为0则表示未找到,则不上送该条COS

        if (hioa == 0)
            isadd = false;
        else
            isadd = true;

        printf("突发COS HIOA [0x%04X] [%.6f]\n", hioa, value);

        bool istoMaster = true;//DBManager::instance().isDevIDneedToMaster(MAP_YC, data.get(offset, sizeof(uint16_t)).value<uint16_t>());

        isadd = isadd && istoMaster;

        offset += sizeof(uint16_t);
        // SIQ
        uint16_t siq = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
        offset += 3*sizeof(uint16_t);
        
        // 总毫秒数 + 从1970年时间戳 +UTC时间偏移
        uint64_t millsec = (uint64_t)secs * (uint64_t)1000 + (uint64_t)mirco / (uint64_t)1000 + (uint64_t)946656000000 + (uint64_t)28800000;
        
        // 如果设置isDefault为true即使用默认配置,从获取配置文件遥测值类型
        if(isDefault) {
            // 如果遇到未知来源,默认按照104的配置来处理
            if(csfrom == 101) {
                type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType;
            }
            else {
                type = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType;
            }
        }

        if (isadd) {
            // 映射到外部点表
            IOA ioa = 0x0000;
            if (!IOAMap::instance()->mapHIOAtoIOA(hioa, ioa, devno)) {
                printf("devno [%d]break;\n", devno);
                continue;
            }
            printf("IOA [0x%04X]\n", ioa);

            transECToYC(publicParam, hioa, value);

            addCount++;

            cosmap[ioa] = {value, millsec};
        }
    }

    for (const auto& item : cosmap)
    {
        // 创建信息体 
        InformationObject io = createMeasured(type, item.first, std::get<0>(item.second), std::get<1>(item.second), IEC60870_QUALITY_GOOD);
        printf("突发遥测 IOA [0x%04X] VALUE [%.6f]\n", item.first, std::get<0>(item.second));
        CS101_ASDU_addInformationObject(asdu, io);
        InformationObject_destroy(io);
    }

    if (addCount==0) {
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }

    return asdu;
}

std::vector<DTU::buffer> dtuHALhandler::createCRCOnly(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam)
{
    std::vector<DTU::buffer> functionResult;

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, true, CS101_COT_SPONTANEOUS, ca, oa, false, false);

    // 使用短浮点数创建
    MEASURED_TYPE type = MEAS_MEASURED_SHORT;

    uint64_t currTime = get_current_mills();

    std::string acheck = DTU::DBManager::instance().GetParamAttributeCheckBuff();
    std::string check = DTU::DBManager::instance().GetParamCheckBuff();

    printf("--------------------------->\n[%s] [%f]\n", check.c_str(), static_cast<float>(crc16((uint8_t*)(check.c_str()),check.size())));

    InformationObject io = createMeasured(type, 0x4015, static_cast<float>(crc16((uint8_t*)(check.c_str()),check.size())), currTime, IEC60870_QUALITY_GOOD);
    CS101_ASDU_addInformationObject(newAsdu, io);
    InformationObject_destroy(io);

    addpayload(functionResult, newAsdu);

    return functionResult;
}

/********************************************************* 内部函数 ***********************************************************/

// 定值相关
std::vector<DTU::buffer> dtuHALhandler::presetParam(transerInfomation info, DTU::buffer &data, RemoteCtrlInfo& rinfo)
{
    std::vector<DTU::buffer> functionReault;
    CS101_AppLayerParameters alParams = createAlParams();
    DTU::buffer writecontent;
    writecontent = data;

    // 准备定值表
    settingInfo setinfo;
    std::vector<uint16_t> vecfix;
    setinfo.parse(data);
    for (const auto& item : setinfo._settings)
    {
        vecfix.emplace_back(item._addr);
    }

    // 组号
    uint16_t group = writecontent.get(0, sizeof(uint16_t)).value<uint16_t>();
    // 参数特征标志
    uint8_t tag = writecontent.get(sizeof(uint16_t), sizeof(uint8_t)).value<uint8_t>();

    ////////////////////////////////////////////////////////////////////////////////////////
    // 是否在本机执行该函数
    bool CurDevExec = false;
    // 远方调度结果集合
    std::vector<DTU::buffer> remoteResult;
    // 远方调度(这里会判断是否在本机执行函数)
    buffer btemp;
    btemp.append((char*)&info, sizeof(info));
    btemp.append(data);
    RemoteDispatchingDevice(CurDevExec, remoteResult, vecfix, "rpc_proto_write_param", btemp, rinfo);
    ////////////////////////////////////////////////////////////////////////////////////////
    if (CurDevExec)
    {
        DTULOG(DTU_INFO,"预置定值,定值区号[%u]",group);

        // 加载预置
        if (DTU_SUCCESS != dtuRuleHandler::PresetParam(writecontent, group)) {
            // 预置失败
            tag = set_bit(tag, 7, true);
            tag = set_bit(tag, 8, false);
            // 无后续, 回复预设确认
            CS101_ASDU ackAsdu = create_Write_SG_ACK(alParams, CS101_COT_DEACTIVATION_CON, group, tag);
            CS101_ASDU_setTypeID(ackAsdu,C_WS_NA_1);
            addpayload(functionReault, ackAsdu);
            // 销毁临时AlParams
            deleteAlParams(alParams);
            return functionReault;
        }
        else {
            // 预置成功
            // 如果没有后续
            if ((tag & 0x01) == 0x00) {
                tag = set_bit(tag, 7, false);
                tag = set_bit(tag, 8, true);
                // 无后续, 回复预设确认
                CS101_ASDU ackAsdu = create_Write_SG_ACK(alParams, CS101_COT_ACTIVATION_CON, group, tag);

                CS101_ASDU_setTypeID(ackAsdu,C_WS_NA_1);
                addpayload(functionReault, ackAsdu);
            }
        }

        // 销毁临时AlParams
        deleteAlParams(alParams);
    }

    if (remoteResult.size() > 0) {
        functionReault.insert(functionReault.end(), remoteResult.begin(), remoteResult.end());
    }

    return functionReault;
}

std::vector<DTU::buffer> dtuHALhandler::operateParam(transerInfomation info, DTU::buffer &data, RemoteCtrlInfo& rinfo)
{
    std::vector<DTU::buffer> functionResult;

    ////////////////////////////////////////////////////////////////////////////////////////
    // 是否在本机执行该函数
    bool CurDevExec = false;
    std::vector<uint16_t> vecfix;
    // 远方调度结果集合
    std::vector<DTU::buffer> remoteResult;
    // 远方调度(这里会判断是否在本机执行函数)
    buffer btemp;
    btemp.append((char*)&info, sizeof(info));
    btemp.append(data);
    RemoteDispatchingDevice(CurDevExec, remoteResult, vecfix, "rpc_proto_write_param", btemp, rinfo);
    ////////////////////////////////////////////////////////////////////////////////////////
    CurDevExec = true;//强制在本机执行
    if (CurDevExec)
    {
        CS101_AppLayerParameters alParams = createAlParams();
        
        CS101_ASDU ackAsdu = nullptr;
        bool haveQuestion = false;

        uint16_t group = data.get(0, sizeof(uint16_t)).value<uint16_t>();
        uint8_t tag = data.get(2, sizeof(uint8_t)).value<uint8_t>();

        if (info.payloadSize < 3) {
            DTULOG(DTU_ERROR, (char*)"固化/者撤销请求数据错误");
            haveQuestion = true;
            goto LABEL_WRITE_PARAM_EXIT;
        }

        if (tag & 0x40) {
            // 取消预置
            if(DTU_SUCCESS != dtuRuleHandler::revertPreset()) {
                DTULOG(DTU_ERROR, (char *)"定值区[%u]取消预置失败",group);
                haveQuestion = true;
                goto LABEL_WRITE_PARAM_EXIT;
            }
            DTULOG(DTU_INFO, (char *)"定值区[%u]取消预置成功",group);
            ackAsdu = create_Write_SG_ACK(alParams, CS101_COT_DEACTIVATION_CON, group, tag);
        } 
        else if ((tag & 0x80) == 0) {
            if (!DSTORE::instance().PreSettingFlag) {
                // 未预置成功直接跳转返回
                DTULOG(DTU_ERROR, (char*)"固化操作未成功进行预设,返回");
                haveQuestion = true;
                goto LABEL_WRITE_PARAM_EXIT;
            }

            uint64_t timetemp = get_current_mills();
            if (timetemp - DSTORE::instance().curTime >= 60000)
            {
                // 超时返回
                DTULOG(DTU_ERROR, (char*)"固化超时,返回");
                dtuRuleHandler::revertPreset();// 取消固化
                haveQuestion = true;
                goto LABEL_WRITE_PARAM_EXIT;
            }

            // 写入
            if(DTU_SUCCESS != dtuRuleHandler::savePreset()) {
                DTULOG(DTU_ERROR, (char *)"定值区[%u]固化参数失败");
                haveQuestion = true;
                goto LABEL_WRITE_PARAM_EXIT;
            }
            DTULOG(DTU_INFO, (char *)"定值区[%u]固化参数成功",group);
            ackAsdu = create_Write_SG_ACK(alParams, CS101_COT_ACTIVATION_CON, group, tag);
        }
        else {
            DTULOG(DTU_ERROR, (char*)"writeParam未知操作TAG:0x%04x", tag);
            haveQuestion = true;
            goto LABEL_WRITE_PARAM_EXIT;
        }

// 退出标签
LABEL_WRITE_PARAM_EXIT:

        if(haveQuestion) {
            // 存在问题的退出
            ackAsdu = create_Write_SG_ACK(alParams, CS101_COT_ACTIVATION_TERMINATION, group, tag);
            CS101_ASDU_setTypeID(ackAsdu, C_WS_NA_1);
            addpayload(functionResult, ackAsdu);
        }
        else {
            // 不存在问题的退出
            CS101_ASDU_setTypeID(ackAsdu, C_WS_NA_1);
            addpayload(functionResult, ackAsdu);
        }

        deleteAlParams(alParams);
    }

    if (remoteResult.size() > 0)
        functionResult.insert(functionResult.end(), remoteResult.begin(), remoteResult.end());

    devStaticSave.clear();

    // 预设标志清空
    DSTORE::instance().PreSettingFirstFlag = false;
    DSTORE::instance().PreSettingFlag = false;

    return functionResult;
}

//文件处理相关
std::vector<DTU::buffer> dtuHALhandler::fileReqDirectory(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> functionResult;
    // 读取目录
    std::vector<DTU::buffer> fileList;
    uint32_t dirID = 0;
    std::string dirName;
    uint8_t flag = 0;
    uint64_t begin = 0, end = 0;

    // 解析要获取的目录
    ParseDirectory(data, dirID, dirName, flag, begin, end);

    // 获取目录
    dtuRuleHandler::readFileDirectory(dirName, fileList, flag, begin, end);

    // 构建ASDU并发送
    uint32_t nCurrent = 0;
    for (;;) 
    {
        bool bMore = true;
        CS101_ASDU newAsdu = create_Directory_ACK(alParams, dirID, fileList, nCurrent, bMore);
        if (newAsdu) {
            addpayload(functionResult, newAsdu);
        }
        if (!bMore) {
            break;
        }
    }

    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::fileReqContent(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams) 
{
    std::vector<DTU::buffer> functionResult;

    std::string fileName = ParseReadfile(data);

    DTULOG(DTU_INFO, (char *)"远端获取文件:%s", fileName.c_str());

    DTU::buffer ackResult;

    dtuRuleHandler::readFileActive(fileName, ackResult);

    ////////////////////////
    // 结果
    uint8_t res = ackResult.get(0, sizeof(res)).value<uint8_t>();
    // 文件名长度
    uint8_t fileNameSize = ackResult.get(sizeof(res), sizeof(fileNameSize)).value<uint8_t>();
    // 文件ID 这里设定为文件名的CRC32校验值 如:/HISTORY/SOE/soe.xml
    uint32_t fileID = 0x00;
    // 文件大小
    uint32_t fileSize = ackResult.get(sizeof(res) + sizeof(fileNameSize) + fileNameSize + sizeof(fileID), sizeof(uint32_t)).value<uint32_t>();
    ////////////////////////

    // 创建激活确认
    CS101_ASDU newAsdu = create_Readfile_ACK(alParams, fileID, fileName, fileSize, res);
    
    DTULOG(DTU_INFO, (char *)"回复规约读文件激活确认");
    // 发送读文件激活确认,成功
    CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);
    addpayload(functionResult, newAsdu);

    // 紧跟发送文件内容
    if (res == 1) {
        // 无法读取文件,结束
        DTULOG(DTU_ERROR, (char *)"无法读取文件,返回");
        return functionResult;
    }

    DTU::buffer content;
    fileID = crc32((uint8_t*)fileName.c_str(), fileName.size());
    int ret = dtuRuleHandler::readFileContent(fileID, content);

    // 传输文件数据
    uint32_t offset = 0;
    const uint32_t readsize = 200;
    for (;;) 
    {
        // 读取文件内容
        CS101_ASDU fileAsdu = create_Readfile_CON_ACK(alParams, fileID, content.data(), content.size(), offset);
        if (fileAsdu) {
            addpayload(functionResult, fileAsdu);
            DTULOG(DTU_INFO, "回复文件内容帧");
        }  
        else
            break;
    }
    ////////////////////////////
}

void dtuHALhandler::fileTransOK(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams)
{
    uint32_t fileID = data.get(4, sizeof(uint32_t)).value<uint32_t>();  // payload + 4
    DTULOG(DTU_INFO, (char *)"文件%u传输完成", fileID);
}

std::vector<DTU::buffer> dtuHALhandler::FileRecvComfirm(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> functionResult;
    uint32_t fileID = 0, fileSize = 0;
    // 需要配置一个上传的临时目录
    std::string fileName = ParseWritefileACT(data, fileID, fileSize);

    // 临时上传文件目录
    fileName = "/update/rulekit/" + fileName;

    // 获取结果
    uint8_t actFlag = dtuRuleHandler::writeFileActive(fileID, fileName, fileSize);

    CS101_ASDU actAsdu = create_Writefile_ACT(alParams, fileID, actFlag, fileName, fileSize);
    CS101_ASDU_setTypeID(actAsdu, F_FR_NA_2);
    // 发送激活确认
    addpayload(functionResult, actAsdu);

    return functionResult;
}

std::vector<DTU::buffer> dtuHALhandler::FileRecvContent(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> functionalResult;

    uint32_t fileID = 0, fileSize = 0, filePos = 0;
    uint8_t more = 0, mod = 0;

    DTU::buffer content = ParseWritefileData(data, fileID, filePos, more, fileSize, mod);

    DTU::buffer fileContent;
    fileContent.append(content);
    if (content.size() > 0) {

        auto ret = dtuRuleHandler::writeFileContent(fileID, filePos, mod, fileContent, more);

        if (ret != 0) {
            // 出现错误
            CS101_ASDU confAsdu = create_Writefile_Complete(alParams, fileID, (uint8_t)ret, filePos);
            CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
            addpayload(functionalResult, confAsdu);
        }
        else 
        {
            if (more != 0)
            {   
                // 传输过程出现了错误
                if (ret != 0)
                {
                    CS101_ASDU confAsdu = create_Writefile_Complete(alParams, fileID, (uint8_t)ret, filePos);
                    CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
                    addpayload(functionalResult, confAsdu);
                }
            }
            else{
                // 回复传输完成
                CS101_ASDU confAsdu = create_Writefile_Complete(alParams, fileID, (uint8_t)ret, filePos);
                CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
                addpayload(functionalResult, confAsdu);
            }
        }
    }
    else {
        CS101_ASDU confAsdu = create_Writefile_Complete(alParams, fileID, 3, filePos);
        CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
        addpayload(functionalResult, confAsdu);
    }

    return functionalResult;
}

// 构建相关
CS101_AppLayerParameters dtuHALhandler::createAlParams()
{
    CS101_AppLayerParameters alParams = new sCS101_AppLayerParameters;
    alParams->maxSizeOfASDU = 249;
    alParams->originatorAddress = 0;
    alParams->sizeOfCA = 2;
    alParams->sizeOfCOT = 2;
    alParams->sizeOfIOA = 3;
    alParams->sizeOfTypeId = 1;
    alParams->sizeOfVSQ = 1;
    return alParams;
}

void dtuHALhandler::deleteAlParams(CS101_AppLayerParameters param)
{
    if(param) {
        delete param;
        param = nullptr;
    }
}

// 该函数会销毁ASDU autodestroy默认自动销毁
void dtuHALhandler::addpayload(std::vector<DTU::buffer> &buff,CS101_ASDU asdu, bool autodestroy,bool isSequence)
{
    transerInfomation transer;
    transer.Ti = CS101_ASDU_getTypeID(asdu);
    transer.COT = CS101_ASDU_getCOT(asdu);
    transer.elementNumber = CS101_ASDU_getNumberOfElements(asdu);
    transer.payloadSize = CS101_ASDU_getPayloadSize(asdu);
    transer.isSequence = isSequence;

    DTU::buffer data;
    data.append(((char*)&transer), sizeof(transerInfomation));
    if (transer.payloadSize > 0) {
        data.append((char*)CS101_ASDU_getPayload(asdu), transer.payloadSize);
        buff.emplace_back(data);
    }

    if (autodestroy) {
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
}

// 解析相关函数
void dtuHALhandler::ParseReadParam(DTU::buffer &data, uint16_t& group, uint8_t& tag, std::vector<uint32_t>& fixvec, RemoteCtrlInfo &rinfo)
{
    uint8_t *payload = (uint8_t*)(data.const_data());
    int32_t payloadsize = data.size();

    if (!payload || payloadsize == 0) {
        return;
    }

    fixvec.clear();

    group = *((uint16_t*)payload);

    tag = 0x01;//帧中并无此项,故写成固定值

    uint32_t offset = 2;

    int IOALength = 0;

    if (rinfo.cmdFrom == RC_CMD_101) {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
    }
    else {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
    }

    while(offset < payloadsize) {
        // 原始IOA
        uint32_t fix = 0;
        memcpy(&fix, payload + offset, IOALength);
        fixvec.push_back(fix);
        offset += IOALength;
    }
}

void dtuHALhandler::ParseDirectory(DTU::buffer &data, uint32_t &dirID, std::string &dirName, uint8_t &flag, uint64_t &begin, uint64_t &end) 
{
    if (data.size() == 0) {
        DTULOG(DTU_ERROR, (char *)"create_directory_asdu 文件请求目录为空");
        return;
    }

    // 获取tag
    uint32_t offset = 3;
    uint8_t opt = data.get(offset, sizeof(uint8_t)).value<uint8_t>(); // payload[3]
    offset += sizeof(opt);

    // 目录ID
    memcpy(&dirID, data.const_data() + offset, sizeof(dirID));
    offset += sizeof(uint32_t);

    // 目录名字长度
    uint8_t dirsize = 0;
    memcpy(&dirsize, data.const_data() + offset, sizeof(uint8_t));
    offset += sizeof(dirsize);

    if (dirsize == 0) {
        dirName = "/";  // 读取根目录
    } 
    else {
        dirName = std::string(data.const_data() + offset, dirsize);
        offset += dirsize;
        // 获取召唤标志
        flag = data.get(offset, sizeof(flag)).value<uint8_t>();
        if (flag == 1) {
            offset += sizeof(flag);
            // 按时间召唤
            auto beginCPTime = data.get(offset, sizeof(sCP56Time2a)).value<sCP56Time2a>();
            begin = CP56Time2a_toMsTimestamp(&beginCPTime);
            offset += 7;
            auto endCPTime = data.get(offset, sizeof(sCP56Time2a)).value<sCP56Time2a>();
            end = CP56Time2a_toMsTimestamp(&endCPTime);
        }
    }
}

std::string dtuHALhandler::ParseReadfile(DTU::buffer &data) 
{
    std::string fileName;
    if (data.size() == 0)
        return fileName;
    uint8_t namesize = data.get(4, sizeof(uint8_t)).value<uint8_t>();  // payload[4]
    fileName = std::string((data.const_data() + 5), namesize);
    return fileName;
}

std::string dtuHALhandler::ParseWritefileACT(DTU::buffer &data, uint32_t &fileID, uint32_t &filesize)
{
    std::string fileName;
    uint8_t fileNameSize = data.get(4, sizeof(uint8_t)).value<uint8_t>();  // payload[4]
    fileName = std::string((data.const_data() + 5), fileNameSize);
    // 文件ID
    fileID = data.get((5+fileNameSize), sizeof(uint32_t)).value<uint32_t>();
    // 文件大小
    filesize = data.get((5+fileNameSize+sizeof(fileID)), sizeof(uint32_t)).value<uint32_t>();
    return fileName;
}

DTU::buffer dtuHALhandler::ParseWritefileData(DTU::buffer &data, uint32_t &fileID, uint32_t &pos, uint8_t &more, uint32_t &size, uint8_t& mod)
{
    uint32_t offset = 4;
    // 文件ID
    fileID = data.get(offset, sizeof(fileID)).value<uint32_t>();
    offset += sizeof(fileID);
    // 写入位置
    pos = data.get(offset, sizeof(pos)).value<uint32_t>();
    offset += sizeof(pos);
    // 后续
    more = data.get(offset, sizeof(more)).value<uint8_t>();
    offset += sizeof(more);
    // 
    size = data.size() - offset - 1;
    offset += size;
    mod = data.get(offset, sizeof(mod)).value<uint8_t>();
    return data.get(13, (data.size()-13-1));  // payload + 13
}

int dtuHALhandler::ParseRemoteCtrl(transerInfomation &info, DTU::buffer &data, uint16_t &operate, uint32_t &fix, uint32_t &originfix, int &fixopet, bool &isexec)
{
    auto Ti = info.Ti;
    auto COT = info.COT;

    if(data.size() == 0) {
        operate = 0;
        fix = 0;
        // 未知的传动原因
        return CS101_COT_UNKNOWN_CA;
    }

    uint8_t SDCO;
    auto retCOT = COT;  // 返回值为 操作确认返回值

    int IOALength = 3;

    if(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use) {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
    }
    else {
        IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
    }

    // 检测报文是否超过了长度
    if (IOALength + 1 > data.size()) {
        // IOA长度+命令长度(1字节)
        operate = 0;
        fix = 0;
        return CS101_COT_UNKNOWN_CA;
    }

    SDCO = data.get(IOALength, sizeof(uint8_t)).value<uint8_t>();
    memcpy(&fix, data.const_data(), IOALength);

    // 1.通过单双命令判断分合闸     主要是通过这个找到内部映射点表值
    if(info.Ti == C_SC_NA_1) {
        // 单命令
        if((SDCO & 0x01) == 0x00) {
            fixopet = 2;    // 分闸(操作1)
        }
        else if((SDCO & 0x01) == 0x01) {
            fixopet = 1;    // 合闸(操作2)
        }
    }
    else if(info.Ti == C_DC_NA_1)
    {
        //双命令
        if( (SDCO & 0x03) == 0x01 ) {
            fixopet = 2;    // 分闸(操作1)
        }
        else if( (SDCO & 0x03) == 0x02 ) {
            fixopet = 1;    // 合闸(操作2)
        }
        else { 
            fixopet = 2;
            // 激活终止
            return CS101_COT_UNKNOWN_COT;
        }
    }

    operate = RC_CMD_EXE;

    // 2.映射信息体地址
    originfix = fix;

    HIOA hioa = 0x00;
    if (!IOAMap::instance()->mapIOAtoHIOA(fix, hioa)) {
        operate = 0;
        fix = 0;
        DTULOG(DTU_INFO, "遥控IOA转换为HIOA失败");
        return CS101_COT_UNKNOWN_CA;
    }

    fix = DTU::DBManager::instance().GetRMCMapFixidByoutID(hioa,fixopet,isexec);
    if (originfix == 0x6004)
        isexec = true;

    // 3.判断是否为遥控预设命令
    if((SDCO & 0x80) == 0x80) {
        // 遥控预设命令  刷掉原来的命令
        operate = RC_CMD_PRE;
    }

    // 激活确认
    retCOT = CS101_COT_ACTIVATION_CON;

    // 如果是遥控取消
    if(COT == CS101_COT_DEACTIVATION) {
        // 遥控取消
        operate = RC_CMD_CAN;
        fixopet = 1;
        retCOT = CS101_COT_DEACTIVATION_CON;
    }

    return retCOT;
}

uint64_t dtuHALhandler::ParseCurrentTime(DTU::buffer &time)
{
    uint16_t year = time.get(0,sizeof(uint16_t)).value<uint16_t>();
    uint8_t mon = time.get(2,sizeof(uint8_t)).value<uint8_t>();
    uint8_t day = time.get(3,sizeof(uint8_t)).value<uint8_t>();
    uint8_t hour = time.get(4,sizeof(uint8_t)).value<uint8_t>();
    uint8_t min = time.get(5,sizeof(uint8_t)).value<uint8_t>();
    uint8_t sec = time.get(6,sizeof(uint8_t)).value<uint8_t>();

    uint16_t msec = (((time.get(8,sizeof(uint8_t)).value<uint8_t>()) << 8) & (0xFF00) ) | 
                                                time.get(7,sizeof(uint8_t)).value<uint8_t>();
    char buf[128] = {};
    struct tm timeinfo;
    sprintf(buf,"%04u-%02u-%02u %02u:%02u:%02u",year,mon,day,hour,min,sec);
    strptime(buf, "%Y-%m-%d %H:%M:%S", &timeinfo);
    time_t timeStamp = mktime(&timeinfo);
    uint64_t timeoffset = 28800000;
    uint64_t dtime = static_cast<uint64_t>(timeStamp) * 1000 + msec;
    dtime = dtime + timeoffset;
    return dtime;
}

// 构造相关函数
CS101_ASDU dtuHALhandler::create_Write_SG_ACK(CS101_AppLayerParameters alParam, CS101_CauseOfTransmission cot, uint16_t group, uint8_t tag)
{
    tag = 0x00;
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, cot, TEMP_CA, TEMP_OA, false, false);
    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&group, sizeof(group));
    CS101_ASDU_addPayload(newAsdu, &tag, sizeof(tag));
    return newAsdu;
}

#define IOA_LENGTH 3

CS101_ASDU dtuHALhandler::create_Directory_ACK(CS101_AppLayerParameters alParam, uint32_t dirID,
                                        const std::vector<DTU::buffer> &fileList, uint32_t &currentIndex, bool &bMore)
{
    CS101_ASDU newAsdu = nullptr;
    bool bCreateAsdu = true;
    uint8_t fileNum = 0;
    // 判断文件目录下是否存在文件不存在则返回
    if(fileList.size() == 0) {
        newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_REQUEST, TEMP_CA, TEMP_OA, false, false);

        CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);

        // 信息体地址
        int infoaddr = 0x00;
        CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, IOA_LENGTH);
        // 附加数据包类型
        uint8_t packtype = 0x02;
        CS101_ASDU_addPayload(newAsdu, &packtype, sizeof(packtype));
        uint8_t optflag = F_OPT_DIR_ACK;
        // 标志
        CS101_ASDU_addPayload(newAsdu, &optflag, sizeof(optflag));
        uint8_t success = 0;
        // 成功标识
        CS101_ASDU_addPayload(newAsdu, &success, sizeof(success));
        // 目录ID
        CS101_ASDU_addPayload(newAsdu, (uint8_t *)&dirID, sizeof(dirID));
        // 后续标志
        uint8_t islast = 0;
        CS101_ASDU_addPayload(newAsdu, &islast, sizeof(islast));
        // 本帧文件数
        CS101_ASDU_addPayload(newAsdu, &fileNum, sizeof(fileNum));

        bMore = false;

        return newAsdu;
    }
    else {
        for (auto i = currentIndex; i < fileList.size(); i++) 
        {
            // 创建头部信息
            if (bCreateAsdu) 
            {
                newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_REQUEST, TEMP_CA, TEMP_OA, false, false);

                CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);

                // 信息体地址
                int infoaddr = 0x00;
                CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, IOA_LENGTH);
                // 附加数据包类型
                uint8_t packtype = 0x02;
                CS101_ASDU_addPayload(newAsdu, &packtype, sizeof(packtype));
                uint8_t optflag = F_OPT_DIR_ACK;
                // 标志
                CS101_ASDU_addPayload(newAsdu, &optflag, sizeof(optflag));
                uint8_t success = 0;
                // 成功标识
                CS101_ASDU_addPayload(newAsdu, &success, sizeof(success));
                // 目录ID
                CS101_ASDU_addPayload(newAsdu, (uint8_t *)&dirID, sizeof(dirID));
                // 后续标志
                uint8_t islast = 0;
                CS101_ASDU_addPayload(newAsdu, &islast, sizeof(islast));
                // 本帧文件数
                CS101_ASDU_addPayload(newAsdu, &fileNum, sizeof(fileNum));
                bCreateAsdu = false;
                // 当前帧 文件/文件夹 数量计数
                fileNum = 0;
                // 如果未创建成功则直接返回
                if (!newAsdu) {
                    return nullptr;
                }
            }
            // 添加文件内容
            if (i == fileList.size() - 1) { // 判断是否结束
                /* 结束 */
                auto payload = CS101_ASDU_getPayload(newAsdu);

                // 获取当前payload长度 判断和是否小于240
                auto payloadsize = CS101_ASDU_getPayloadSize(newAsdu);

                if(payloadsize + fileList[i].size() > MAX_ASDU_LENGTH) {
                    /* 添加失败 */
                    // 成功/失败标志位
                    payload[IOA_LENGTH + 2] = 0;
                    // 设定后续标志(0无后续 1有后续)
                    payload[IOA_LENGTH + 7] = 1;
                    // 设置文件数量
                    payload[IOA_LENGTH + 8] = fileNum;

                    bMore = true;

                    currentIndex = i;
                }
                else {
                    /* 添加成功 */
                    CS101_ASDU_addPayload(newAsdu, (uint8_t*)fileList[i].const_data(), fileList[i].size());
                    // 这是发送的结束
                    fileNum++;
                    // 成功/失败标志位
                    payload[IOA_LENGTH + 2] = 0;
                    // 设定后续标志(0无后续 1有后续)
                    payload[IOA_LENGTH + 7] = 0;
                    // 设置文件数量
                    payload[IOA_LENGTH + 8] = fileNum;

                    bMore = false;
                }
                return newAsdu;
            }
            else {
                /* 未结束 */
                // 获取当前payload长度 判断和是否小于249
                auto payloadsize = CS101_ASDU_getPayloadSize(newAsdu);
                if(payloadsize + fileList[i].size() > MAX_ASDU_LENGTH) {
                    /* 添加失败 */
                    auto payload = CS101_ASDU_getPayload(newAsdu);
                    // 成功/失败标志位
                    payload[IOA_LENGTH + 2] = 0;
                    // 设定后续标志(0无后续 1有后续)
                    payload[IOA_LENGTH + 7] = 1;
                    // 设置文件数量
                    payload[IOA_LENGTH + 8] = fileNum;

                    bMore = true;

                    currentIndex = i;

                    return newAsdu;
                }
                else {
                    /* 添加成功 */
                    CS101_ASDU_addPayload(newAsdu, (uint8_t*)fileList[i].const_data(), fileList[i].size());
                    fileNum++;
                }
            }
        }
    }
}

CS101_ASDU dtuHALhandler::create_Readfile_CON_ACK(CS101_AppLayerParameters alParam, uint32_t fileID, const char *filecontent,
                                        uint32_t filesize, uint32_t &offset)
{
    const uint32_t segsize = 220;

    if (offset < filesize) {
        auto readsize = std::min(segsize, filesize - offset);
        //
        uint8_t *sendcontent = (uint8_t *)(filecontent + offset);

        CS101_ASDU fileAsdu = CS101_ASDU_create(alParam, false, CS101_COT_REQUEST, TEMP_CA, TEMP_OA, false, false);
        // 设置Ti
        CS101_ASDU_setTypeID(fileAsdu, F_FR_NA_2);
        // 信息体地址
        int addr = 0x00;
        CS101_ASDU_addPayload(fileAsdu, (uint8_t *)&addr, IOA_LENGTH);
        // 附加数据包类型
        uint8_t type = 0x02;
        CS101_ASDU_addPayload(fileAsdu, &type, sizeof(type));
        // 操作标识符
        uint8_t optflg = F_OPT_RFILE_DATA;
        CS101_ASDU_addPayload(fileAsdu, &optflg, sizeof(optflg));
        // 文件ID
        CS101_ASDU_addPayload(fileAsdu, (uint8_t *)&fileID, sizeof(fileID));
        // 数据段号,文件偏移
        CS101_ASDU_addPayload(fileAsdu, (uint8_t *)&offset, sizeof(uint32_t));
        offset += readsize;
        // 后续标志
        uint8_t isEnd = 1;
        if (offset >= filesize) {
            isEnd = 0;
        }
        CS101_ASDU_addPayload(fileAsdu, &isEnd, sizeof(uint8_t));
        // 文件数据
        CS101_ASDU_addPayload(fileAsdu, sendcontent, readsize);
        // 校验码
        uint8_t mod = get_mod(sendcontent, readsize);
        CS101_ASDU_addPayload(fileAsdu, &mod, sizeof(uint8_t));

        return fileAsdu;
    }
    return nullptr;
}

CS101_ASDU dtuHALhandler::create_Readfile_ACK(CS101_AppLayerParameters alParam, uint32_t fileID, const std::string &fileName, 
                                        uint32_t filesize, uint8_t result)
{
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA, false, false);
    // Ti
    CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);
    // 信息体地址
    int addr = 0x00;
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&addr, IOA_LENGTH);
    // 附加数据包类型
    uint8_t type = 0x02;
    CS101_ASDU_addPayload(newAsdu, &type, sizeof(type));
    // 操作标识
    uint8_t flag = F_OPT_RFILE_ACT_ACK;
    CS101_ASDU_addPayload(newAsdu, &flag, sizeof(flag));
    // 结果描述符
    CS101_ASDU_addPayload(newAsdu, &result, sizeof(result));
    // 文件名长度
    uint8_t namesize = fileName.size();
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&namesize, sizeof(namesize));
    // 文件名
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)const_cast<char *>(fileName.c_str()), fileName.size());
    // 文件id
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&fileID, 4);
    // 文件大小
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&filesize, sizeof(filesize));

    return newAsdu;
}

CS101_ASDU dtuHALhandler::create_Writefile_ACT(CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res,
                                    const std::string &fileName, uint32_t filesize) 
{

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA, false, false);

    // 信息体地址
    uint16_t infoaddr =0x00;
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, sizeof(infoaddr));
    // 附加数据包类型
    uint8_t packtype = 0x02;
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&packtype, sizeof(packtype));
    // 操作标识
    uint8_t ackflag = F_OPT_WFILE_ACT_ACK;
    CS101_ASDU_addPayload(newAsdu, &ackflag, sizeof(ackflag));

    CS101_ASDU_addPayload(newAsdu, &res, sizeof(res));
    uint8_t fileNameSize = fileName.size();
    // 文件名长度
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&fileNameSize, sizeof(fileNameSize));
    // 文件名
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)fileName.c_str(), fileNameSize);
    // 文件ID
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&fileID, sizeof(fileID));
    // 文件大小
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&filesize, sizeof(filesize));

    return newAsdu;
}

CS101_ASDU dtuHALhandler::create_Writefile_Complete(CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res, uint32_t pos) 
{
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, TEMP_CA, TEMP_OA, false, false);
    // 信息体地址
    uint16_t infoaddr = 0x00;
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, sizeof(infoaddr));
    // 附加数据包类型
    uint8_t packtype = 0x02;
    CS101_ASDU_addPayload(newAsdu, &packtype, sizeof(packtype));
    // 操作标识
    uint8_t ackflag = F_OPT_WFILE_CON;
    CS101_ASDU_addPayload(newAsdu, &ackflag, sizeof(ackflag));
    // 文件ID
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&fileID, sizeof(fileID));
    // 数据段号
    CS101_ASDU_addPayload(newAsdu, (uint8_t *)&pos, sizeof(pos));
    // 结果描述
    CS101_ASDU_addPayload(newAsdu, &res, sizeof(res));

    return newAsdu;
}

void dtuHALhandler::unpack(const DTU::buffer &data, transerInfomation &info, DTU::buffer &payload)
{
    info = data.get(0, sizeof(transerInfomation)).value<transerInfomation>();
    payload = data.get(sizeof(transerInfomation), (data.size()-sizeof(transerInfomation)));
}


std::vector<DTU::buffer> dtuHALhandler::readParam(const DTU::buffer &data, RemoteCtrlInfo &rinfo)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return readParam(info, payload, rinfo);
}

std::vector<DTU::buffer> dtuHALhandler::writeParam(const DTU::buffer &data, RemoteCtrlInfo &rinfo)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return writeParam(info, payload, rinfo);
}

std::vector<DTU::buffer> dtuHALhandler::fileRequest(const DTU::buffer &data)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return fileRequest(info, payload);
}

std::vector<DTU::buffer> dtuHALhandler::changeCurrentGroup(const DTU::buffer &data)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return changeCurrentGroup(info, payload);
}

std::vector<DTU::buffer> dtuHALhandler::readCurrentGroup(const DTU::buffer &data, RemoteCtrlInfo& rinfo)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return readCurrentGroup(info, payload, rinfo);
}

std::vector<DTU::buffer> dtuHALhandler::remoteCtrl(const DTU::buffer &data, RemoteCtrlInfo rinfo)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return remoteCtrl(info, payload, rinfo);
}

std::vector<DTU::buffer> dtuHALhandler::timeCapture(DTU::buffer &data)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return timeCapture(info, payload);
}

std::vector<DTU::buffer> dtuHALhandler::unknownTypeID(DTU::buffer &data)
{
    transerInfomation info;
    DTU::buffer payload;
    unpack(data, info, payload);
    return unknownTypeID(info, payload);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 构建所有的定值的点表
static std::vector<ParamID> ParamList = {
       ParamPublic,			    // 公共定值
       ParamSoftPress,			// 软压板信息
       ParamGroupNo,			// 定值区号
       ParamRoutine,			// 常规保护
       ParamAutoReclose,		// 自动重合闸
       ParamAutoLocal,			// 就地馈线自动化
       ParamDistributFA,		// 智能分布式FA
       ParamSynchronousClose,	// 同期合闸
       ParamAutoSplit,			// 自动解列
       ParamSmallCurrent,		// 小电流接地
       ParamDisconnWarn,		// 线路断线告警
       ParamDriveSwitch,		// 传动开关
       ParamAutomation,		    // 自动化参数
   };

// 附录A遥调(新)
std::vector<DTU::buffer> dtuHALhandler::NewreadParam_A(CS101_ASDU asdu, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> result;

    std::vector<DTU::IOA> ioavec;
    bool readall = false;
    int groupno = 1;            // 定值区号暂时不看

    TYParse::parseReadParam(asdu, ioavec, readall, groupno, alParams->sizeOfIOA);

    // 将IOA组按CA进行分组
    std::map<CA, std::vector<IOA>> findmap;
    std::map<IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> rmresult;

    // 这里的CA不会出现CA=0的情况
    for (const auto& item : ioavec)
    {
        CA ca;
        IOAMap::instance()->whereIOAFrom(item, ca);

        findmap[ca].emplace_back(item);
    }

    // 按照CA调用readParam_A函数 将结果打包

    if (DSYSCFG::instance().isPublic()) {
        for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
        {
            if (!item.second.use)
                continue;

            // 看看找没找到CA,没找到则无需查找定值
            auto ita = findmap.find(item.second.ca);
            if (ita == findmap.end())
                continue;

            auto temp = dtuHALhandler::call<std::map<IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>>>(item.second, "rpc_proto_readParam_A", ita->second);

            // 追加结果
            rmresult.insert(temp.begin(), temp.end());
        }
    }

    // int groupno = DSTORE::instance().get_current_group();

    // 结果打包
    auto resultpack = TYParse::readParamReturn(rmresult, alParams, CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), groupno);

    for (const auto&item : resultpack)
    {
        addpayload(result, item);
    }

    return result;
}

std::map<IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> dtuHALhandler::readParam_A(std::vector<IOA> ioavec)
{
    // 返回结果
    std::map<IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> result;

    std::map<ParamID, DTU::buffer> ParamBuffer;

    for(auto& item : ParamList)
    {
        auto cmd = DTU::DParamConfig::instance().get_read_cmd(item);
        DTU::buffer paramResult;
        // 直接从DSP中读取,而不是从数据库中读取
        dsptask_execute_read(cmd, paramResult);

        ParamBuffer.insert({item, paramResult});
    }

    for (const auto& item : ioavec)
    {
        HIOA hioa = 0x0000;
        // 将外部IOA映射为内部HIOA
        if (!IOAMap::instance()->mapIOAtoHIOA(item, hioa, DSYSCFG::instance().devno)) {
            // 如果无法映射则不进行查询,跳过本次查询
            continue;
        }

        // 这里infixid为内部硬件ID
        uint16_t infixid = DBManager::instance().FixidMapOuttoin(hioa);

        //////// 单个信息组合

        // 数据类型
        uint16_t val_type = DTU::DParamConfig::instance().get_value_type(infixid);
        uint8_t Tag = 0;

        switch(val_type)
        {
            case PType8_t  :Tag = 32;break;
            case PType16_t :Tag = 45;break;
            case PType32_t :Tag = 35;break;
            case PTypeFLO_t:Tag = 38;break;
            case PTypeSTR_t:Tag = 4 ;break;
            case PTypeBool :Tag = 1 ;break;
            default:
                DTULOG(DTU_ERROR,"未知的Tag标识[%u]", val_type);
        }

        // 获取buffer
        uint8_t vlen = DTU::DParamConfig::instance().get_value_length(TABLE_PARAM, infixid);
        uint32_t offset = DTU::DParamConfig::instance().get_value_offset(TABLE_PARAM, infixid);
        
        // PID类型
        ParamID pid = static_cast<ParamID>(DBManager::instance().GetParamIDByFixid(infixid));
        DTU::buffer value = ParamBuffer[pid].get(offset, vlen);
        result.insert({item, {Tag, vlen, value}});
    }

    return result;
}

std::vector<DTU::buffer> dtuHALhandler::NewwriteParam_A(CS101_ASDU asdu, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> result;

    // 获取元素个数
    int count = CS101_ASDU_getNumberOfElements(asdu);

    TYParse::TYPacker parsepack;
    bool isPreset = false;
    bool isConfirm = false;
    int group = 1;          // 暂时不看

    // uint6_t 为硬件点表地址
    std::map<CA, std::map<uint16_t, DTU::buffer>> findmap;

    // 解析写定值信息
    TYParse::parseWriteParam(asdu, parsepack, isPreset, isConfirm, group, alParams->sizeOfIOA);

    if (count == 0) {
        // 预置确认/取消报文
        std::map<uint16_t, DTU::buffer> presetmap;

        for (const auto& item : parsepack)
        {
            // 查找IOA来源
            CA ca;
            if (!IOAMap::instance()->whereIOAFrom(item.first, ca))
                continue;

            // 获取数据长度
            HIOA hioa = 0x0000;
            // 将外部IOA映射为内部HIOA
            if (!IOAMap::instance()->mapIOAtoHIOA(item.first, hioa, ca)) {
                // 如果无法映射则不进行查询,跳过本次查询
                printf("YT A IOA [0x%04X] break;\n", item.first);
                continue;
            }

            // 是否备用
            if (IOAMap::instance()->testYTSpare(item.first)) {
                printf("YT A CA [%d] IOA [0x%04X] SPARE break;\n", ca ,item.first);
                continue;
            }

            // 这里infixid为内部硬件ID
            uint16_t infixid = DBManager::instance().FixidMapOuttoin(hioa);

            // 添加对应值
            presetmap.insert({infixid, std::get<2>(item.second)});
        }

        if (DSYSCFG::instance().isPublic()) {
            for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
            {
                if (!item.second.use)
                    continue;

                // 看看找没找到CA,没找到则无需查找定值
                auto ita = findmap.find(item.second.ca);
                if (ita == findmap.end())
                    continue;

                dtuHALhandler::call(item.second, "rpc_proto_preset_A", presetmap);
            }
        }
    }
    else {
        // 预置报文
        if (DSYSCFG::instance().isPublic()) {
            for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
            {
                if (!item.second.use)
                    continue;

                dtuHALhandler::call(item.second, "rpc_proto_confirm_A", !isConfirm);
            }
        }
    }

    // 创建报文回复
    

    return result;
}

void dtuHALhandler::Preset_A(std::map<uint16_t, DTU::buffer> presetmap)
{
    // 设置编辑定制区
    DBManager::instance().SetEditGroup(DBManager::instance().GetCurrGroup());
    // 预置定值
    for (const auto& item : presetmap)
    {
        DTULOG(DTU_INFO, "预设定值[0x%04X]", item.first);
        if (!DBManager::instance().setPreParamValue(item.first, item.second)) {
            DTULOG(DTU_ERROR, "[0x%04X]预设定值失败", item.first);
        }
    }
}

void dtuHALhandler::Confirm_A(bool isConfirm)
{
    if (isConfirm) {
        // 确认预设定值
        DTU::DSTORE::instance().save_presetting_data(dsptask_execute_write);
    }
    else {
        // 撤销预设定值
        DTU::DSTORE::instance().revert_setting_data();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 更新规约相关


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 附录B遥调定值 该类型结果一般不会超过一帧数据
// 主站读取定值 Ti=108 (0x6C)
std::vector<DTU::buffer> dtuHALhandler::NewreadParam_B(CS101_ASDU asdu, CS101_AppLayerParameters alParams, RemoteCtrlInfo &rinfo)
{
    std::vector<DTU::buffer> functionResult;

    try 
    {

    // 获取元素个数
    int num = CS101_ASDU_getNumberOfElements(asdu);
    
    // 获取载荷
    DTU::buffer payload((char*)CS101_ASDU_getPayload(asdu), CS101_ASDU_getPayloadSize(asdu));

    std::map<CA, std::vector<IOA>> findmap;

    int IOALength = alParams->sizeOfIOA;

    // 解析载荷
    int offset = 0;
    for (;offset < payload.size();)
    {
        IOA ioa = 0x0000;
        // 获取IOA
        if (IOALength < 2)
            ioa = payload.get(offset, IOALength).value<uint8_t>();
        else
            ioa = payload.get(offset, IOALength).value<IOA>();

        offset += IOALength;
        // 跳过空数据(4字节)
        offset += 4;
        // 查找IOA是否在需求点表中,不在则返回

        CA ca;
        if (!IOAMap::instance()->whereIOAFrom(ioa, ca))
            continue;

        findmap[ca].emplace_back(ioa);
        printf("遥调 CA [%d] IOA [0x%04X]\n", ca, ioa);
    }

    std::map<IOA, DTU::buffer> rmresult;
    // 调用远方函数readParam_B
    // 如果是公共单元则进行远端读取
    if (DSYSCFG::instance().isPublic()) {
        for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
        {
            if (!item.second.use)
                continue;

            // 看看找没找到CA,没找到则无需查找定值
            auto ita = findmap.find(item.second.ca);
            if (ita == findmap.end())
                continue;

            auto temp = dtuHALhandler::call<std::map<IOA, DTU::buffer>>(item.second, "rpc_proto_readParam_B", ita->second);

            // 追加结果
            rmresult.insert(temp.begin(), temp.end());
        }
    }

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_OA, TEMP_CA, false, false);

    int frameCount = 0;

    for (const auto& item : rmresult)
    {
        // 制作返回包
        DTU::buffer pack;
        // 追加IOA
        if (IOALength <= 2)
            pack.append((char*)(&item.first), IOALength);
        else {
            pack.append((char*)(&item.first), 2);
            pack.append(DTU::buffer(1));
        }
            
        // 追加数据
        if (item.second.size() == 4)
            pack.append(item.second);
        else {
            pack.append(item.second);                       // 追加数据
            pack.append(DTU::buffer(4-item.second.size())); // 追加空数据,补全4字节
        }

        if (!CS101_ASDU_addPayload(newAsdu, (uint8_t*)(pack.const_data()), pack.size())) {
            CS101_ASDU_setTypeID(newAsdu, (IEC60870_5_TypeID)108);
            CS101_ASDU_setNumberOfElements(newAsdu, frameCount);
            addpayload(functionResult, newAsdu);
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, TEMP_OA, TEMP_CA, false, false);
            CS101_ASDU_addPayload(newAsdu, (uint8_t*)(pack.const_data()), pack.size());
            frameCount = 0;
        }
        frameCount++;
    }

    CS101_ASDU_setTypeID(newAsdu, (IEC60870_5_TypeID)108);
    CS101_ASDU_setNumberOfElements(newAsdu, frameCount);
    addpayload(functionResult, newAsdu);

    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR, "NewreadParam_B发生未知错误:%s", e.what());
    }

    return functionResult;
}

std::map<IOA, DTU::buffer> dtuHALhandler::readParam_B(std::vector<IOA> ioavec)
{
    // 结果集合
    std::map<IOA, DTU::buffer> functionResult;

    // 读取并存储定值内容
    std::map<ParamID, DTU::buffer> ParamBuffer;
    for(auto& item : ParamList)
    {
        auto cmd = DTU::DParamConfig::instance().get_read_cmd(item);
        DTU::buffer paramResult;
        // 直接从DSP中读取,而不是从数据库中读取
        dsptask_execute_read(cmd, paramResult);

        ParamBuffer.insert({item, paramResult});
    }

    for (const auto& item : ioavec)
    {
        HIOA hioa = 0x0000;
        // 将外部IOA映射为内部HIOA
        if (!IOAMap::instance()->mapIOAtoHIOA(item, hioa, DSYSCFG::instance().devno)) {
            // 如果无法映射则不进行查询,跳过本次查询
            continue;
        }

        // 检查是否为备用
        if (IOAMap::instance()->testYTSpare(item)) {
            // 如果是备用 插入0
            functionResult.insert({item, DTU::buffer(4)});
            continue;
        }

        // 这里infixid为内部硬件ID
        uint16_t infixid = DBManager::instance().FixidMapOuttoin(hioa);

        // 获取buffer
        uint8_t vlen = DTU::DParamConfig::instance().get_value_length(TABLE_PARAM, infixid);
        uint32_t offset = DTU::DParamConfig::instance().get_value_offset(TABLE_PARAM, infixid);
        uint16_t val_type = DTU::DParamConfig::instance().get_value_type(infixid);
        // PID类型
        ParamID pid = static_cast<ParamID>(DBManager::instance().GetParamIDByFixid(infixid));
        DTU::buffer value = ParamBuffer[pid].get(offset, vlen);

        float finalvalue = 0.0f;

        switch(val_type)
        {
            case PType8_t  : finalvalue = (float)(value.value<uint8_t>());break;
            case PType16_t : finalvalue = (float)(value.value<uint16_t>());break;
            case PType32_t : finalvalue = (float)(value.value<uint32_t>());break;
            case PTypeFLO_t: finalvalue = (float)(value.value<float>());break;
            case PTypeSTR_t: finalvalue = 0.0f;break;
            default:
                DTULOG(DTU_ERROR,"未知的类型标识[%u]", val_type);
        }

        functionResult.insert({item, DTU::buffer((char*)&finalvalue, sizeof(finalvalue))});
    }

    return functionResult;
}

// Ti=55 (0x37)
std::vector<DTU::buffer> dtuHALhandler::NewwriteParam_B(CS101_ASDU asdu, CS101_AppLayerParameters alParams)
{
    std::vector<DTU::buffer> functionResult;

    try 
    {

    // uint6_t 为硬件点表地址
    std::map<CA, std::map<uint16_t, DTU::buffer>> findmap;

    // 预置执行或者预置取消
    auto COT = CS101_ASDU_getCOT(asdu);
    bool isExec = (COT == CS101_COT_DEACTIVATION) ? false: true;

    // 解析数据
    DTU::buffer payload((char*)CS101_ASDU_getPayload(asdu), CS101_ASDU_getPayloadSize(asdu));

    // 仅解析第一个解析QDS值
    uint8_t QDS = 0x00;

    int offset = 0;
    for (;offset < payload.size();)
    {
        // 点表
        IOA ioa = 0x0000;
        if (alParams->sizeOfIOA > 2)
            ioa = payload.get(offset, 2).value<IOA>();
        else
            ioa = payload.get(offset, alParams->sizeOfIOA).value<IOA>();

        offset += alParams->sizeOfIOA;

        // 获取CA
        CA ca;
        if (!IOAMap::instance()->whereIOAFrom(ioa, ca))
            continue;

        // 原始数据
        DTU::buffer value = payload.get(offset, 4);
        offset +=4;

        // QDS
        QDS = payload.get(offset, sizeof(uint8_t)).value<uint8_t>();
        offset +=1;

        printf("YT CA [%d] IOA [0x%04X]\n", ca, ioa);

        // 获取数据长度
        HIOA hioa = 0x0000;
        // 将外部IOA映射为内部HIOA
        if (!IOAMap::instance()->mapIOAtoHIOA(ioa, hioa, ca)) {
            // 如果无法映射则不进行查询,跳过本次查询
            printf("YT IOA [0x%04X] break;\n", ioa);
            continue;
        }

        // 是否备用
        if (IOAMap::instance()->testYTSpare(ioa)) {
            printf("YT CA [%d] IOA [0x%04X] SPARE break;\n", ca ,ioa);
            continue;
        }

        // 这里infixid为内部硬件ID
        uint16_t infixid = DBManager::instance().FixidMapOuttoin(hioa);
        uint8_t vlen = DTU::DParamConfig::instance().get_value_length(TABLE_PARAM, infixid);
        uint16_t val_type = DTU::DParamConfig::instance().get_value_type(infixid);

        float tempvalue = value.value<float>();

        DTU::buffer finalvalue(4);

        switch(val_type)
        {
            case PType8_t  : {
                uint8_t temp = (uint8_t)tempvalue;
                finalvalue.set(0, (char*)&temp, sizeof(temp));
                break;
            }
            case PType16_t :  {
                uint16_t temp = (uint16_t)tempvalue;
                finalvalue.set(0, (char*)&temp, sizeof(temp));
                break;
            }
            case PType32_t : {
                uint32_t temp = (uint32_t)tempvalue;
                finalvalue.set(0, (char*)&temp, sizeof(temp));
                break;
            }
            case PTypeFLO_t: {
                finalvalue = value;break;
            }
            case PTypeSTR_t: ;break;
            default:
                DTULOG(DTU_ERROR,"未知的类型标识[%u]", val_type);
        }

        findmap[ca].insert({infixid, finalvalue});
        printf("写定值 CA [%d] HIOA [0x%04X] DEVFIX [0x%04X] VALUE [%f]\n", ca, hioa, infixid, finalvalue.value<float>());
        finalvalue.dump(0, finalvalue.size());
    }

    // 解析QDS和COT 判断是要做什么
    bool isPreset = ((QDS & 0x80) == 0x80);

    if (isPreset) {
        DTULOG(DTU_INFO, "预置定值");
        // 预置命令
        if (DSYSCFG::instance().isPublic()) {
            for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
            {
                if (!item.second.use)
                    continue;

                // 看看找没找到CA,没找到则无需查找定值
                auto ita = findmap.find(item.second.ca);
                if (ita == findmap.end())
                    continue;

                dtuHALhandler::call(item.second, "rpc_proto_preset_B", ita->second);
            }
        }
    }
    else {
        DTULOG(DTU_INFO, "%s定值", isExec ? "确认" : "取消");
        // 执行或者预置取消
        if (DSYSCFG::instance().isPublic()) {
            for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
            {
                if (!item.second.use)
                    continue;

                // 看看找没找到CA,没找到则无需查找定值
                auto ita = findmap.find(item.second.ca);
                if (ita == findmap.end())
                    continue;

                dtuHALhandler::call(item.second, "rpc_proto_confirm_B", isExec);
            }
        }
    }

    if (COT == CS101_COT_DEACTIVATION)
        COT = CS101_COT_DEACTIVATION_CON;   // 停止激活确认
    else 
        COT = CS101_COT_ACTIVATION_CON;     // 激活确认

    CS101_ASDU_setCOT(asdu, COT);

    // 创建回复包
    addpayload(functionResult, asdu, false);
    
    // 防止段错误
    // asdu = CS101_ASDU_create(alParams, false, COT, TEMP_OA, TEMP_CA, false, false);

    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR, "NewwriteParam_B解析发生未知错误%s", e.what());
    }

    return functionResult;
}

void dtuHALhandler::Preset_B(std::map<uint16_t, DTU::buffer> presetmap)
{
    // 设置编辑定制区
    DBManager::instance().SetEditGroup(DBManager::instance().GetCurrGroup());
    // 预置定值
    for (const auto& item : presetmap)
    {
        DTULOG(DTU_INFO, "预设定值[0x%04X]", item.first);
        if (!DBManager::instance().setPreParamValue(item.first, item.second)) {
            DTULOG(DTU_ERROR, "[0x%04X]预设定值失败", item.first);
        }
    }
}

void dtuHALhandler::Confirm_B(bool isConfirm)
{
    if (isConfirm) {
        // 确认预设定值
        DTU::DSTORE::instance().save_presetting_data(dsptask_execute_write);
    }
    else {
        // 撤销预设定值
        DTU::DSTORE::instance().revert_setting_data();
    }
}