#include "dtuparamconfig.h"
#include <pugixml/pugixml.hpp>
#include <dtucmdcode.h>
#include <map>

#include "dtudbmanager.h"

using namespace DTU;

uint16_t DParamConfig::get_param_id_from_cmd(uint16_t cmd) {
    return DTU::DBManager::instance().Get_ParamID_By_CMD(cmd);
}

// 获取参数的读命令
uint16_t DParamConfig::get_read_cmd(uint16_t pid) {
    return DTU::DBManager::instance().Get_R_CMD_By_ParamID(pid);
}

// 获取参数的写命令
uint16_t DParamConfig::get_write_cmd(uint16_t pid) {
    return DTU::DBManager::instance().Get_W_CMD_By_ParamID(pid);
}

// 按ParamID读取该定值整体长度
uint32_t DParamConfig::get_param_length(uint16_t pid) {
    return DTU::DBManager::instance().GetParamInfoByID(pid).size;
}

uint32_t DParamConfig::get_info_length(uint16_t infoid) {
    return DTU::DBManager::instance().GetInfomationTableByIndex(infoid).size;
}

// 读参数值点号
uint16_t DParamConfig::get_value_fix(std::string desc) {
    return DBManager::instance().GetInfomFixidByDesc(desc);
}

// 读取值长度
uint32_t DParamConfig::get_value_length(uint16_t whichTable, uint16_t fixid)
{
    switch(whichTable)
    {
        // 信息查看表
        case TABLE_INFOM: {
            return DBManager::instance().GetOneInfoItemByFixid(fixid).size;
        };break;
        // 定值表
        case TABLE_PARAM: {
            return DBManager::instance().GetOneParamInfoByFix(fixid).size;
        };break;
        default:
            return 0;
    }
}

// 读取值偏移
uint32_t DParamConfig::get_value_offset(uint16_t whichTable, uint16_t fixid)
{
    switch(whichTable)
    {
        // 信息查看表
        case TABLE_INFOM: {
            return DBManager::instance().GetOneInfoItemByFixid(fixid).offset;
        };break;
        // 定值表
        case TABLE_PARAM: {
            return DBManager::instance().GetOneParamInfoByFix(fixid).offset;
        };break;
        default:
            return 0;
    }
}

// 读参数值类型
uint16_t DParamConfig::get_value_type(uint16_t fixid)
{
    return DBManager::instance().GetOneParamInfoByFix(fixid).type;
}
