/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivestransrcd.h
  *Description: 
    暂态文件档案
  *History: 
    1, 创建, wangjs, 2021-8-25
**********************************************************************************/
#ifndef _DTU_ARCHIVES_TRANS_RCD_H
#define _DTU_ARCHIVES_TRANS_RCD_H
#include "dtuarchivestorage.h"
#include "dturecorder.h"
#include <dtustructs.h>
#include <map>
#include <dtubuffer.h>
namespace DTU
{
    class DArchiveTransRcd : public DArchivesBase
    {
    public:
        DArchiveTransRcd(){}
    public:
        virtual void add_archives_data(const buffer& data) override;
        // 形成报告文件
        virtual void form_archives_file(const std::vector<buffer>& data) override;
    private:
        void create_cfg(std::string fileName, const std::vector<DTransRcd>& data, DTUParamAdjust adjParam);
        void create_dat(std::string fileName, const std::vector<DTransRcd>& data);
    private:
        std::mutex _lock;
        // 临时存储保护动作报告
        std::map<uint64_t, std::vector<buffer>> _trans_rcd;
    };
};
#endif 