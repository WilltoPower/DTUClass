/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivestorage.h
  *Description: 
    生成文件类的档案基类, 用于处理录波，保护动作报告等
  *History: 
    1, 创建, wangjs, 2021-8-11
    2, 加入档案头, wangjs, 2021-8-25
**********************************************************************************/
#ifndef _DTU_ARCHIVE_STORAGE_H
#define _DTU_ARCHIVE_STORAGE_H
#include <dtubuffer.h>
#include <mutex>
namespace DTU
{
    class DArchivesBase
    {
    public:
        // 添加录波报告
        virtual void add_archives_data(const buffer& data) {}
        // 形成报告文件
        virtual void form_archives_file(const std::vector<buffer>& data) {}
    };
}; // namespace DTU
#endif