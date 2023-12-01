/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description: 
    存储保护动作报告
  *History: 
    1, 创建, wangjs, 2021-8-10
**********************************************************************************/
#ifndef _DTU_PRO_ACT_ARC_H
#define _DTU_PRO_ACT_ARC_H
#include <dtubuffer.h>
#include <map>
#include <vector>
#include <mutex>
#include "dtuarchivestorage.h"
namespace DTU
{
    class DArchiveProtact : public DArchivesBase
    {
    public:
        DArchiveProtact(){}
    public:
        virtual void add_archives_data(const buffer& data) override;
        // 形成报告文件
        virtual void form_archives_file(const std::vector<buffer>& data) override;
    private:
        std::mutex _lock;
        // 临时存储保护动作报告
        std::map<uint64_t, std::vector<buffer>> _prot_act;
    };
};
#endif