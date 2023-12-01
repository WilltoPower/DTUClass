#ifndef _DTU_HAL_ASDU_H_
#define _DTU_HAL_ASDU_H_

#include <lib60870/cs104_connection.h>

#include <vector>

#include <rest_rpc/rest_rpc.hpp>

#include "dtusystemconfig.h"
#include "dtudbmanager.h"
#include "dtustructs.h"
#include "dtubuffer.h"
#include "dtucommon.h"
#include "dtuioamap.h"
#include "dtulog.h"

// RPC延时500ms
#ifndef _RPC_TIME_OUT
#define _RPC_TIME_OUT 100
#endif

// 定值确认函数名称
#define PARAM_FUNCTION_NAME ""

namespace DTU
{
    using namespace DTUCFG;

    class dtuHALhandler
    {
        public:
            // 从本机读取定值并打包(本机/子机) (多返回需要特殊处理)
            static std::vector<DTU::buffer> readParam(const DTU::buffer &data, RemoteCtrlInfo &rinfo);
            // 写定值(本机/子机) (多返回需要特殊处理)
            static std::vector<DTU::buffer> writeParam(const DTU::buffer &data, RemoteCtrlInfo &rinfo);
            // 文件处理请求
            static std::vector<DTU::buffer> fileRequest(const DTU::buffer &data);
            // 更换定值区(只更改本机定值区)
            static std::vector<DTU::buffer> changeCurrentGroup(const DTU::buffer &data);
            // 读取定值区(只更改本机定值区)
            static std::vector<DTU::buffer> readCurrentGroup(const DTU::buffer &data, RemoteCtrlInfo &rinfo);
            // 远方遥控(本机/子机)单返回
            static std::vector<DTU::buffer> remoteCtrl(const DTU::buffer &data, RemoteCtrlInfo rinfo);
            // 时间获取(只获取本机)
            static std::vector<DTU::buffer> timeCapture(DTU::buffer &data);
            // 未知Ti(只在本机使用)
            static std::vector<DTU::buffer> unknownTypeID(DTU::buffer &data);
            // 总召唤相关处理(弃用)
            static std::vector<DTU::buffer> IMasterConnect(int type, bool curdev);


            // (新)总召唤相关处理
            static std::vector<DTU::buffer> NewIMasterConnect(int csfrom, CS101_AppLayerParameters alParams);
            static std::map<IOA, bool> GetYXValue();
            static std::map<IOA, bool> GetYXValueEx(); // 新创建遥信
            static std::map<IOA, float> GetYCValue();


            // (新)读取定值附录A格式
            static std::vector<DTU::buffer> NewreadParam_A(CS101_ASDU asdu, CS101_AppLayerParameters alParams);
            static std::map<IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> readParam_A(std::vector<IOA> ioavec);
            static std::vector<DTU::buffer> NewwriteParam_A(CS101_ASDU asdu, CS101_AppLayerParameters alParams);

            static void Confirm_A(bool isConfirm);
            static void Preset_A(std::map<uint16_t, DTU::buffer> presetmap);

            // 读取定值 附录B格式
            static std::vector<DTU::buffer> NewreadParam_B(CS101_ASDU asdu, CS101_AppLayerParameters alParams, RemoteCtrlInfo &rinfo);
            static std::map<IOA, DTU::buffer> readParam_B(std::vector<IOA> ioavec);

            static std::vector<DTU::buffer> NewwriteParam_B(CS101_ASDU asdu, CS101_AppLayerParameters alParams);
            static void Preset_B(std::map<uint16_t, DTU::buffer> presetmap);
            static void Confirm_B(bool isConfirm);

        public:
            template<typename _type,typename ...Args>
            static void RemoteDispatchingDevice(bool &CurDevExec, std::vector<DTU::buffer> &remoteResult, 
                                            const std::vector<_type> &outfixvec, std::string funcname, Args... param)
            {
                // 所有装置都执行
                bool allDevExec = false;
                // 根据解析的点表读取定值
                if (DSYSCFG::instance().isPublic()) {
                    std::map<int, int> devnovec;
                    if (funcname != PARAM_FUNCTION_NAME)
                    {
                        if (outfixvec.size() == 0)
                            allDevExec = true;
                        else {
                            for(auto & item : outfixvec)
                            {
                                CA whereca;
                                if (IOAMap::instance()->whereIOAFrom(item, whereca)) {
                                    DTULOG(DTU_INFO, "远方访问CA[%d]", whereca);
                                    devnovec.insert({whereca, whereca});
                                }

                                // auto ret = DBManager::instance().whereFixFrom(item);
                                // if(ret.ok)
                                //     devnovec.insert({ret.devno, ret.devno});
                            }
                        }
                    }
                    else
                    {
                        devnovec = devStaticSave;
                    }


                    // 本机设备执行
                    auto ita = devnovec.find(0);
                    if (ita != devnovec.end())
                        CurDevExec = true;
                    
                    // 远方调度函数并获取本函数最终执行结果
                    for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
                    {
                        try
                        {
                            // 判断是否使用
                            if (item.second.use) {
                                auto ita = devnovec.find(item.second.ca);
                                // 判断该设备号是否在需求的设备号中
                                if (ita != devnovec.end() || allDevExec) {
                                    std::vector<DTU::buffer> temp = caller(item.second, funcname, param...);
                                    remoteResult.insert(remoteResult.end(), temp.begin(), temp.end());
                                    if (funcname == PARAM_FUNCTION_NAME)
                                        devStaticSave.insert({item.second.ca, item.second.ca});
                                }
                                else {
                                    continue;
                                }
                            }
                            else {
                                continue;
                            }
                        }
                        catch(const std::exception& e)
                        {
                            DTULOG(DTU_INFO, "公共单元调用间隔单元[%d]发生错误[%s][%d]:%s", item.second.ca,
                                item.second.ProtoRPC.ip.c_str(), item.second.ProtoRPC.port, e.what());
                        }
                    }
                }
                else {
                    CurDevExec = true;
                }
            }

        public:
            template<typename ...Args>
            static std::vector<DTU::buffer> caller(const DSYSCFG::OneBAYCFG& cfg, std::string callbackName, Args... param)
            {
                std::vector<DTU::buffer> result;
                try
                {
                    // 公共单元远程调用(只调用配置的间隔单元)
                    if(cfg.use) {
                        DTULOG(DTU_INFO, "远方请求[%s][%u]CA[%d]参数数量[%d]",cfg.ProtoRPC.ip.c_str(), cfg.ProtoRPC.port, cfg.ca, sizeof...(param));
                        rest_rpc::rpc_client client(cfg.ProtoRPC.ip, cfg.ProtoRPC.port);
                        client.set_connect_timeout(_RPC_TIME_OUT);
                        if (!client.connect()) {
                            DTULOG(DTU_ERROR,(char*)"caller() 连接失败");
                            return result;
                        }
                        result = client.call<std::vector<DTU::buffer>>(callbackName, param...);
                    }
                }
                catch(std::exception& e)
                {
                    DTULOG(DTU_ERROR, (char*)"间隔单元[%s]调用发生未知错误:%s", cfg.ProtoRPC.ip.c_str(), e.what());
                    return result;
                }

                return result;
            }

        public:
            template<typename _T, typename ...Args>
            static _T call(const DSYSCFG::OneBAYCFG& cfg, std::string callbackName, Args... param)
            {
                _T result;
                try
                {
                    // 公共单元远程调用(只调用配置的间隔单元)
                    if(cfg.use) {
                        DTULOG(DTU_INFO, "远方请求[%s][%u]CA[%d]参数数量[%d]",cfg.ProtoRPC.ip.c_str(), cfg.ProtoRPC.port, cfg.ca, sizeof...(param));
                        rest_rpc::rpc_client client(cfg.ProtoRPC.ip, cfg.ProtoRPC.port);
                        client.set_connect_timeout(_RPC_TIME_OUT);
                        if (!client.connect()) {
                            DTULOG(DTU_ERROR,(char*)"call() 连接失败");
                            return result;
                        }
                        result = client.call<_T>(callbackName, param...);
                    }
                }
                catch(std::exception& e)
                {
                    DTULOG(DTU_ERROR, (char*)"间隔单元[%s]调用发生未知错误:%s", cfg.ProtoRPC.ip.c_str(), e.what());
                    return result;
                }

                return result;
            }

            template<typename ...Args>
            static void call(const DSYSCFG::OneBAYCFG& cfg, std::string callbackName, Args... param)
            {
                try
                {
                    // 公共单元远程调用(只调用配置的间隔单元)
                    if(cfg.use) {
                        DTULOG(DTU_INFO, "远方请求[%s][%u]CA[%d]参数数量[%d]",cfg.ProtoRPC.ip.c_str(), cfg.ProtoRPC.port, cfg.ca, sizeof...(param));
                        rest_rpc::rpc_client client(cfg.ProtoRPC.ip, cfg.ProtoRPC.port);
                        client.set_connect_timeout(_RPC_TIME_OUT);
                        if (!client.connect()) {
                            DTULOG(DTU_ERROR,(char*)"call() 连接失败");
                        }
                        client.call<void>(callbackName, param...);
                    }
                }
                catch(std::exception& e)
                {
                    DTULOG(DTU_ERROR, (char*)"间隔单元[%s]调用发生未知错误:%s", cfg.ProtoRPC.ip.c_str(), e.what());
                }
            }

        private:
            // 这里是隐藏的真正的函数入口
            // 从本机读取定值并打包
            static std::vector<DTU::buffer> readParam(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo &rinfo);
            // 写定值
            static std::vector<DTU::buffer> writeParam(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo &rinfo);
            // 文件处理请求
            static std::vector<DTU::buffer> fileRequest(transerInfomation info, DTU::buffer &payload);
            static std::vector<DTU::buffer> changeCurrentGroup(transerInfomation info, DTU::buffer &payload);
            static std::vector<DTU::buffer> readCurrentGroup(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo rinfo);
            static std::vector<DTU::buffer> remoteCtrl(transerInfomation info, DTU::buffer &payload, RemoteCtrlInfo rinfo);
            static std::vector<DTU::buffer> timeCapture(transerInfomation info, DTU::buffer &payload);
            static std::vector<DTU::buffer> unknownTypeID(transerInfomation info, DTU::buffer &payload);

        public:
            static std::vector<CS101_ASDU> createHYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom);
            static std::vector<CS101_ASDU> createSYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& yxresult, int csfrom);

            static std::vector<CS101_ASDU> createYX(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& hyxresult, DTU::buffer& syxresult, int csfrom);
            static std::vector<CS101_ASDU> createYC(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, DTU::buffer& ycresult, DTU::buffer& PublicParam, int csfrom);
            static CS101_ASDU createCRC(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam);

            static CS101_ASDU createSOE(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, TELEGRAM_TYPE type, bool isDefault = true, bool isFrombay = false, int devno = 0);
            static CS101_ASDU createCOS(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam, const DTU::buffer& data, int csfrom, MEASURED_TYPE type, bool isDefault = true, bool isFrombay = false, int devno = 0);

            static std::vector<DTU::buffer> createCRCOnly(int32_t oa, int32_t ca, CS101_AppLayerParameters alParam);

        private:
            // 预设定值
            static std::vector<DTU::buffer> presetParam(transerInfomation info, DTU::buffer &data, RemoteCtrlInfo& rinfo);
            // 操作定值(撤销/写入)
            static std::vector<DTU::buffer> operateParam(transerInfomation info, DTU::buffer &data, RemoteCtrlInfo& rinfo);

            // 创建操作
            static CS101_AppLayerParameters createAlParams();
            static void deleteAlParams(CS101_AppLayerParameters param);
            static CS101_ASDU create_Write_SG_ACK(CS101_AppLayerParameters alParam, CS101_CauseOfTransmission cot, uint16_t group, uint8_t tag);
            static CS101_ASDU create_Directory_ACK(CS101_AppLayerParameters alParam, uint32_t dirID, const std::vector<DTU::buffer> &fileList, uint32_t &currentIndex, bool &bMore);
            static CS101_ASDU create_Readfile_ACK(CS101_AppLayerParameters alParam, uint32_t fileID, const std::string &fileName, uint32_t filesize, uint8_t result);
            static CS101_ASDU create_Readfile_CON_ACK(CS101_AppLayerParameters alParam, uint32_t fileID, const char *filecontent, uint32_t filesize, uint32_t &offset); 
            static CS101_ASDU create_Writefile_ACT(CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res, const std::string &fileName, uint32_t filesize);
            static CS101_ASDU create_Writefile_Complete(CS101_AppLayerParameters alParam, uint32_t fileID, uint8_t res, uint32_t pos);
            
            static void ParseReadParam(DTU::buffer &data, uint16_t& group, uint8_t& tag, std::vector<uint32_t>& fixvec, RemoteCtrlInfo &rinfo);
            static void ParseDirectory(DTU::buffer &data, uint32_t &dirID, std::string &dirName, uint8_t &flag, uint64_t &begin, uint64_t &end);
            static std::string ParseReadfile(DTU::buffer &data);
            static std::string ParseWritefileACT(DTU::buffer &data, uint32_t &fileID, uint32_t &filesize);
            static DTU::buffer ParseWritefileData(DTU::buffer &data, uint32_t &fileID, uint32_t &pos, uint8_t &more, uint32_t &size, uint8_t& mod);
            static int ParseRemoteCtrl(transerInfomation &info, DTU::buffer &data, uint16_t &operate, uint32_t &fix,  uint32_t &originfix, int &fixopet, bool &isexec);
            static uint64_t ParseCurrentTime(DTU::buffer &time);

            //遥测遥信相关
            // 创建遥信对象
            static InformationObject createTelegram(TELEGRAM_TYPE type, int32_t ioa, bool value, uint64_t time, QualityDescriptor q);
            // 创建遥测对象
            static InformationObject createMeasured(MEASURED_TYPE type, int32_t ioa, float value, uint64_t time, QualityDescriptor q);

            // 文件处理相关
            // 读取文件目录
            static std::vector<DTU::buffer> fileReqDirectory(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams);
            static std::vector<DTU::buffer> fileReqContent(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams);
            static void fileTransOK(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams);
            static std::vector<DTU::buffer> FileRecvComfirm(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams);
            static std::vector<DTU::buffer> FileRecvContent(transerInfomation info, DTU::buffer &data, CS101_AppLayerParameters alParams);

            // 将数据添加到返回队列 注:该函数会销毁ASDU
            static void addpayload(std::vector<DTU::buffer> &buff,CS101_ASDU asdu, bool autodestroy=true, bool isSequence = false);

            // 发送的数据解包
            static void unpack(const DTU::buffer &data, transerInfomation &info, DTU::buffer &payload);
        
        private:
            static std::map<int, int> devStaticSave;
    };
}

#undef _RPC_TIME_OUT
#endif /* _DTU_HAL_ASDU_H_ */