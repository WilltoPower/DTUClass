/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturuleskit.cpp
  *Description:
    用于实现规约相关的通用功能
  *History:
    1, 创建, wangjs, 2021-8-31
**********************************************************************************/
#include "dturuleshandle.h"
#include <dtulog.h>
#include <dtuerror.h>
#include <sys/stat.h>
#include <string.h>
#include <dturulesfile.h>
#include <dtucommon.h>
//#include <rpc/client.h>
#include <rest_rpc/rest_rpc.hpp>
#include <dtusystemconfig.h>
#include <dtustorage.h>
#include <dtuprotocol.h>
#include <dtucmdcode.h>
#include <unistd.h>
#include <dtutask_dsp.h>
#include <dtutask_iec.h>
#include <dtustorage.h>
#include <dtutask_iec.h>
#include <dtuparamconfig.h>
#include <dtudbmanager.h>
#include <dtusystemconfig.h>

using namespace DTU;
using namespace DTUCFG;

#define RPC_TIME_OUT 5000

template<typename resulttype, typename ...Args>
int remote_request(resulttype& result, int32_t ca, std::string func, Args... param)
{
    try
    {
        bool ok = false;
        auto item = DSYSCFG::instance().GetUnitCFG(ca, ok);
        if(ok) {
            rest_rpc::rpc_client client(item.ProtoRPC.ip, item.ProtoRPC.port);
            client.set_connect_timeout(RPC_TIME_OUT);
            if (!client.connect()){
                DTULOG(DTU_ERROR,(char*)"remote_write 连接失败");
                return DTU_RPC_ERROR;
            }
            result = client.call<resulttype>(func, param...);
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, (char*)"remote_request 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

template<typename ...Args>
int remote_write(int32_t ca, const std::string& func, Args... param)
{
    try
    {
        bool ok = false;
        auto item = DSYSCFG::instance().GetUnitCFG(ca, ok);
        if(ok) {
            rest_rpc::rpc_client client(item.ProtoRPC.ip, item.ProtoRPC.port);
            client.set_connect_timeout(RPC_TIME_OUT);
            if (!client.connect()) {
                DTULOG(DTU_ERROR,(char*)"remote_write 连接失败");
                return DTU_RPC_ERROR;
            }
            return client.call<int>(func, param...);
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, (char*)"remote_request 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

DTU::buffer make_file_info_buffer(FILEINFO& sf)
{
    DTU::buffer result;
    // 文件名长度
    uint8_t fileNameSize = std::get<0>(sf).size();
    result.append((char*)&fileNameSize, sizeof(fileNameSize));
    // 文件名
    result.append(std::get<0>(sf).c_str(), fileNameSize);
    // 文件属性
    uint8_t attr = 0;
    result.append((char*)&std::get<3>(sf), sizeof(attr));
    // 文件大小
    result.append((char*)&(std::get<1>(sf)), sizeof(uint32_t));
    // 文件时间
    struct sCP56Time2a newTime;
    CP56Time2a_createFromMsTimestamp(&newTime, std::get<2>(sf));
    result.append((char*)newTime.encodedValue, 7);
    return std::move(result);
}

DTU::buffer get_file_info_buffer(const std::string& fileName)
{
    auto sf = get_file_info(fileName);
    DTU::buffer result;
    // 文件名长度
    uint8_t fileNameSize = fileName.size();
    result.append((char*)&fileNameSize, sizeof(fileNameSize));
    // 文件名
    result.append(fileName.c_str(), fileName.size());
    // 文件属性
    uint8_t attr = 0;
    result.append((char*)&std::get<3>(sf), sizeof(attr));
    // 文件大小
    result.append((char*)&std::get<1>(sf), sizeof(uint32_t));
    // 文件时间
    struct sCP56Time2a newTime;
    CP56Time2a_createFromMsTimestamp(&newTime, std::get<2>(sf));
    result.append((char*)newTime.encodedValue, 7);

    return std::move(result);
}
int DRULESReqHandle::read_file_directory(int32_t ca, const std::string& dir, 
    std::vector<DTU::buffer> &result, uint8_t flag, uint64_t tbegin,
            uint64_t tend)
{
    // 不使用时间
    if (flag == 0)
    {
        tbegin = 0;
        tend = 0;
    }
    FILELIST resultList;
    //=========================================================================//
    if (ca != DSYSCFG::instance().ASDU()) 
    {
        if (DSYSCFG::instance().isPublic()) 
        {
            // 公共单元向间隔单元读取
            remote_request<FILELIST>(resultList, ca, "rpc_get_dir", dir, tbegin, tend);
        } 
        else 
        {
            DTULOG(DTU_ERROR, (char *)"错误的地址:%u, 应该为%u", ca, DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    else
    {
        resultList = get_file_list(dir);
        if (flag == 1) 
        {
            auto ita = resultList.begin();
            while (ita != resultList.end()) 
            {
                if (std::get<2>(*ita) < tbegin || std::get<2>(*ita) > tend) 
                {
                    resultList.erase(ita);
                    continue;
                }
                ita++;
            }
        }
    }

    // 将路径添加到结果
    for(auto item : resultList)
    {
        result.emplace_back(make_file_info_buffer(item));
    }

    return DTU_SUCCESS;
}

int DRULESReqHandle::read_file_active(int32_t ca, const std::string& fileName, DTU::buffer& result)
{
    FILEINFO fileInfo;
    DTU::buffer fileContent;
    std::string fileFullName = get_exec_dir() + fileName;
    if (ca != DSYSCFG::instance().ASDU())
    {
        if (DSYSCFG::instance().isPublic())
        {
            // 公共单元向间隔单元读取
            remote_request<DTU::buffer>(fileContent, ca, "rpc_get_file", fileName);
        }
        else
        {
            DTULOG(DTU_ERROR, (char *)"错误的地址:%u, 应该为%u", ca, DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    else
    {
        get_file(fileFullName, fileContent);
    }
    // uint32_t fileID = get_current_seconds();
    std::string CRCFile = fileName;
    uint32_t fileID = crc32((uint8_t*)CRCFile.c_str(), CRCFile.size());

    // res 0:成功 1:失败
    uint8_t res = 0;
    uint8_t fileNameSize = fileName.size();
    // 获取文件信息
    {
        std::lock_guard<std::mutex> lock(_read_lock);
        _read_file_map[fileID] = fileContent;
    }

    result.remove();
    // 结果描述字
    result.append((char*)&res,sizeof(res));
    // 文件名字长度
    result.append((char*)&fileNameSize, sizeof(fileNameSize));
    // 文件名
    result.append(fileFullName.c_str(), fileFullName.size());
    // 文件ID
    result.append((char*)&fileID, sizeof(fileID));
    // 文件大小
    uint32_t filesize = fileContent.size();
    result.append((char*)&filesize, sizeof(filesize));

    return DTU_SUCCESS;
}

// 读取文件内容
int DRULESReqHandle::read_file_content(int32_t ca, uint32_t fileID, DTU::buffer& content)
{
    content.remove();
    std::lock_guard<std::mutex> lock(_read_lock);
    auto ita = _read_file_map.find(fileID);
    if (ita != _read_file_map.end())
    {
        if (ita->second.size()==0)
        {
            DTULOG(DTU_WARN, (char *)"read_file_content 文件:%u内容为空, 未激活", fileID);
            // 重新打开文件
            return DTU_UNKNOWN_ERROR;
        }
        content = ita->second;
    }
    else
    {
        DTULOG(DTU_WARN, (char *)"read_file_content 未找到文件:%u, 未激活", fileID);
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int DRULESReqHandle::read_file_content_ack(int32_t ca, uint32_t fileID, uint32_t offset, uint8_t result)
{
    std::lock_guard<std::mutex> lock(_read_lock);
    auto ita = _read_file_map.find(fileID);
    if (ita != _read_file_map.end())
    {
        DTULOG(DTU_INFO,(char*)"read_file_content_ack 读取文件%u结束,结果:%u,大小:%u", fileID, result, offset);
        _read_file_map.erase(ita);
    }
    else
    {
        DTULOG(DTU_ERROR,(char*)"read_file_content_ack 没有已经激活的文件:%u", fileID);
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}
int DRULESReqHandle::write_file_active(uint32_t ca, uint32_t fileID, const std::string& fileName, uint32_t filesize)
{
    std::string fileFullName = get_exec_dir() + fileName;
    {
        std::lock_guard<std::mutex> lock(_write_lock);
        auto ita = _write_file_map.find(fileID);
        if (ita != _write_file_map.end())
        {
            ita->second = std::make_tuple(fileFullName, DTU::buffer(filesize));
        }
        else
        {
            _write_file_map.insert({fileID, std::make_tuple(fileFullName, DTU::buffer(filesize))});
        }
    }
    return DTU_SUCCESS;
}
int DRULESReqHandle::write_file_content(uint32_t ca, int32_t fileID, uint32_t offset, uint8_t mod, DTU::buffer& content, uint8_t more)
{
    /*
    写文件首先将文件内容保存到缓存中,当文件传输完成时,写入本地或者一次发送给间隔单元
    */
    std::lock_guard<std::mutex> lock(_write_lock);
    auto ita = _write_file_map.find(fileID);
    if (ita == _write_file_map.end()){
        return 4;
    }
    std::string fileName = std::get<0>(ita->second);
    //
    try{
        uint8_t check = get_mod((uint8_t*)content.data(), content.size());
        if (check != mod){
            return 2;
        }
        std::get<1>(ita->second).set(offset, content);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_INFO, (char *)"write_file_content 保存文件错误:%s", e.what());
        return 3;
    }
    //
    int ret  = 0;
    if (more == 0){
        
        if (ca != DSYSCFG::instance().ASDU()) 
        {
            if (DSYSCFG::instance().isPublic()) {
                // 公共单元向间隔单元读取
                DTU::buffer& fileData = std::get<1>(ita->second);
                ret = remote_write(ca, "rpc_set_file", std::get<0>(ita->second), fileData);
            }
            else 
            {
                DTULOG(DTU_ERROR, (char *)"write_file_content 错误的地址:%u, 应该为%u", ca,
                    DSYSCFG::instance().ASDU());
                ret = 4;
            }
        }
        else{
            // 保存到本地
            ret = save_file(std::get<0>(ita->second), std::get<1>(ita->second));

            _write_file_map.erase(ita);
        }
    }
    return ret;
}
int DRULESReqHandle::read_current_group(uint32_t ca, DTU::buffer& result)
{
    // 最小区号
    uint16_t minno = 1;
    // 当前区号
    uint16_t currentno = 0;
    // 最大区号
    uint16_t maxno = 0;

    if (ca != DSYSCFG::instance().ASDU())
    {
        if (DSYSCFG::instance().isPublic()) {
            // 公共单元向间隔单元读取
            remote_request<DTU::buffer>(result, ca, "rpc_read_group");
            if (result.size() != 0){
                currentno = result.get(0, sizeof(currentno)).value<uint16_t>();
                maxno = result.get(4, sizeof(maxno)).value<uint16_t>();
            }

        } else {
            DTULOG(DTU_ERROR, (char *)"read_current_group 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    else {
        // 当前区号
        currentno = DTU::DSTORE::instance().get_current_group();
        // 最大区号
        maxno = DTU::DSTORE::instance().get_max_group();
    }

    result.remove();
    result.append(DTU::buffer(3));
    result.append((char*)&currentno, sizeof(currentno));
    result.append((char*)&minno, sizeof(minno));
    result.append((char*)&maxno, sizeof(maxno));

    return DTU_SUCCESS;
}
//
int DRULESReqHandle::change_current_group(uint32_t ca, uint32_t group, uint32_t current)
{
    if (ca != DSYSCFG::instance().ASDU()) {
        if (DSYSCFG::instance().isPublic()) {
            return remote_write(ca, "rpc_change_group", current, group);
        } else {
            DTULOG(DTU_ERROR, (char *)"change_current_group 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
   return iectask_execute_change(current, group);
}

int DRULESReqHandle::write_setting_preset(int32_t ca, const DTU::buffer& setInfo, uint16_t group)
{
    if (ca != DSYSCFG::instance().ASDU()) {
        if (DSYSCFG::instance().isPublic()) {
            return remote_write(ca, "rpc_preset_setting", setInfo, group);
        } else {
            DTULOG(DTU_ERROR, (char *)"write_setting_preset 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    // 选择定值区
    DTU::DSTORE::instance().seletc_edit_sg(group);
    // 预设值
    DTU::DSTORE::instance().preset_setting_data(setInfo);
    return DTU_SUCCESS;
}
// 撤销预置区域
int DRULESReqHandle::revert_setting_preset(int32_t ca)
{
    if (ca != DSYSCFG::instance().ASDU()) {
        if (DSYSCFG::instance().isPublic()) {
            return remote_write(ca, "rpc_revert_setting");
        } else {
            DTULOG(DTU_ERROR, (char *)"revert_setting_preset 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    DTU::DSTORE::instance().revert_setting_data();
    return DTU_SUCCESS;
}
// 保存预置区域
int DRULESReqHandle::save_setting_preset(int32_t ca)
{

    if (ca != DSYSCFG::instance().ASDU()) 
    {
        if (DSYSCFG::instance().isPublic()) {
            return remote_write(ca, "rpc_confirm_setting");
            //return remote_request(ca, "rpctask_preset_confirm");
        } else {
            DTULOG(DTU_ERROR, (char *)"save_setting_preset 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    try
    {
        return iectask_execute_confirm();
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, (char *)"save_setting_preset 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int DRULESReqHandle::read_setting(int32_t ca, uint32_t group, std::vector<uint32_t>& vecfix, std::vector<DTU::buffer>& result)
{
    // TOFIX CA转设备号
    // 根据fixid查找设备号

    // 设备号 + 定值表
    std::map<int, std::map<ParamID, std::tuple<std::vector<uint16_t>, DTU::buffer>>> DevSettingBuffer;

    //auto ret = DBManager::instance().whereFixFrom();

    // ParamID fixid组 本组定值数据
    std::map<ParamID, std::tuple<std::vector<uint16_t>, DTU::buffer>> settingBuffer;
    // 点表数组为空,获取全部定值信息
    if (vecfix.empty())
    {
        // 构建所有的定值的点表
        static std::vector<ParamID> ParamList = {
                ParamPublic,			// 公共定值
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
                ParamAutomation,		// 自动化参数
            };

        for(auto &item : ParamList)
        {
            std::vector<uint16_t> OneParamFixidAttr;
            for(auto &oneParam : DBManager::instance().GetParamInfoByID(item).info)
            {
                OneParamFixidAttr.emplace_back(oneParam.second.fixid);
            }
            settingBuffer[item] = std::make_tuple(OneParamFixidAttr, DTU::buffer());
        }
    }
    else
    {
        for(auto& item : vecfix)
        {
            uint16_t infixid = DBManager::instance().FixidMapOuttoin(item);
            uint16_t ParamId = DBManager::instance().GetParamIDByFixid(infixid);
            auto ita = settingBuffer.find(static_cast<ParamID>(ParamId));
            if(ita == settingBuffer.end())
            {
                std::vector<uint16_t> OneParamFixidAttr;
                OneParamFixidAttr.emplace_back(infixid);
                settingBuffer[static_cast<ParamID>(ParamId)] = std::make_tuple(OneParamFixidAttr, DTU::buffer());
            }
            else
            {
                std::get<0>(ita->second).emplace_back(infixid);
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////
    if (ca != DSYSCFG::instance().ASDU()) 
    {
        if (DSYSCFG::instance().isPublic()) {
            
            for(auto& item : settingBuffer)
            {
                auto cmd = DBManager::instance().Get_R_CMD_By_ParamID(item.first);
                DTU::buffer paramResult;
                remote_request<DTU::buffer>(paramResult, ca, "rpc_read_setting", cmd);
                std::get<1>(item.second) = paramResult;
            }
        } else {
            DTULOG(DTU_ERROR, (char *)"read_setting 错误的地址:%u, 应该为%u", ca,
                   DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    else
    {
        for(auto& item : settingBuffer)
        {
            auto cmd = DTU::DParamConfig::instance().get_read_cmd(item.first);
            DTU::buffer paramResult;
            // 直接从DSP中读取,而不是从数据库中读取
            if(item.first == ParamGroupNo) {
                int groupno = DBManager::instance().GetCurrGroup();
                paramResult.append((char*)&groupno,sizeof(groupno));
            }
            else {
                dsptask_execute_read(cmd, paramResult);
            }
            //DTU::DSTORE::instance().read_setting_data(item.first, paramResult, DTU::DSTORE::instance().get_current_group());
            std::get<1>(item.second) = paramResult;
        }
    }

    // 组织定值结构
    for(const auto& item : settingBuffer)
    {
        for(const auto& fixidItem : std::get<0>(item.second))
        {
            uint16_t val_type = DTU::DParamConfig::instance().get_value_type(fixidItem);
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
                    DTULOG(DTU_ERROR,"未知的Tag标识[%u]",val_type);
            }
            // 获取数据长度和偏移值
            uint8_t vlen = DTU::DParamConfig::instance().get_value_length(TABLE_PARAM, fixidItem);
            uint32_t offset = DTU::DParamConfig::instance().get_value_offset(TABLE_PARAM, fixidItem);
            // 获取buffer
            DTU::buffer value = std::get<1>(item.second).get(offset, vlen);
            // 构造数据
            DTU::buffer resultItem;

            uint16_t outfixid = 0;
            if (item.first == ParamAutomation)
                outfixid = DBManager::instance().FixidMapIntoout(MAP_AU, fixidItem);
            else
                outfixid = DBManager::instance().FixidMapIntoout(MAP_YT, fixidItem);

            resultItem.append((char*)&outfixid, sizeof(uint16_t));
            resultItem.append(DTU::buffer(1));// 点表偏移
            resultItem.append((char*)&Tag, sizeof(Tag));
            resultItem.append((char*)&vlen, sizeof(vlen));
            resultItem.append(value);
            //
            result.emplace_back(std::move(resultItem));
        }
    }
}

// 遥控
int DRULESReqHandle::remote_control(int32_t ca,uint16_t fix,uint16_t operate,RemoteCtrlInfo info)
{
    if (ca != DSYSCFG::instance().ASDU())
    {
        if (DSYSCFG::instance().isPublic())
        {
            return remote_write(ca, "rpc_rmctrl", fix, operate, info);
        }
        else
        {
            DTULOG(DTU_ERROR, (char *)"remote_ctrl 错误的地址:%u, 应该为%u", ca,
                DSYSCFG::instance().ASDU());
            return DTU_UNKNOWN_ERROR;
        }
    }
    //////////////////////////////////////////////////
    return iectask_execute_control(fix,operate,info);
}
