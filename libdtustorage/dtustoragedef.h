/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtustoragedef.h
  *Description: 
    定义相关数据结构
  *History: 
    1, 创建, wangjs, 2021-8-11
**********************************************************************************/
#ifndef _DTU_STORAGE_DEF_H
#define _DTU_STORAGE_DEF_H
#include <string>
namespace DTU
{
    typedef struct setting_db_config
    {
        uint32_t _group;        // 定值区总数
        uint32_t _groupsize;    // 定值区大小
        uint32_t _currentgroup; // 当前定值区号
    }STHeader;

    typedef struct param_db_cfg{
        uint32_t _id;
        std::string _fileName;      // 文件名
        uint32_t _default_size;     // 默认文件映射大小
        uint32_t _default_maxgroup;    // 最大区号
        uint32_t _default_groupsize;
    }DParCfg;

    typedef struct dtu_report_header
    {
        uint32_t _totleCount;   // 总条数
        uint32_t _currentIndex; // 当前的索引位置
        uint64_t _currentNo;    // 当前的序号
        uint64_t _countNo;      // 计数号 主要用于录波文件计数不超过9999
    }DReportHeader, DArchiveHeader;
    
    typedef struct dtu_report_cfg{
        uint16_t _id;
        std::string _fileName;      // 文件名
        uint32_t _item_size;     // 默认文件映射大小
        uint32_t _max_item;
    }DRptCfg;
};
#endif