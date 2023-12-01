/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuparamconfig.h
  *Description: 
    DTU的所有参数定植相关的配置
  *History: 
    1, 创建, wangjs, 2021-12-29
**********************************************************************************/
#ifndef _DTU_PARAM_CONFIG_H
#define _DTU_PARAM_CONFIG_H
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>

#define TABLE_INFOM 0x00    // infomation表
#define TABLE_PARAM 0x01    // Param定值表


namespace DTU{
    class DParamConfig
    {
        private:
            DParamConfig(){}
        public:
            static DParamConfig& instance(){
                static DParamConfig config;
                return config;
            }
        
        public:
            // 通过cmd获取ParamID
            uint16_t get_param_id_from_cmd(uint16_t cmd);
            // 获取参数的读命令
            uint16_t get_read_cmd(uint16_t pid);
            // 获取参数的写命令
            uint16_t get_write_cmd(uint16_t pid);
            // 按ParamID读取该定值整体长度
            uint32_t get_param_length(uint16_t pid);
            // 按InfoID读取info长度
            uint32_t get_info_length(uint16_t infoid);
        public:
            // 读参数值点号
            uint16_t get_value_fix(std::string desc);
            // 读取值长度
            uint32_t get_value_length(uint16_t whichTable, uint16_t fixid);
            // 读取值偏移
            uint32_t get_value_offset(uint16_t whichTable, uint16_t fixid);

            // 读参数值类型
            uint16_t get_value_type(uint16_t fixid);
    };
};

#define DTU_GET_PARAM_ID(cmd) DTU::DParamConfig::instance().get_param_id_from_cmd(cmd)

#endif