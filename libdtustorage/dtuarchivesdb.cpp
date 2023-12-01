/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivesdb.cpp
  *Description: 
    档案管理系统,用于处理录波数据，保护动作报告等需要生成文件的信息
  *History: 
    1, 创建, wangjs, 2021-8-11
    2, 加入暂态录波功能, wangjs, 2021-8-25
    3, 加入业务录波功能, wangjs, 2021-8-26
**********************************************************************************/
#include "dtuarchivesdb.h"
#include "json/json.h"
#include <dtucmdcode.h>
#include <dtulog.h>
#include <dtucommon.h>
#include <fstream>
#include "dtuarchivesproact.h"
#include "dtuarchivestransrcd.h"
#include "dtuarchivesworkrcd.h"
using namespace DTU;
//
ARCHIVEDB::ARCHIVEDB()
{
    // 保护动作报告
    _arc_db[RAM_FLAG_PROTACT] = std::make_unique<DArchiveProtact>();
    // 暂态录波数据
    _arc_db[RAM_FLAG_TRANSRECORDER] = std::make_unique<DArchiveTransRcd>();
    // 业务录波数据
    _arc_db[RAM_FLAG_PROTRECORDER] = std::make_unique<DArchiveWorkRcd>();
}
void ARCHIVEDB::add_archives(uint16_t id, const DTU::buffer& data)
{
    auto ita = _arc_db.find(id);
    if (ita != _arc_db.end())
    {
        ita->second->add_archives_data(data);
    }
    else
    {
        DTULOG(DTU_ERROR,(char*)"命令0x%04X无法生成档案", id);
    }
}
