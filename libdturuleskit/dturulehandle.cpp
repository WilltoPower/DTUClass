#include "dturulehandle.h"

#include <lib60870/cs104_connection.h>

#include <rest_rpc/rest_rpc.hpp>

#include "dtuparamconfig.h"
#include "dtudbmanager.h"
#include "dtutask_dsp.h"
#include "dtutask_iec.h"
#include "dtustorage.h"
#include "dtulog.h"
#include "dutxmlgenerate.h"
#include "dtusystemconfig.h"
#include "dtuioamap.h"

using namespace DTU;
using namespace DTUCFG;

// 读锁
std::mutex dtuRuleHandler::_read_lock;
// 写锁
std::mutex dtuRuleHandler::_write_lock;
// 保存读文件的Handle
std::map<uint32_t, DTU::buffer> dtuRuleHandler::_read_file_map;
// 保存写文件的Handle
std::map<uint32_t, std::tuple<std::string, DTU::buffer>> dtuRuleHandler::_write_file_map;

int dtuRuleHandler::readParam(uint16_t &group, std::vector<uint32_t> &vecfix, std::vector<DTU::buffer> &result, RemoteCtrlInfo &rinfo)
{
    // ParamID fixid组 本组定值数据
    std::map<ParamID, std::tuple<std::vector<uint16_t>, DTU::buffer>> settingBuffer;
    group = DBManager::instance().GetCurrGroup();
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
                // 此处添加的为内部点表的值(硬件点表值)
                OneParamFixidAttr.emplace_back(oneParam.second.fixid);
            }
            settingBuffer[item] = std::make_tuple(OneParamFixidAttr, DTU::buffer());
        }
    }
    else
    {
        for(auto& item : vecfix)
        {
            HIOA hioa = 0x0000;
            // 将外部IOA映射为内部HIOA
            if (!IOAMap::instance()->mapIOAtoHIOA(item, hioa, DSYSCFG::instance().devno)) {
                // 如果无法映射则不进行查询,跳过本次查询
                continue;
            }

            // 这里infixid为内部硬件ID
            uint16_t infixid = DBManager::instance().FixidMapOuttoin(hioa);

            // // 如果点表不属于本设备则跳过本次循环
            // if (!DBManager::instance().testFixidBelongCurDevice(item))
            //     continue;

            uint16_t ParamId = DBManager::instance().GetParamIDByFixid(infixid);
            auto ita = settingBuffer.find(static_cast<ParamID>(ParamId));
            if(ita == settingBuffer.end()) {
                std::vector<uint16_t> OneParamFixidAttr;
                OneParamFixidAttr.emplace_back(infixid);
                settingBuffer[static_cast<ParamID>(ParamId)] = std::make_tuple(OneParamFixidAttr, DTU::buffer());
            }
            else {
                std::get<0>(ita->second).emplace_back(infixid);
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////

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

        std::get<1>(item.second) = paramResult;
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
                    DTULOG(DTU_ERROR,"未知的Tag标识[%u]", val_type);
            }
            // 获取数据长度和偏移值
            uint8_t vlen = DTU::DParamConfig::instance().get_value_length(TABLE_PARAM, fixidItem);
            uint32_t offset = DTU::DParamConfig::instance().get_value_offset(TABLE_PARAM, fixidItem);
            // 获取buffer
            DTU::buffer value = std::get<1>(item.second).get(offset, vlen);
            // 构造数据
            DTU::buffer resultItem;
            // 将点表从内部映射回外部
            // fixidItem为内部硬件点表
            uint16_t outfixid = DBManager::instance().FixidMapIntoout(MAP_YT, fixidItem);

            IOA ioa = 0x0000;
            if (!IOAMap::instance()->mapHIOAtoIOA(outfixid, ioa, DSYSCFG::instance().devno)) {
                DTULOG(DTU_WARN, "HIOA [0x%04X]未找到对应IOA值", outfixid);
                continue;
            }

            int IOALength = 0;
            if (rinfo.cmdFrom == RC_CMD_101) {
                IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
            }
            else {
                IOALength = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
            }

            if (IOALength == 3) {
                resultItem.append((char*)&ioa, sizeof(uint16_t));
                resultItem.append(DTU::buffer(1));// 点表偏移
            }
            else {
                resultItem.append((char*)&ioa, IOALength);
            }

            resultItem.append((char*)&Tag, sizeof(Tag));
            resultItem.append((char*)&vlen, sizeof(vlen));
            resultItem.append(value);
            //
            result.emplace_back(resultItem);
        }
    }
}

int dtuRuleHandler::PresetParam(const DTU::buffer &setInfo, uint16_t group)
{
    // 选择定值区
    DTU::DSTORE::instance().seletc_edit_sg(group);
    // 预设值
    return DTU::DSTORE::instance().preset_setting_data(setInfo) ? DTU_SUCCESS : DTU_INVALID_CMD;
}

int dtuRuleHandler::revertPreset()
{
    DTU::DSTORE::instance().revert_setting_data();
    return DTU_SUCCESS;
}

int dtuRuleHandler::savePreset()
{
    try
    {
        return iectask_execute_confirm();
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, (char *)"savePreset()发生未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int dtuRuleHandler::readFileDirectory(const std::string &dir, std::vector<DTU::buffer> &result, uint8_t flag, uint64_t tbegin, uint64_t tend)
{
    // 不使用时间
    if (flag == 0) {
        tbegin = 0;
        tend = 0;
    }
    FILELIST resultList;

    resultList = get_file_list(dir);

    if (flag == 1)  {
        auto ita = resultList.begin();
        while (ita != resultList.end())
        {
            if (std::get<2>(*ita) < tbegin || std::get<2>(*ita) > tend) {
                resultList.erase(ita);
                continue;
            }
            ita++;
        }
    }

    // 将路径添加到结果
    for(auto item : resultList) {
        result.emplace_back(MakeFileinfoToBuffer(item));
    }

    return DTU_SUCCESS;
}

// 提取文件名称
static std::string ExtractFileName(const std::string &fileName)
{
    std::string result;
    auto pos = fileName.rfind('/');
    if (pos != std::string::npos)
        result = fileName.substr(pos+1);
    else
        result = fileName;
    return result;
}

int dtuRuleHandler::readFileActive(const std::string &fileName, DTU::buffer &result)
{
    FILEINFO fileInfo;
    DTU::buffer fileContent;
    std::string fileFullName = get_exec_dir() + fileName;

    std::string purefileName = ExtractFileName(fileName);

    if (fileName.find("ulog.xml") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/ULOG/" + purefileName;
    }
    else if(fileName.find("soe.xml") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/SOE/" + purefileName;
    }
    else if(fileName.find("frz") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/FRZ/" + purefileName;
    }
    else if(fileName.find("flowrev.xml") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/FLOWREV/" + purefileName;
    }
    else if(fileName.find("fixpt") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/FIXPT/" + purefileName;
    }
    else if(fileName.find("exv") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/EXV/" + purefileName;
    }
    else if(fileName.find("co.xml") != std::string::npos) {
        fileFullName = get_exec_dir() + "/HISTORY/CO/" + purefileName;
    }
    else if((fileName.find(".dat") != std::string::npos) || (fileName.find(".cfg") != std::string::npos)) {
        fileFullName = get_exec_dir() + "/COMTRADE/" + purefileName;
    }

    DTULOG(DTU_INFO, (char*)"规约远程读取文件:%s", fileFullName.c_str());

    get_file(fileFullName, fileContent);

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
    result.append(fileName.c_str(), fileName.size());
    // 文件ID
    result.append((char*)&fileID, sizeof(fileID));
    // 文件大小
    uint32_t filesize = fileContent.size();
    result.append((char*)&filesize, sizeof(filesize));

    return DTU_SUCCESS;
}

int dtuRuleHandler::readFileContent(uint32_t fileID, DTU::buffer &content)
{
    content.remove();
    std::lock_guard<std::mutex> lock(_read_lock);
    auto ita = _read_file_map.find(fileID);
    if (ita != _read_file_map.end()) {
        if (ita->second.size()==0)
        {
            DTULOG(DTU_WARN, (char *)"readFileContent() 文件:%u内容为空, 未激活", fileID);
            // 重新打开文件
            return DTU_UNKNOWN_ERROR;
        }
        content = ita->second;
    }
    else {
        DTULOG(DTU_WARN, (char *)"readFileContent() 未找到文件:%u, 未激活", fileID);
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int dtuRuleHandler::writeFileActive(uint32_t fileID, const std::string &fileName, uint32_t filesize)
{
    std::string fileFullName = get_exec_dir() + fileName;
    {
        std::lock_guard<std::mutex> lock(_write_lock);
        auto ita = _write_file_map.find(fileID);
        if (ita != _write_file_map.end())
            ita->second = std::make_tuple(fileFullName, DTU::buffer(filesize));
        else
            _write_file_map.insert({fileID, std::make_tuple(fileFullName, DTU::buffer(filesize))});
        DTULOG(DTU_INFO, "准备接收文件[%s]", fileName.c_str());
    }
    return DTU_SUCCESS;
}

int dtuRuleHandler::writeFileContent(int32_t fileID, uint32_t offset, uint8_t mod, DTU::buffer &content, uint8_t more)
{
    /*
    写文件首先将文件内容保存到缓存中,当文件传输完成时,一次写入本地
    */
    std::lock_guard<std::mutex> lock(_write_lock);
    auto ita = _write_file_map.find(fileID);
    if (ita == _write_file_map.end()) {
        return 4;
    }
    std::string fileName = std::get<0>(ita->second);
    //
    try
    {
        uint8_t check = get_mod((uint8_t*)content.data(), content.size());
        if (check != mod) {
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
    if (more == 0) {
        // 保存到本地
        ret = save_file(fileName, std::get<1>(ita->second));
        DTULOG(DTU_INFO, "规约传输文件[%s]完成,文件大小[%u]", fileName.c_str(), std::get<1>(ita->second).size());
        if (fileName.find("GooseConfigPlus.xml") != std::string::npos) {
            DTULOG(DTU_INFO, "发现GooseConfigPlus.xml GOOSE总配置文件准备进行GOOSE配置");
            dtuGooseCFGPlus cfgplus;
            if (!cfgplus.load(get_exec_dir() + "/update/rulekit/GooseConfigPlus.xml")) {
                DTULOG(DTU_ERROR, "GooseConfigPlus.xml GOOSE总配置文件加载错误");
                goto DTU_RULE_WRITE_FILE;
            }
            auto &localcfg = DSYSCFG::instance().ModifyGooseCFG();
            if (DSYSCFG::instance().isPublic()) {
                // 公共单元才进行操作,间隔单元不进行操作
                auto ita= cfgplus.config().find(0);
                if (ita != cfgplus.config().end()) {
                    // 公共单元配置
                    localcfg.appid = cfgplus.config()[0].appid;
                    localcfg.mac = cfgplus.config()[0].mac;
                    // 网卡配置
                    localcfg.ineth = cfgplus.config()[0].ineth;
                    localcfg.outeth = cfgplus.config()[0].outeth;
                    // 是否使用
                    // M侧赋值
                    for (int i=0;i<3;i++)
                    {
                        localcfg.mside[i].appid = cfgplus.config()[0].mside[i].appid;
                        localcfg.mside[i].use = cfgplus.config()[0].mside[i].use;
                    }
                    // N侧赋值
                    for (int i=0;i<3;i++)
                    {
                        localcfg.nside[i].appid = cfgplus.config()[0].nside[i].appid;
                        localcfg.nside[i].use = cfgplus.config()[0].nside[i].use;
                    }
                    // 网卡赋值
                    // 公共单元保存到本机配置
                    DSYSCFG::instance().save_to_file(localcfg, false);
                }

                // 配置间隔单元分发配置
                for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
                {
                    DTULOG(DTU_INFO, "间隔单元[%s]%s", item.second.use ? "启用" : "未启用");
                    if (item.second.use) {
                        auto cfgita = cfgplus.config().find(item.second.ca);
                        if (cfgita != cfgplus.config().end()) {
                        try
                        {
                            DTULOG(DTU_INFO, "公共单元修改[%s][%u]Goose配置", 
                            item.second.ProtoRPC.ip.c_str(), item.second.ProtoRPC.port);
                            rest_rpc::rpc_client client(item.second.ProtoRPC.ip, item.second.ProtoRPC.port);
                            client.set_connect_timeout(50);
                            if (!client.connect()) {
                                DTULOG(DTU_ERROR,(char*)"caller() 连接失败");
                                continue;
                            }

                            auto remoteGooseCFG = client.call<DTUCFG::DSYSCFG::GooseCFG>("rpc_read_goose_cfg");

                            // 公共单元配置
                            remoteGooseCFG.appid = cfgita->second.appid;
                            remoteGooseCFG.mac = cfgita->second.mac;
                            // 网卡配置
                            remoteGooseCFG.ineth = cfgita->second.ineth;
                            remoteGooseCFG.outeth = cfgita->second.outeth;
                            // 是否使用
                            // M侧赋值
                            for (int i=0;i<3;i++)
                            {
                                remoteGooseCFG.mside[i].appid = cfgita->second.mside[i].appid;
                                remoteGooseCFG.mside[i].use = cfgita->second.mside[i].use;
                            }
                            // N侧赋值
                            for (int i=0;i<3;i++)
                            {
                                remoteGooseCFG.nside[i].appid = cfgita->second.nside[i].appid;
                                remoteGooseCFG.nside[i].use = cfgita->second.nside[i].use;
                            }

                            bool ret = client.call<bool>("rpc_save_goose_cfg", remoteGooseCFG);
                            if (!ret)
                                DTULOG(DTU_ERROR, "规约向间隔单元保存GOOSE失败");
                        }
                        catch(std::exception& e)
                        {
                            DTULOG(DTU_ERROR, (char*)"间隔单元[%s]调用发生未知错误:%s", item.second.ProtoRPC.ip.c_str(), e.what());
                            continue;
                        }
                        }
                    }
                }
            }
        }
// 退出程序
DTU_RULE_WRITE_FILE:
        // 擦除缓存
        _write_file_map.erase(ita);
    }
    return ret;
}

int dtuRuleHandler::changeCurrentGroup(uint32_t group, uint32_t current)
{
   return iectask_execute_change(current, group);
}

int dtuRuleHandler::readCurrentGroup(DTU::buffer &result, RemoteCtrlInfo& rinfo)
{
    result.remove();
    // 最小区号
    uint16_t minno = 1;
    // 当前区号
    uint16_t currentno = DTU::DSTORE::instance().get_current_group();
    // 最大区号
    uint16_t maxno = DTU::DSTORE::instance().get_max_group();

    result.remove();

    int ioasize = 3;
    if (rinfo.cmdFrom == RC_CMD_101) {
        ioasize = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA;
    }
    else {
        ioasize = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
    }

    result.append(DTU::buffer(ioasize));
    result.append((char*)&currentno, sizeof(currentno));
    result.append((char*)&minno, sizeof(minno));
    result.append((char*)&maxno, sizeof(maxno));

    return DTU_SUCCESS;
}

int dtuRuleHandler::remoteControl(uint16_t fix, uint16_t operate, RemoteCtrlInfo info)
{
    return iectask_execute_control(fix,operate,info);
}

int dtuRuleHandler::queryTime(DTU::buffer &result)
{
    return iectask_read_time(result);
}

/********************************************************* 内部函数 ***********************************************************/

DTU::buffer dtuRuleHandler::MakeFileinfoToBuffer(FILEINFO& sf)
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

