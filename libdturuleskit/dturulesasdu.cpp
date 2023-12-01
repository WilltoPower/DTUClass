/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturulesasdu.h
  *Description:
    将结果封装成ASDU
  *History:
    1, 创建, wangjs, 2021-11-16
**********************************************************************************/
#include "dturulesasdu.h"
#include "dturuleshandle.h"
#include <string.h>
#include <dtulog.h>
#include <dtucommon.h>
#include <dtuparamconfig.h>
#include <lib60870/cs101_information_objects.h>
#include <lib60870/cs101_slave.h>
#include <lib60870/cs104_slave.h>
#include <dtucmdcode.h>
#include <iostream>
#include <dtutask_iec.h>
#include <dtutask_dsp.h>
#include "dturulesnotifymsg.h"
#include "dtudbmanager.h"

#include <rest_rpc/rest_rpc.hpp>

#include <dturulesfile.h>

using namespace DTU;

#define HYX_LENGTH 4
#define SYX_LENGTH 32

void parse_directory_asdu(CS101_ASDU asdu, uint32_t& dirID, std::string& dirName, uint8_t& flag, uint64_t& begin, uint64_t& end) 
{
    // 读取数据
    auto ca = CS101_ASDU_getCA(asdu);

    auto payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (payloadsize == 0) {
        DTULOG(DTU_ERROR, (char *)"create_directory_asdu 文件请求目录为空");
        return;
    }
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    // 获取tag
    uint32_t offset = 3;
    uint8_t opt = payload[3];
    offset += sizeof(opt);

    // 目录ID
    memcpy(&dirID, payload + offset, sizeof(dirID));
    offset += sizeof(uint32_t);
    // 目录名字长度
    uint8_t dirsize = 0;
    memcpy(&dirsize, payload + offset, sizeof(uint8_t));
    offset += sizeof(dirsize);

    if (dirsize == 0) 
    {
        // 读取根目录
        dirName = "/";
    } 
    else 
    {
        dirName = std::string((char *)(payload + offset), dirsize);
        offset += dirsize;
        // 召唤标志
        flag = payload[offset];
        if (flag == 1) {
            offset += sizeof(flag);
            // 按时间召唤
            begin = CP56Time2a_toMsTimestamp((CP56Time2a)(payload + offset));
            offset += 7;
            end = CP56Time2a_toMsTimestamp((CP56Time2a)(payload + offset));
        } 
    }
}

std::string parse_readfile_asdu(CS101_ASDU asdu) 
{
    std::string fileName;
    if (!asdu) 
    {
        return fileName;
    }
    auto payload = CS101_ASDU_getPayload(asdu);
    uint8_t namesize = payload[4];
    fileName = std::string((char *)(payload + 5), namesize);
    return fileName;
}

uint32_t parse_readfile_asdu_ack(CS101_ASDU asdu) {
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int32_t payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (payloadsize == 0) {
        return 0;
    }
    //
    uint32_t fileID = (uint32_t)(*(payload + 4));
    return fileID;
}

std::string parse_writefile_asdu_act(CS101_ASDU asdu, uint32_t &fileID, uint32_t &filesize) {

    std::string fileName;

    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (!payload || payloadsize == 0) {
        return fileName;
    }
    uint8_t fileNameSize = payload[4];
    fileName = std::string((char *)(payload + 5), fileNameSize);
    // 文件ID
    memcpy(&fileID, payload + 5 + fileNameSize, sizeof(fileID));
    // 文件大小
    memcpy(&filesize, payload + 5 + fileNameSize + sizeof(fileID), sizeof(filesize));

    return fileName;
}

uint32_t parse_change_group(CS101_ASDU asdu)
{
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (!payload || payloadsize == 0) {
        return 0;
    }

    uint16_t groupno = *((uint16_t*)(payload+3));

    return groupno;
}

// 返回值为 操作确认返回值
int parase_remote_ctrl(CS101_ASDU asdu, uint16_t &operate, uint32_t &fix,int &fixopet, bool &isexec)
{
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int payloadsize = CS101_ASDU_getPayloadSize(asdu);
    auto Ti = CS101_ASDU_getTypeID(asdu);
    auto COT = CS101_ASDU_getCOT(asdu);
    if(!payload || payloadsize == 0)
    {
        operate = 0;
        fix = 0;
        // 未知的传动原因
        return CS101_COT_UNKNOWN_CA;
    }

    uint8_t SDCO;
    auto retCOT = COT;

    SDCO = payload[3];
    memcpy(&fix,payload,sizeof(uint8_t)*3);
    
    // 1.通过单双命令判断分合闸     主要是通过这个找到内部映射点表值
    if(Ti == C_SC_NA_1) {
        // 单命令
        if((SDCO & 0x01) == 0x00) {
            fixopet = 2;    // 分闸(操作1)
        }
        else if((SDCO & 0x01) == 0x01) {
            fixopet = 1;    // 合闸(操作2)
        }
    }
    else if(Ti == C_DC_NA_1)
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
    fix = DTU::DBManager::instance().GetRMCMapFixidByoutID(fix,fixopet,isexec);

    // 3.判断是否为遥控预设命令
    if((SDCO & 0x80) == 0x80) {
        // 遥控预设命令  刷掉原来的命令
        operate = RC_CMD_PRE;
    }

    // 激活确认
    retCOT = CS101_COT_ACTIVATION_CON;

    // 如果是遥控取消
    if(COT == CS101_COT_DEACTIVATION)
    {
        // 遥控取消
        operate = RC_CMD_CAN;
        fixopet = 1;
        retCOT = CS101_COT_DEACTIVATION_CON;
    }

    return retCOT;
}

std::tuple<uint16_t, std::vector<uint32_t>> parse_param_value(CS101_ASDU asdu)
{
    // 读取数据
    auto payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if(payloadsize < 2)
    {
        DTULOG(DTU_ERROR, (char *)"parse_param_value 获取定值区号SN错误");
        return make_tuple(0,std::vector<uint32_t>());
    }
    // 偏移量
    uint32_t offset = 0;
    uint16_t SN;            //定值区号
    uint32_t fixaddr_value; //点表值
    std::vector<uint32_t> fixaddr_list;
    // 读取内容
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    //获取定值区号
    memcpy(&SN,payload,sizeof(uint16_t));
    //获取定值区号以后的点表值
    for(int i = 0;i < (payloadsize - 2)/3;i++)
    {
        memcpy(&fixaddr_value,(payload+2+offset),sizeof(uint8_t)*3);
        fixaddr_list.push_back(fixaddr_value);
        offset += 3;
    }
    return make_tuple(SN, fixaddr_list);
}

uint8_t *parse_writefile_asdu_data(CS101_ASDU asdu, uint32_t &fileID, uint32_t &pos, uint8_t &more, uint32_t &size, uint8_t& mod) {

    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int32_t payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (!payload || payloadsize == 0) {
        return nullptr;
    }
    //
    uint32_t offset = 4;
    // 文件ID
    memcpy(&fileID, payload + offset, sizeof(fileID));
    offset += sizeof(fileID);
    // 写入位置
    memcpy(&pos, payload + offset, sizeof(pos));
    offset += sizeof(pos);
    // 后续
    memcpy(&more, payload + offset, sizeof(more));
    offset += sizeof(more);
    //
    size = payloadsize - offset - 1;
    offset += size;
    memcpy(&mod, payload+offset, sizeof(mod));
    return (payload + 13);
}
void parse_read_setting_asdu(CS101_ASDU asdu, uint16_t& group, uint8_t& tag, std::vector<uint32_t>& fixvec)
{
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int32_t payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (!payload || payloadsize == 0) {
        return;
    }
    fixvec.clear();

    group = *(uint16_t*)payload;
    
    //tag = *(payload+sizeof(uint16_t));
    tag = 0x81;//帧中并无此项,故写成固定值

    uint32_t offset = 2;

    while(offset < payloadsize)
    {
        uint32_t fix = 0;
        memcpy(&fix, payload + offset, 3);
        fixvec.push_back(fix);

        offset += 3;
    }
}
void parse_write_setting_asdu(CS101_ASDU asdu, DTU::buffer& result)
{
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    int32_t payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (!payload || payloadsize == 0) {
        return;
    }
    result.append((char*)payload, payloadsize);
}

CS101_ASDU create_directory_asdu_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t dirID,
                                     const std::vector<DTU::buffer> &fileList, uint32_t &currentIndex, bool &bMore) 
{
    CS101_ASDU newAsdu = nullptr;
    bool bCreateAsdu = true;
    uint8_t fileNum = 0;
    // 判断文件目录下是否存在文件不存在则返回
    if(fileList.size() == 0)
    {
        newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_REQUEST, ca, oa, false, false);

        CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);

        // 信息体地址
        uint16_t infoaddr = 0x00;
        CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, sizeof(infoaddr));
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
        // 如果未创建成功则直接返回
        if (!newAsdu)
        {
            return nullptr;
        }

        bMore = false;

        return newAsdu;
    }

    for (auto i = currentIndex; i < fileList.size(); i++) 
    {
        ////////////////////////////////////////////////
        // 创建头部信息
        if (bCreateAsdu) 
        {
            newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_REQUEST, ca, oa, false, false);

            CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);

            // 信息体地址
            uint16_t infoaddr = 0x00;
            CS101_ASDU_addPayload(newAsdu, (uint8_t *)&infoaddr, sizeof(infoaddr));
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
            if (!newAsdu)
            {
                return nullptr;
            }
        }
        // 添加文件内容
        if (i == fileList.size() - 1)// 判断是否结束
        {/* 结束 */
            auto payload = CS101_ASDU_getPayload(newAsdu);

            // 获取当前payload长度 判断和是否小于240
            auto payloadsize = CS101_ASDU_getPayloadSize(newAsdu);

            if(payloadsize + fileList[i].size() > 240) 
            {/* 添加失败 */
                // 成功/失败标志位
                payload[4] = 0;
                // 设定后续标志(0无后续 1有后续)
                payload[9] = 1;
                // 设置文件数量
                payload[10] = fileNum;

                bMore = true;

                currentIndex = i;
            }
            else
            {/* 添加成功 */
                CS101_ASDU_addPayload(newAsdu, (uint8_t*)fileList[i].const_data(), fileList[i].size());
                // 这是发送的结束
                fileNum++;
                // 成功/失败标志位
                payload[4] = 0;
                // 设定后续标志(0无后续 1有后续)
                payload[9] = 0;
                // 设置文件数量
                payload[10] = fileNum;

                bMore = false;
            }
            return newAsdu;
        }
        else
        {/* 未结束 */
            // 获取当前payload长度 判断和是否小于240
            auto payloadsize = CS101_ASDU_getPayloadSize(newAsdu);
            if(payloadsize + fileList[i].size() > 240) 
            {/* 添加失败 */
                auto payload = CS101_ASDU_getPayload(newAsdu);
                // 成功/失败标志位
                payload[4] = 0;
                // 设定后续标志(0无后续 1有后续)
                payload[9] = 1;
                // 设置文件数量
                payload[10] = fileNum;

                bMore = true;

                currentIndex = i;

                return newAsdu;
            }
            else
            {/* 添加成功 */
                CS101_ASDU_addPayload(newAsdu, (uint8_t*)fileList[i].const_data(), fileList[i].size());
                fileNum++;
            }
        }

        ////////////////////////////////////////////////
    }
}

///////////////
CS101_ASDU create_readfile_asdu_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID,
                                    const std::string &fileName, uint32_t filesize, uint8_t result) {
    //
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, ca, oa, false, false);
    // 操作标识
    CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);
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
CS101_ASDU create_readfile_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, const char *filecontent,
                                uint32_t filesize, uint32_t &offset) {
    const uint32_t segsize = 200;
    // uint32_t offset = 0;
    if (offset < filesize) {
        // memset(content, 0, contentsize);
        auto readsize = std::min(segsize, filesize - offset);
        //
        uint8_t *sendcontent = (uint8_t *)(filecontent + offset);

        CS101_ASDU fileAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, ca, oa, false, false);

        CS101_ASDU_setTypeID(fileAsdu, F_FR_NA_2);
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
CS101_ASDU create_writefile_asdu_act(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res,
                                     const std::string &fileName, uint32_t filesize) {

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, oa, ca, false, false);

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

CS101_ASDU create_writefile_asdu_complete(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res,
                                          uint32_t pos) {

    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, CS101_COT_ACTIVATION_CON, ca, oa, false, false);
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

// 创建遥信对象
InformationObject create_tele(TELEGRAM_TYPE type, int32_t ioa, bool value, uint64_t time, QualityDescriptor q)
{
    InformationObject io;
    switch(type)
    {
        // 单点遥信
        case TELE_SINGLE_POINT:
        {
            io = (InformationObject)SinglePointInformation_create(NULL, ioa, value, IEC60870_QUALITY_GOOD);
            break;
        }
        // 单点遥信 CP24时标
        case TELE_SINGLE_POINT_CP24:
        {
            CP24Time2a cptime_24 = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)SinglePointWithCP24Time2a_create(NULL, ioa, value, IEC60870_QUALITY_GOOD, cptime_24);
            break;
        }
        // 单点遥信 CP56时标
        case TELE_SINGLE_POINT_CP56:
        {
            CP56Time2a cptime_56 = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)SinglePointWithCP56Time2a_create(NULL, ioa, value, IEC60870_QUALITY_GOOD, cptime_56);
            break;
        }
        // 双点遥信
        case TELE_DOUBLE_POINT:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            io = (InformationObject)DoublePointInformation_create(NULL, ioa, dpvalue, IEC60870_QUALITY_GOOD);
            break;
        }
        // 双点遥信 带CP24时标
        case TELE_DOUBLE_POINT_CP24:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            CP24Time2a cptime_24 = CP24Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)DoublePointWithCP24Time2a_create(NULL, ioa, dpvalue, IEC60870_QUALITY_GOOD, cptime_24);
            break;
        }
        // 双点遥信 CP56时标
        case TELE_DOUBLE_POINT_CP56:
        {
            DoublePointValue dpvalue = value?IEC60870_DOUBLE_POINT_ON:IEC60870_DOUBLE_POINT_OFF;
            CP56Time2a cptime_56 = CP56Time2a_createFromMsTimestamp(NULL, time);
            io = (InformationObject)DoublePointWithCP56Time2a_create(NULL, ioa, dpvalue, IEC60870_QUALITY_GOOD, cptime_56);
            break;
        }
        default:
            DTULOG(DTU_ERROR,"错误的遥信值类型type=%u,已使用默认值[单点遥信]创建",static_cast<int>(type));
            io = (InformationObject)SinglePointInformation_create(NULL, ioa, value, IEC60870_QUALITY_GOOD);
            break;
    }
    return io;
}

std::vector<CS101_ASDU> create_hyx_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom)
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

    for(const auto& item : attr)
    {
        InformationObject io = create_tele(type, item.second.fixno, yxbits[item.second.offset], currTime, IEC60870_QUALITY_GOOD);
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            // 发送失败原因:
            // 1.发送帧超过长度
            // 2.压缩格式下信息体地址不连续
            DTULOG(DTU_WARN,"硬遥信Fix[0x%04X]添加数据失败,构造新io发送",item.second.fixno);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"硬遥信Fix[0x%04X]重构后添加数据失败",item.second.fixno);
            }
        }
            
        InformationObject_destroy(io);
    }

    return ASDUAttr;
}

std::vector<CS101_ASDU> create_syx_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom)
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

    for(const auto& item : attr)
    {
        InformationObject io = create_tele(type, item.second.fixno, syxbits[item.second.offset], currTime, IEC60870_QUALITY_GOOD);
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            DTULOG(DTU_WARN,"软遥信Fix[0x%04X]添加数据失败,构造新io发送",item.second.fixno);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"软遥信Fix[0x%04X]重构后添加数据失败",item.second.fixno);
            }
        }
        InformationObject_destroy(io);
    }

    return ASDUAttr;
}

// 创建遥测对象
InformationObject create_measured(MEASURED_TYPE type, int32_t ioa, float value, uint64_t time, QualityDescriptor q)
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

std::vector<CS101_ASDU> create_yc_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& ycresult, int csfrom)
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

    for(const auto& oneItem : attr.info)
    {
        float value = ycresult.get(oneItem.second.offset, oneItem.second.size).value<float>();
        InformationObject io = create_measured(type, oneItem.second.fixid, value, currTime, IEC60870_QUALITY_GOOD);
        if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
            DTULOG(DTU_WARN,"遥测Fix[0x%04X]添加数据失败,构造新io发送",oneItem.second.fixid);
            newAsdu = CS101_ASDU_create(alParam, isSequence, CS101_COT_INTERROGATED_BY_STATION, ca, oa, false, false);
            ASDUAttr.emplace_back(newAsdu);
            if(!CS101_ASDU_addInformationObject(newAsdu, io)) {
                DTULOG(DTU_ERROR,"遥测Fix[0x%04X]重构后添加数据失败",oneItem.second.fixid);
            }
        }

        InformationObject_destroy(io);
    }
    return ASDUAttr;
}

CS101_ASDU create_crc_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam)
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
        InformationObject io = create_measured(type, std::get<0>(item.second), std::get<2>(item.second), currTime, IEC60870_QUALITY_GOOD);
        CS101_ASDU_addInformationObject(newAsdu, io);
        InformationObject_destroy(io);
    }
    return newAsdu;
}

bool send_asdu_attr(IMasterConnection connection, std::vector<CS101_ASDU> &attr)
{
    for(auto &ASDUitem : attr)
    {
        if(ASDUitem == nullptr) {
            continue;
        }
        // 发送ASDU
        IMasterConnection_sendASDU(connection, ASDUitem);
        // 销毁ASDU
        CS101_ASDU_destroy(ASDUitem);
        ASDUitem = nullptr;
    }
    attr.clear();
    return true;
}

CS101_ASDU create_write_sg_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, CS101_CauseOfTransmission cot, uint16_t group, unsigned char tag)
{
    CS101_ASDU newAsdu = CS101_ASDU_create(alParam, false, cot, ca, oa, false, false);

    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&group, sizeof(group));

    CS101_ASDU_addPayload(newAsdu, &tag, sizeof(tag));

    return newAsdu;
}

CS101_ASDU create_cos_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, MEASURED_TYPE type, bool isDefault)
{
    int offset = 0;
    // 
    CS101_ASDU asdu = CS101_ASDU_create(alParam, false, CS101_COT_SPONTANEOUS, oa, ca, false, false);

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
        // 点表
        uint16_t fix = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
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
        
        // 创建信息体 
        InformationObject io = create_measured(type, fix, value, millsec, IEC60870_QUALITY_GOOD);
        CS101_ASDU_addInformationObject(asdu, io);
        InformationObject_destroy(io);
    }
    return asdu;
}
CS101_ASDU create_soe_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, TELEGRAM_TYPE type, bool isDefault)
{

    int offset = 0;
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
        offset += sizeof(uint16_t);
        // SOSE类别
        uint16_t SOEtype = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
        offset += sizeof(uint16_t);
        // 点表值
        uint16_t fix = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
        // 点表值映射
        fix = DTU::DBManager::instance().GetSOEMapFixidByinID(fix);
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
        // 创建单SOE信息
        InformationObject io = create_tele(type,fix,flag,millsec,IEC60870_QUALITY_GOOD);
        CS101_ASDU_addInformationObject(asdu, io);
        InformationObject_destroy(io);
    }

    return asdu;
}
///////////////////////////////////////
void deal_file_request(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU,IMasterConnection)> callback)
{
     if (!asdu) {
        return;
    }

    // 读取数据
    auto payloadsize = CS101_ASDU_getPayloadSize(asdu);
    if (payloadsize == 0) 
    {
        DTULOG(DTU_ERROR, (char *)"deal_file_request 文件请求目录为空");
        return;
    }
    uint8_t *payload = CS101_ASDU_getPayload(asdu);
    // 获取tag
    // uint32_t offset = 0;
    uint8_t opt = payload[3];
    DTULOG(DTU_INFO, (char *)"文件处理请求[%u]",opt);
    switch (opt) 
    {
    // 读取文件目录
    case F_OPT_DIR: {
        send_file_directory(con, asdu, callback);
        break;
    }
    // 读取文件
    case F_OPT_RFILE_ACT: {
        send_file_content(con, asdu, callback);
        break;
    }
    // 读文件确认
    case F_OPT_RFILE_CON: {
        recv_file_send_ack(con, asdu);
        break;
    }
    // 上传文件
    case F_OPT_WFILE_ACT: {
        write_file_comfirm(con, asdu,callback);
        break;
    }
    // 传文件内容
    case F_OPT_WFILE_DATA: {
        write_file_data(con, asdu, callback);
        break;
    }
    default:
        DTULOG(DTU_WARN, (char *)"未知文件处理请求[%u]",opt);
        break;
    }
}

void send_file_directory(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU,IMasterConnection)> callback)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);
    // 读取目录
    std::vector<DTU::buffer> fileList;
    uint32_t dirID = 0;
    std::string dirName;
    uint8_t flag = 0;
    uint64_t begin = 0, end = 0;

    parse_directory_asdu(asdu, dirID, dirName, flag, begin, end);
    // 获取目录
    DTU::DRULESReqHandle::instance().read_file_directory(CS101_ASDU_getCA(asdu), dirName, fileList, flag, begin, end);
    // 构建ASDU并发送
    uint32_t nCurrent = 0;
    for (;;) 
    {
        bool bMore = true;
        CS101_ASDU newAsdu = create_directory_asdu_ack(CS101_ASDU_getOA(asdu),  CS101_ASDU_getCA(asdu),  alParams, dirID, fileList, nCurrent, bMore);
        if (newAsdu) {
            auto test = CS101_ASDU_getPayloadSize(newAsdu);
            callback(newAsdu, nullptr);
        }
        if (!bMore) {
            break;
        }
    }
}

void send_file_content(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU,IMasterConnection)> sendfunc) 
{
    std::string fileName = parse_readfile_asdu(asdu);

    DTULOG(DTU_INFO, (char *)"远端获取文件:%s", fileName.c_str());

    DTU::buffer ackResult;

    DTU::DRULESReqHandle::instance().read_file_active(CS101_ASDU_getCA(asdu), fileName, ackResult);

    ////////////////////////
    // 结果
    uint8_t res = ackResult.get(0, sizeof(res)).value<uint8_t>();
    // 文件名长度
    uint8_t fileNameSize = ackResult.get(sizeof(res), sizeof(fileNameSize)).value<uint8_t>();
    // 文件ID
    //uint32_t fileID = ackResult.get(sizeof(res) + sizeof(fileNameSize) + fileNameSize, sizeof(fileID)).value<uint32_t>();
    // 文件ID 这里设定为文件名的CRC32校验值 如:/HISTORY/SOE/soe.xml
    uint32_t fileID = 0x00;
    // 文件大小
    uint32_t fileSize = ackResult.get(sizeof(res) + sizeof(fileNameSize) + fileNameSize + sizeof(fileID), sizeof(fileSize)).value<uint32_t>();
    ////////////////////////

    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    // 创建激活确认
    CS101_ASDU newAsdu = create_readfile_asdu_ack(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, fileName, fileSize, res);
    // 发送读文件激活确认,成功
    CS101_ASDU_setTypeID(newAsdu, F_FR_NA_2);
    sendfunc(newAsdu, nullptr);

    if (res == 1) 
    {
        // 无法读取文件,结束
        DTULOG(DTU_ERROR, (char *)"无法读取文件,返回");
        return;
    }
    //
    DTU::buffer content;
    fileID = crc32((uint8_t*)fileName.c_str(), fileName.size());
    int ret = DTU::DRULESReqHandle::instance().read_file_content(CS101_ASDU_getCA(asdu), fileID, content);
    // 传输文件数据
    uint32_t offset = 0;
    const uint32_t readsize = 200;
    for (;;) 
    {
        // 读取文件内容
        CS101_ASDU fileAsdu =
            create_readfile_asdu(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, content.data(), content.size(), offset);
        if (fileAsdu) 
        {
            sendfunc(newAsdu, nullptr);
        } 
        else 
        {
            break;
        }
    }
    ////////////////////////////
}

void recv_file_send_ack(IMasterConnection con, CS101_ASDU asdu) 
{
    auto fileID = parse_readfile_asdu_ack(asdu);
    DTULOG(DTU_INFO, (char *)"文件%u传输完成", fileID);
}

void write_file_comfirm(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> sendfunc) 
{

    uint32_t fileID = 0, fileSize = 0;
    // 需要配置一个上传的临时目录
    std::string fileName = parse_writefile_asdu_act(asdu, fileID, fileSize);
    // 获取结果
    uint8_t actFlag = DTU::DRULESReqHandle::instance().write_file_active(CS101_ASDU_getCA(asdu), fileID, fileName, fileSize);

    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    CS101_ASDU actAsdu = create_writefile_asdu_act(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, actFlag, fileName, fileSize);
    CS101_ASDU_setTypeID(actAsdu, F_FR_NA_2);
    // 发送激活确认
    sendfunc(actAsdu,nullptr);
    // CS104_Slave_enqueueASDU(_pSlave, actAsdu);
    // CS101_ASDU_destroy(actAsdu);
}

void write_file_data(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    uint32_t fileID = 0, fileSize = 0, filePos = 0;
    uint8_t more = 0, mod = 0;
    char *content = (char*)parse_writefile_asdu_data(asdu, fileID, filePos, more, fileSize, mod);
    DTU::buffer fileContent;
    fileContent.append(content, fileSize);
    if (content) {

        auto ret = DTU::DRULESReqHandle::instance().write_file_content(CS101_ASDU_getCA(asdu), fileID, filePos, mod, fileContent, more);
        if (ret != 0)
        {
            // 出现错误
            CS101_ASDU confAsdu = create_writefile_asdu_complete(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, (uint8_t)ret, filePos);
            CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
            sendfunc(confAsdu,nullptr);
            // CS104_Slave_enqueueASDU(_pSlave, confAsdu);
            // CS101_ASDU_destroy(confAsdu);
        }
        else 
        {
            if (more != 0)
            {   
                // 传输过程出现了错误
                if (ret != 0)
                {
                    CS101_ASDU confAsdu = create_writefile_asdu_complete(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, (uint8_t)ret, filePos);
                    CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
                    sendfunc(confAsdu,nullptr);
                    // CS104_Slave_enqueueASDU(_pSlave, confAsdu);
                    // CS101_ASDU_destroy(confAsdu);
                }
            }
            else{
                // 回复传输完成
                CS101_ASDU confAsdu = create_writefile_asdu_complete(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, (uint8_t)ret, filePos);
                CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
                sendfunc(confAsdu,nullptr);
                // CS104_Slave_enqueueASDU(_pSlave, confAsdu);
                // CS101_ASDU_destroy(confAsdu);
            }
        }
    }
    else{
        CS101_ASDU confAsdu = create_writefile_asdu_complete(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, fileID, 3, filePos);
        CS101_ASDU_setTypeID(confAsdu, F_FR_NA_2);
        sendfunc(confAsdu,nullptr);
        // CS104_Slave_enqueueASDU(_pSlave, confAsdu);
        // CS101_ASDU_destroy(confAsdu);
    }
}

void deal_write_sg_request(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    if (CS101_ASDU_getPayloadSize(asdu) == 3){
        // 固化或者撤销
        operate_setting(con,asdu, sendfunc);
    }
    else {
        // 携带数据,认为是预设
        preset_setting(con, asdu, sendfunc);
    }
}

void operate_setting(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    uint8_t *payLoad = CS101_ASDU_getPayload(asdu);
    int32_t payLoadSize = CS101_ASDU_getPayloadSize(asdu);
    if (!payLoad || payLoadSize < 3){
        DTULOG(DTU_ERROR,(char*)"固化/者撤销请求数据错误");
        return;
    }

    uint16_t group = *((uint16_t*)payLoad);

    uint8_t tag = *(payLoad+2);
    CS101_ASDU ackAsdu = nullptr;
    if (tag & 0x40)
    {
        // 取消预置
        if(DTU_SUCCESS != DTU::DRULESReqHandle::instance().revert_setting_preset(CS101_ASDU_getCA(asdu)))
        {
            DTULOG(DTU_ERROR, (char *)"定值区[%u]取消预置失败",group);
            return;
        }
        DTULOG(DTU_INFO, (char *)"定值区[%u]取消预置成功",group);
        ackAsdu = 
            create_write_sg_ack(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, CS101_COT_DEACTIVATION_CON, group, tag);
    }
    else if ((tag & 0x80) == 0)
    {
        // 写入
        if(DTU_SUCCESS != DTU::DRULESReqHandle::instance().save_setting_preset(CS101_ASDU_getCA(asdu)))
        {
            DTULOG(DTU_ERROR, (char *)"定值区[%u]固化参数失败");
            return;
        }
        DTULOG(DTU_INFO, (char *)"定值区[%u]固化参数成功",group);
        ackAsdu = 
            create_write_sg_ack(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, CS101_COT_ACTIVATION_CON, group, tag);
    }
    else {
        DTULOG(DTU_ERROR,(char*)"operate_setting 104 未知操作TAG:0x%04x",tag);
        return;
    }
    CS101_ASDU_setTypeID(ackAsdu,C_WS_NA_1);
    sendfunc(ackAsdu,con);
    // IMasterConnection_sendASDU(con, ackAsdu);
    // CS101_ASDU_destroy(ackAsdu);
}

void preset_setting(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    DTU::buffer writecontent;

    parse_write_setting_asdu(asdu, writecontent);
    // 组号
    uint16_t group = writecontent.get(0, sizeof(uint16_t)).value<uint16_t>();
    // 参数特征标志
    uint8_t tag = writecontent.get(sizeof(uint16_t), sizeof(uint8_t)).value<uint8_t>();
    DTULOG(DTU_INFO,"预置定值,定值区号[%u]",group);
    // 加载预置
    if (DTU_SUCCESS !=  DTU::DRULESReqHandle::instance().write_setting_preset(CS101_ASDU_getCA(asdu), writecontent, group))
    {
        // 预置失败
        tag = set_bit(tag, 7, true);
        tag = set_bit(tag, 8, false);
        // 无后续, 回复预设确认
        CS101_ASDU ackAsdu = 
            create_write_sg_ack(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, CS101_COT_DEACTIVATION_CON, group, tag);

        CS101_ASDU_setTypeID(ackAsdu,C_WS_NA_1);
        sendfunc(ackAsdu,con);
        // IMasterConnection_sendASDU(con, ackAsdu);
        // CS101_ASDU_destroy(ackAsdu);
        return;
    }
    // 如果没有后续
    if ((tag & 0x01) == 0x00){
        tag = set_bit(tag, 7, false);
        tag = set_bit(tag, 8, true);
        // 无后续, 回复预设确认
        CS101_ASDU ackAsdu = 
            create_write_sg_ack(CS101_ASDU_getOA(asdu), CS101_ASDU_getCA(asdu), alParams, CS101_COT_ACTIVATION_CON, group, tag);

        CS101_ASDU_setTypeID(ackAsdu,C_WS_NA_1);
        sendfunc(ackAsdu,con);
        // IMasterConnection_sendASDU(con, ackAsdu);
        // CS101_ASDU_destroy(ackAsdu);
    }
}

void read_current_group(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    DTU::buffer result;
    DTU::DRULESReqHandle::instance().read_current_group(CS101_ASDU_getCA(asdu), result);
    //  
    CS101_ASDU groupAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, 
        CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), false, false);
    
    CS101_ASDU_setTypeID(groupAsdu, C_RR_NA_1);

    CS101_ASDU_addPayload(groupAsdu, (uint8_t*)result.data(), result.size());
    // //
    sendfunc(groupAsdu, con);
    // IMasterConnection_sendASDU(con, groupAsdu);

    // CS101_ASDU_destroy(groupAsdu);
}

void change_current_group(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    uint32_t group = parse_change_group(asdu);

    CS101_ASDU newAsdu = nullptr;

    if (DTU::DRULESReqHandle::instance().change_current_group(CS101_ASDU_getCA(asdu), group) != DTU_SUCCESS)
    {
       newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_UNKNOWN_IOA, 
        CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), false, false);
    }
    else{
       newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, 
        CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), false, false);
    }
     
    CS101_ASDU_setTypeID(newAsdu, C_SR_NA_1);

    sendfunc(newAsdu, con);
    // IMasterConnection_sendASDU(con, newAsdu);

    // CS101_ASDU_destroy(newAsdu);
}

void read_setting(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{

     //CS101_ASDU_getElement()
    int elementNum = CS101_ASDU_getNumberOfElements(asdu);

    std::vector<std::uint32_t> vecfix;
    std::vector<DTU::buffer> result;
    
    uint16_t group = 0;
    uint8_t tag = 0;

    parse_read_setting_asdu(asdu, group, tag, vecfix);

    // 读取全部定值
    DTU::DRULESReqHandle::instance().read_setting(CS101_ASDU_getCA(asdu), group, vecfix, result);

    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);
    CS101_ASDU newAsdu = nullptr;
    int nCount = 0;
    int nCountsize = 12;//帧总长度计数
    for(auto& item : result)
    {
        if (!newAsdu) {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, 
                 CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu),false, false);

            CS101_ASDU_addPayload(newAsdu, (uint8_t*)&group, sizeof(uint16_t));
            CS101_ASDU_addPayload(newAsdu, &tag, sizeof(tag));

            nCount = 0;
        }

        if(nCountsize + item.size() >= 254)
        {/* 超过帧长 */
            if (nCount == result.size() - 1)
            {
                tag = set_bit(tag, 1, false);
            }else{
                tag = set_bit(tag, 1, true);
            }
            CS101_ASDU_getPayload(newAsdu)[2] = tag;
            CS101_ASDU_setNumberOfElements(newAsdu, nCount);
            CS101_ASDU_setTypeID(newAsdu,C_RS_NA_1);
            sendfunc(newAsdu, con);
            // IMasterConnection_sendASDU(con, newAsdu);
            // CS101_ASDU_destroy(newAsdu);
            newAsdu = nullptr;
            nCount = 0;
            nCountsize = 12;

            //将此次结果重新添加到新的帧
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, 
                 CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu),false, false);

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
    
    if(newAsdu)
    {
        tag = set_bit(tag, 1, false);
        CS101_ASDU_getPayload(newAsdu)[2] = tag;
        CS101_ASDU_setNumberOfElements(newAsdu, nCount);
        CS101_ASDU_setTypeID(newAsdu,C_RS_NA_1);
        sendfunc(newAsdu, con);
        // IMasterConnection_sendASDU(con, newAsdu);
        // CS101_ASDU_destroy(newAsdu);
        // newAsdu = nullptr;
    }
}

void romate_ctrl(IMasterConnection con, CS101_ASDU asdu, RemoteCtrlInfo rinfo,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    // 获取通信参数
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    uint32_t fix;
    uint16_t operate;
    // 在分闸操作中是否执行
    bool exec = false;
    // 需要执行分闸还是合闸  1是合闸 2是分闸
    int fixopt = 1;
    int retCOT = parase_remote_ctrl(asdu,operate,fix,fixopt,exec);
    int CA = CS101_ASDU_getCA(asdu);
    int OA = CS101_ASDU_getOA(asdu);
    auto Ti = CS101_ASDU_getTypeID(asdu);
    // 如果exec为false(即只有执行命令)且下发的是合闸命令才下发
    int execret = DTU_SUCCESS;
    if(fixopt == 1) {
        // 合闸
        execret = DTU::DRULESReqHandle::instance().remote_control(CS101_ASDU_getCA(asdu),fix,operate,rinfo);
    }
    else if(fixopt == 2) {
        //分闸
        if(exec)
            execret = DTU::DRULESReqHandle::instance().remote_control(CS101_ASDU_getCA(asdu),fix,operate,rinfo);
    }

    int payloadsize = CS101_ASDU_getPayloadSize(asdu);
    uint8_t payload[8] = {};
    memcpy(payload,CS101_ASDU_getPayload(asdu),payloadsize);
    
    if(execret != DTU_SUCCESS)
        retCOT = CS101_COT_DEACTIVATION_CON;

    // 返回发送确认命令
    CS101_ASDU newAsdu = nullptr;
    newAsdu = CS101_ASDU_create(alParams, false, static_cast<CS101_CauseOfTransmission>(retCOT), CA, OA, false, false);
    CS101_ASDU_addPayload(newAsdu,payload,payloadsize);
    CS101_ASDU_setTypeID(newAsdu,Ti);
    sendfunc(newAsdu, con);

    // 如果是执行命令 返回确认命令
    if(operate == RC_CMD_EXE) {
        newAsdu = nullptr;
        newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_TERMINATION, CA, OA, false, false);
        CS101_ASDU_addPayload(newAsdu,payload,payloadsize);
        CS101_ASDU_setTypeID(newAsdu,Ti);
        sendfunc(newAsdu, con);
    }
}

void time_ctrl(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    if(asdu == nullptr) {
        DTULOG(DTU_ERROR,"ASDU无效");
        return;
    }
    
    int COT = CS101_ASDU_getCOT(asdu);

    if(COT != CS101_COT_REQUEST)
        return;

    // 获取时间
    DTU::buffer result;
    iectask_read_time(result);
    uint16_t year = result.get(0,sizeof(uint16_t)).value<uint16_t>();
    uint8_t mon = result.get(2,sizeof(uint8_t)).value<uint8_t>();
    uint8_t day = result.get(3,sizeof(uint8_t)).value<uint8_t>();
    uint8_t hour = result.get(4,sizeof(uint8_t)).value<uint8_t>();
    uint8_t min = result.get(5,sizeof(uint8_t)).value<uint8_t>();
    uint8_t sec = result.get(6,sizeof(uint8_t)).value<uint8_t>();

    uint16_t msec = (((result.get(8,sizeof(uint8_t)).value<uint8_t>()) << 8) & (0xFF00) ) | 
                                                result.get(7,sizeof(uint8_t)).value<uint8_t>();
    char buf[128] = {};
    struct tm timeinfo;
    sprintf(buf,"%04u-%02u-%02u %02u:%02u:%02u",year,mon,day,hour,min,sec);
    strptime(buf, "%Y-%m-%d %H:%M:%S", &timeinfo);
    time_t timeStamp = mktime(&timeinfo);
    uint64_t timeoffset = 28800000;
    uint64_t dtime = static_cast<uint64_t>(timeStamp) * 1000 + msec;
    dtime = dtime + timeoffset;
    // 获取通信参数
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);

    CS101_ASDU newAsdu = nullptr;
    newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_REQUEST, 
                CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), false, false);
    CS101_ASDU_setTypeID(newAsdu,C_CS_NA_1);
    sCP56Time2a snewTime = {};
    CP56Time2a_setFromMsTimestamp(&snewTime, dtime);
    uint16_t infofix = 0;
    // 添加信息体地址点表
    CS101_ASDU_addPayload(newAsdu,(uint8_t*)&infofix,3);
    // 添加时间点表
    CS101_ASDU_addPayload(newAsdu,(uint8_t*)&snewTime,sizeof(sCP56Time2a));
    sendfunc(newAsdu, con);
}

void unknown_TypeID(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);
    CS101_ASDU newAsdu = nullptr;
    auto Origin_Ti = CS101_ASDU_getTypeID(asdu);
    newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_UNKNOWN_TYPE_ID, 
        CS101_ASDU_getCA(asdu), CS101_ASDU_getOA(asdu), false, false);

    CS101_ASDU_setTypeID(newAsdu, Origin_Ti);

    sendfunc(newAsdu, con);
}

std::string CP56Time2aToString(CP56Time2a time)
{
    char timebuf[128] = {0};

    sprintf(timebuf,"%02d/%02d/%02d %02d:%02d:%02d",CP56Time2a_getYear(time) + 2000, CP56Time2a_getMonth(time),
                                                    CP56Time2a_getDayOfMonth(time),CP56Time2a_getHour(time), 
                                                    CP56Time2a_getMinute(time), CP56Time2a_getSecond(time));
    return std::string(timebuf);
}

void ClockSynchronization(IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    uint64_t newSystemTimeInMs = CP56Time2a_toMsTimestamp(newTime);
    
    DTU::buffer result;
    //1.换算成总秒数  2.减去8小时从格林尼治时间转为北京时间 3.转换为从2000年开始
    // 总秒数
    uint32_t time = newSystemTimeInMs/1000 - 28800 - 946656000;
    // 微秒数
    uint32_t mtime = newSystemTimeInMs - newSystemTimeInMs/(uint64_t)1000;
    result.append((char*)&time,sizeof(time));
    result.append((char*)&time,sizeof(mtime));
    if(DTU_SUCCESS != iectask_execute_time(result))
    {
        DTULOG(DTU_ERROR,"规约同步时间失败");
        return;
    }

    for(int i=0;i<2;i++)
    {
        if(AutoTime::instance().CalibrateSystemTimeOnce())
        {
            break;
        }
        else
        {
            DTULOG(DTU_WARN,"规约同步时间失败,重试%d/2",i);
        }
        if(i==2)
        {
            DTULOG(DTU_ERROR,"规约同步时间失败,返回");
            return;
        }
    }
    DTULOG(DTU_INFO,"规约同步时间成功 [%s]",CP56Time2aToString(newTime).c_str());

    // 给每一台间隔单元都同步时间
    if (DSYSCFG::instance().isPublic()) {
        for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
        {
            if (item.second.use) {
                rest_rpc::rpc_client caller(item.second.ProtoRPC.ip, item.second.ProtoRPC.port);
                caller.set_connect_timeout(100);

                if (caller.connect()) {
                    try
                    {
                        caller.call<int>("rpc_async_time", result);
                    }
                    catch(const std::exception& e)
                    {
                        DTULOG(DTU_WARN, "规约校准间隔单元[%d]时间发生错误", item.second.ca);
                    }
                }
                else {
                    DTULOG(DTU_WARN, "规约校准间隔单元[%d]时间失败", item.second.ca);
                }
            }
        }
    }
}

void connectionMessage(uint8_t type,bool sent,uint8_t proto,uint32_t length,uint8_t *msg,std::string log)
{
    notifyToolCS(type,sent,proto,length,msg,log);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 形成电能量报文
void elecEnergyFormFile(CS101_ASDU asdu)
{
    // 这里应该有 8个15min冻结 + 8个日冻结 共16个元素
    int nElements = CS101_ASDU_getNumberOfElements(asdu);
	int payloadsize = CS101_ASDU_getPayloadSize(asdu);
	uint8_t* payload = CS101_ASDU_getPayload(asdu);

    DTU::buffer data((char*)payload, payloadsize);

    uint16_t addrfirst = data.get(0,3).value<uint16_t>();
    data.remove(0,3); //移除掉信息体地址

    DTU::buffer result;

	int offset = 0;
	for (int i = 0; i < nElements; i++)
	{
		float value = data.get(offset, sizeof(float)).value<float>();
        offset += sizeof(float);
		// uint8_t QDS = data.get(offset, sizeof(uint8_t)).value<uint8_t>();
        offset +=sizeof(uint8_t);   // QDS

        CP56Time2a time = (CP56Time2a)(payload + offset);
        int year = CP56Time2a_getYear(time);
		int mon = CP56Time2a_getMonth(time);
		int day = CP56Time2a_getDayOfMonth(time);
		int hour = CP56Time2a_getHour(time);
		int min = CP56Time2a_getMinute(time);
		int sec = CP56Time2a_getSecond(time);
		int msec = CP56Time2a_getMillisecond(time);
        char buf[17] = {};
		sprintf(buf, "%02d%02d%02d_%02d%02d%02d_%03d", year, mon, day, hour, min, sec, msec);
		offset += 7;

        uint16_t addr = addrfirst + i;

        result.append(buf, sizeof(buf));
        result.append((char*)(&addr), sizeof(addr));
        result.append((char*)(&value), sizeof(value));
	}

    // 添加一条电能量报文
    DRULESFILE::instance().add_frz_rcd(result);
}

// 形成潮流电能量报文
void elecEnergyFormTideFile(CS101_ASDU asdu)
{
    int nElements = CS101_ASDU_getNumberOfElements(asdu);
	int payloadsize = CS101_ASDU_getPayloadSize(asdu);
	uint8_t* payload = CS101_ASDU_getPayload(asdu);

    DTU::buffer result;
    DTU::buffer data((char*)payload, payloadsize);
    int offset = 0;

    for (int i = 0; i < nElements; i++)
    {
        uint16_t ioa = data.get(offset, 3).value<uint16_t>();
        offset += 3;

        float value = data.get(offset, 4).value<float>();
        offset += sizeof(float);

        CP56Time2a time = (CP56Time2a)(payload + offset);
        int year = CP56Time2a_getYear(time);
		int mon = CP56Time2a_getMonth(time);
		int day = CP56Time2a_getDayOfMonth(time);
		int hour = CP56Time2a_getHour(time);
		int min = CP56Time2a_getMinute(time);
		int sec = CP56Time2a_getSecond(time);
		int msec = CP56Time2a_getMillisecond(time);
        char buf[17] = {};
		sprintf(buf, "%02d%02d%02d_%02d%02d%02d_%03d", year, mon, day, hour, min, sec, msec);
		offset += 7;

        result.append(buf,sizeof(buf));
        result.append((char*)(&ioa), sizeof(ioa));
        result.append((char*)(&value), sizeof(value));
    }
    DRULESFILE::instance().add_fro_data(result);
}
