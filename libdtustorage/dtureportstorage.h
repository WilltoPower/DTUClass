/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description: 
    基于文件映射的参数定值存储系统
  *History: 
    1, 创建, wangjs, 2021-8-9
    2, 加入读取报告全部内容的功能, wangjs, 2021-8-19
**********************************************************************************/
#ifndef _DTU_REPORT_STORAGE_H
#define _DTU_REPORT_STORAGE_H
#include <string>
#include <mutex>
#include <dtuerror.h>
#include <mio/mmap.hpp>
#include <unordered_map>
#include <string.h>
#include <dtubuffer.h>
#include "dtustoragedef.h"
namespace DTU
{
    // 存储报告的文件映射数据库
    class DReportStorage
    {
    public:
        // 加载
        bool load(const DRptCfg& cfg);
        // 获取当前条目
        buffer get_current_item();
        // 按序号读取报告
        buffer get_item_by_no(uint64_t no);
        // 按索引读取报告
        buffer get_item_by_index(uint32_t index);
        // 添加报告
        void add_item(const buffer& rpt);
        // 获取单个报告长度
        uint32_t get_item_size();
        // 清空报告
        void clear();
        //
        void save_header();
		//
		DReportHeader get_header();
        // 
        uint32_t get_index_by_no(uint64_t serialno);
        // 刷新报告内容
        void flush(const buffer& data);
        // 获取全部内容
        void get_content(DTU::buffer& data);
    private:
        std::mutex     _rw_lock;
        mio::mmap_sink _rw_mmap;
        // 报告头
        DReportHeader  _header;
        //
        DRptCfg _cfg;
    };
};
#endif