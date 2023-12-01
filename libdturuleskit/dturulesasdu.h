/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturulesasdu.h
  *Description:
    解析ASDU
    将结果封装成ASDU
  *History:
    1, 创建, wangjs, 2021-11-16
**********************************************************************************/
#ifndef DTURULESPARAM_H
#define DTURULESPARAM_H
#include <string>
#include <lib60870/iec60870_common.h>
#include <lib60870/hal_time.h>
#include <dtustructs.h>
#include <dtubuffer.h>
#include <functional>
#include <dtucommon.h>
#include <dtustructs.h>
#include <dtusystemconfig.h>

#include <mutex>
#include <atomic>
#include <functional>
#include <thread>

typedef struct sIMasterConnection* IMasterConnection;

using namespace DTU;
using namespace DTUCFG;

// 解析读取目录ASDU
void parse_directory_asdu(CS101_ASDU asdu, uint32_t &dirID, std::string &dirName, uint8_t &flag, uint64_t &begin, uint64_t &end);
// 解析读取文件ASDU
std::string parse_readfile_asdu(CS101_ASDU asdu);
// 解析读取文件确认ASDU
uint32_t parse_readfile_asdu_ack(CS101_ASDU asdu);
// 解析写文件激活ASDU
std::string parse_writefile_asdu_act(CS101_ASDU asdu, uint32_t &fileID, uint32_t &filesize);
// 解析定值区切换ASDU
uint32_t parse_change_group(CS101_ASDU asdu);
// 解析写文件ASDU
uint8_t *parse_writefile_asdu_data(CS101_ASDU asdu, uint32_t &fileID, uint32_t &pos, uint8_t &more, uint32_t &size, uint8_t &mod);
// 解析写定值/参数
void parse_write_setting_asdu(CS101_ASDU asdu, DTU::buffer& result);
// 解析读取定值/参数
void parse_read_setting_asdu(CS101_ASDU asdu, uint16_t& group, uint8_t& tag, std::vector<uint32_t>& fixvec);
// 解析遥控参数
int parase_remote_ctrl(CS101_ASDU asdu, uint16_t &operate,uint32_t &fix);

// 创建硬遥信ASDU
std::vector<CS101_ASDU> create_hyx_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom);

// 创建软遥信ASDU
std::vector<CS101_ASDU> create_syx_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom);

// 创建遥测ASDU
std::vector<CS101_ASDU> create_yc_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& ycresult, int csfrom);

// 创建CRC遥测ASDU
CS101_ASDU create_crc_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam);

// 批量发送发送ADSU
bool send_asdu_attr(IMasterConnection connection, std::vector<CS101_ASDU> &attr);

// 创建目录召唤确认ASDU
CS101_ASDU create_directory_asdu_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t dirID,
                                     const std::vector<DTU::buffer> &fileList, uint32_t &currentIndex, bool &bMore);

// 读取文件确认ASDU
CS101_ASDU create_readfile_asdu_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID,
                                    const std::string &fileName, uint32_t filesize, uint8_t result);
// 文件内容ASDU
CS101_ASDU create_readfile_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, const char *filecontent,
                                uint32_t filesize, uint32_t &offset);

// 写文件激活确认ASDU
CS101_ASDU create_writefile_asdu_act(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res,
                                     const std::string &fileName, uint32_t filesize);

CS101_ASDU create_writefile_asdu_complete(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res,
                                          uint32_t pos);
// 定值预设回复
CS101_ASDU create_write_sg_ack(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, CS101_CauseOfTransmission cot, uint16_t group, unsigned char tag);
// 突发遥测ASDU
CS101_ASDU create_cos_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data,
									int csfrom, MEASURED_TYPE type, bool isDefault = true);
// 突发SOE上送
CS101_ASDU create_soe_asdu(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data,
                               		int csfrom, TELEGRAM_TYPE type, bool isDefault = true);
/////////////////////////////////////
// 处理文件传输
void deal_file_request(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> callback);
//////////////////////////////////////////
// 发送文件目录
void send_file_directory(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> callback);
// 发送文件内容
void send_file_content(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> callback);
// 接受读文件传输确认
void recv_file_send_ack(IMasterConnection con, CS101_ASDU asdu);
// //////////////////////////////////////////
// 写文件确认
void write_file_comfirm(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> callback);
// 写文件内容
void write_file_data(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> callback);
// ////////////////////////////////////////////
// 处理写定值相关请求
void deal_write_sg_request(IMasterConnection con, CS101_ASDU asdu, std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 读定值区号
void read_current_group(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 切换定值区
void change_current_group(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 预置定值
void preset_setting(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 操作定值(撤销/写入)
void operate_setting(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 读取定值
void read_setting(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 遥控命令
void romate_ctrl(IMasterConnection con, CS101_ASDU asdu, RemoteCtrlInfo rinfo,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 校时命令
void time_ctrl(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 未知TypeId
void unknown_TypeID(IMasterConnection con, CS101_ASDU asdu,std::function<void(CS101_ASDU, IMasterConnection)> sendfunc);
// 时钟同步
void ClockSynchronization(IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);
// 推送消息
void connectionMessage(uint8_t type,bool sent,uint8_t proto,uint32_t length,uint8_t *msg,std::string log);


/*                60870主站端函数                  */
// 形成电能量报文
void elecEnergyFormFile(CS101_ASDU asdu);
// 形成潮流电能量报文
void elecEnergyFormTideFile(CS101_ASDU asdu);
// 主站下发时钟同步命令
template<typename _funcType>
bool ClockSyncStation(CS101_AppLayerParameters AlParam, int OA, int CA, std::function<_funcType> sendfunc, CP56Time2a newTime = nullptr)
{
    if((newTime != nullptr) && (AlParam != nullptr)) {
        CP56Time2a newTime;
        CP56Time2a_createFromMsTimestamp(newTime, Hal_getTimeInMs());

        CS101_ASDU newAsdu = CS101_ASDU_create(AlParam, false, CS101_COT_ACTIVATION, OA, CA, false, false);

        CS101_ASDU_setTypeID(newAsdu, C_CS_NA_1);

        uint16_t IOA = 0;
        CS101_ASDU_addPayload(newAsdu, (uint8_t*)(&IOA), sizeof(IOA));

        CS101_ASDU_addPayload(newAsdu, (uint8_t*)(&newTime), sizeof(struct sCP56Time2a));

        return sendfunc(1, newAsdu);
    }
}

#endif // DTURULESPARAM_H
