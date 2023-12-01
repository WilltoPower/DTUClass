/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturuleskit.h
  *Description:
    用于实现规约相关的通用功能
  *History:
    1, 创建, wangjs, 2021-8-31
**********************************************************************************/
#ifndef _DTU_RULES_KIT_H
#define _DTU_RULES_KIT_H
#include <lib60870/iec60870_common.h>

#include <vector>
#include <string>
#include <mutex>
#include <dtubuffer.h>
#include <dtustructs.h>
#include "dtucommon.h"

namespace DTU
{
    class DRULESReqHandle
    {
        private:
            DRULESReqHandle(){}
        public:
            static DRULESReqHandle& instance(){
                static DRULESReqHandle handler;
                return handler;
            }
            // 读取文件目录
            int read_file_directory(int32_t ca, const std::string& dir, std::vector<DTU::buffer> &result, uint8_t flag, uint64_t tbegin = 0,
                        uint64_t tend = 0);
            // 读取文件激活 
            int read_file_active(int32_t ca, const std::string& fileName, DTU::buffer& result);
            // 读取文件内容
            int read_file_content(int32_t ca, uint32_t fileID, DTU::buffer& content);
            // 读取文件内容传输确认
            int read_file_content_ack(int32_t ca, uint32_t fileID, uint32_t offset, uint8_t result);

            // 写文件激活
            int write_file_active(uint32_t ca, uint32_t fileID, const std::string& fileName, uint32_t filesize);
            // 写文件数据
            int write_file_content(uint32_t ca, int32_t fileID, uint32_t offset, uint8_t mod, DTU::buffer& content, uint8_t more);

            // 读取当前定值区号
            int read_current_group(uint32_t ca, DTU::buffer& result);
            // 切换定值区
            int change_current_group(uint32_t ca, uint32_t group, uint32_t current = 0);

            // 预设定值
            int write_setting_preset(int32_t ca, const DTU::buffer& setInfo, uint16_t group);
            // 撤销预置区域
            int revert_setting_preset(int32_t ca);
            // 保存预置区域
            int save_setting_preset(int32_t ca);

            // 读取定值
            int read_setting(int32_t ca, uint32_t group, std::vector<uint32_t>& vecfix, std::vector<DTU::buffer>& result);
            // 遥控
            int remote_control(int32_t ca,uint16_t fix,uint16_t operate,RemoteCtrlInfo info);
        private:
            std::mutex _write_lock;
            // 保存写文件的Handle
            std::map<uint32_t, std::tuple<std::string,DTU::buffer>> _write_file_map;

            std::mutex _read_lock;
            // 保存读文件的Handle
            std::map<uint32_t, DTU::buffer> _read_file_map; 
    };
};

#endif
