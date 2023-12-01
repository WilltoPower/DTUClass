/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivesworkrcd.h
  *Description: 
    业务录波档案
  *History: 
    1, 创建, wangjs, 2021-8-26
**********************************************************************************/
#ifndef _DTU_ARCHIVES_WORK_RCD_H
#define _DTU_ARCHIVES_WORK_RCD_H
#include "dtuarchivestorage.h"
#include "dturecorder.h"
#include <dtustructs.h>
#include <map>
#include <dtubuffer.h>
namespace DTU
{
    class DArchiveWorkRcd : public DArchivesBase
    {
    public:
        DArchiveWorkRcd(){}
    public:
        virtual void add_archives_data(const buffer& data) override;
        // 形成报告文件
        virtual void form_archives_file(const std::vector<buffer>& data) override;
    private:
        void create_cfg(std::string fileName, DTUParamAdjust adjParam);
        void create_dat(std::string fileName);
    private:
        std::mutex _lock;
        std::vector<buffer> _work_rcd;
    };
};
#endif