/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivesdb.h
  *Description: 
    档案管理系统,用于处理录波数据，保护动作报告等需要生成文件的信息
  *History: 
    1, 创建, wangjs, 2021-8-11
**********************************************************************************/
#ifndef _DTU_ARCHIVES_DB_H
#define _DTU_ARCHIVES_DB_H
#include <map>
#include <dtubuffer.h>
#include "dtuarchivestorage.h"
namespace DTU
{
    class ARCHIVEDB
    {
    public:
        static ARCHIVEDB& instance(){
            static ARCHIVEDB arc;
            return arc;
        }
        //
        void add_archives(uint16_t id, const DTU::buffer& data);
    private:
        ARCHIVEDB();
    private:
        std::map<uint16_t, std::unique_ptr<DTU::DArchivesBase>> _arc_db;
    };
};
#endif