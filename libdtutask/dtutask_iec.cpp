#include "dtutask_iec.h"
#include "dtutask_dsp.h"
#include <map>
#include <dtucommon.h>
#include <functional>
#include <dtuerror.h>
#include <dtuprotocol.h>
#include <dtucmdcode.h>
#include <dtustorage.h>
#include <dtulog.h>
#include <bitset>
#include <dtuparamconfig.h>

int iectask_execute_change(uint32_t current, uint32_t dest)
{
    try
    {
        if (!DTU::DSTORE::instance().change_setting_num(dest,dsptask_execute_write))
            return DTU_UNKNOWN_ERROR;
        else
            return DTU_SUCCESS;
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_change 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int iectask_execute_query(uint16_t cmd, DTU::buffer& result)
{
    try
    {
        result.remove();
        // 
        if (DTU_SUCCESS != dsptask_execute_query(cmd, result))
        {
            auto id = DTU_GET_PARAM_ID(cmd); //DTUCFG::PARAMCfg::instance().get_param_id(cmd);
            result.resize(DTU::DParamConfig::instance().get_param_length(id));
            return DTU_DSP_ERROR;
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_query 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    
    return DTU_SUCCESS;
}

int iectask_execute_read(uint16_t cmd, uint32_t group, DTU::buffer& result)
{
    try
    {
        result.remove();
        // 
        if (group != DTU::DSTORE::instance().get_current_group()){
            // 不是当前区,直接从存储中获取
            auto id = DTU_GET_PARAM_ID(cmd);
            DTU::DSTORE::instance().read_setting_data(id, result, group);
        }
        else{
            if (DTU_SUCCESS != dsptask_execute_read(cmd, result))
            {
                auto id = DTU_GET_PARAM_ID(cmd);
                result.resize(DTU::DParamConfig::instance().get_param_length(id));
                return DTU_DSP_ERROR;
            }
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_read 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    
    return DTU_SUCCESS;
}

int iectask_execute_write(uint16_t cmd, uint32_t group, const DTU::buffer& value, bool isReboot)
{
    uint16_t reboot = 1;
    if(!isReboot)
    {
        reboot = 0;
    }
    try
    {
        if (group == DTU::DSTORE::instance().get_current_group()){
            if(DTU_SUCCESS != dsptask_execute_write(cmd, value, reboot))
            {
                DTU_USER();
                DTU_THROW((char*)"dsptask_execute_write 定值写入失败");
            }
        }
        DTU::DSTORE::instance().write_setting_data(GET_PARAM_ID_BY_CMD(cmd), value, group);
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_write 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int iectask_execute_control(uint16_t fix,uint16_t operate,RemoteCtrlInfo info)
{
    try
    {
        if (DTU_SUCCESS != dsptask_execute_rmctrl(fix,operate,info))
        {
            return DTU_DSP_ERROR;
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_control 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

// 确认定值预设
int iectask_execute_confirm()
{
    DTU::DSTORE::instance().save_presetting_data(dsptask_execute_write);
    return DTU_SUCCESS;
}

int iectask_execute_time(DTU::buffer& result)
{
    try
    {
        if (DTU_SUCCESS != dsptask_execute_write(PC_W_CLK,result))
        {
            return DTU_DSP_ERROR;
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_execute_time 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

int iectask_read_time(DTU::buffer& result)
{
    try
    {
        if (DTU_SUCCESS != dsptask_execute_read(PC_R_CLK,result))
        {
            return DTU_DSP_ERROR;
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"iectask_read_time 未知错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}