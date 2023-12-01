/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtucorecorder.cpp
  *Description: 
    co规约文件形成
  *History: 
    1, 创建, lhy, 2021-11-26
**********************************************************************************/
#include "dtucorecorder.h"
#include <dtustorage.h>
#include <dtucmdcode.h>
#include <dtucommon.h>
#include <pugixml/pugixml.hpp>
#include <dtulog.h>
#include <vector>
#include <tuple>
#include <map>
#include <time.h>
#include <dtuparamconfig.h>
#include "dtuprotocol.h"
#include "dtutask_dsp.h"
#include "dtustorage.h"

#define TimeBeginTimeStamp 946656000

using namespace DTU;

/*
+-----------------------------------+
|  时间  | 2 w(s) 2 w(us) 1 w(闰秒) |
+-------+--------------------------+
|  点表  |           1 w           |
+-------+--------------------------+
|  操作  |           1 w           |
+-------+--------------------------+
|  合分  |           1 w           |
+-------+--------------------------+
|  保留  |           2 w           |
+-------+--------------------------+
*/

std::string getCmdStr(int opt)
{
    switch(opt)
    {
        case RC_CMD_PRE: return std::string("select");break;
        case RC_CMD_EXE: return std::string("oper");break;
        case RC_CMD_CAN: return std::string("cancel");break;
    }
}

std::string timestamp_to_date(time_t tt,uint32_t ms)
{
    struct tm *t = localtime(&tt);
    char dateBuf[128];
    snprintf(dateBuf, sizeof(dateBuf), "%02d%02d%02d_%02d%02d%02d_%03d", t->tm_year+1900,
    t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,ms);
    std::string date(dateBuf);
    return date;
}

uint32_t StrToTimeStamp(std::string time)
{
    struct tm stm;
    strptime(time.c_str(),"%Y%m%d%H%M%S",&stm);
    uint32_t ret = mktime(&stm);
    return ret;
}

void DCoRcd::add_co_file(uint16_t fix,int operate,int status)
{
    // 获取时间
    DTU::buffer result;
    DTU::buffer findbuf;
    std::string time_trans;
    std::string time_show;
    int ms = 0;
    if(dsptask_execute_query(PC_R_CLK, findbuf) != DTU_SUCCESS)
    {
        DTULOG(DTU_ERROR,"add_co_file() 查询时间错误");
        std::stringstream ss;
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        ss << std::put_time(std::localtime(&t), "%Y%m%d%H%M%S");
        time_trans = ss.str();
        ss.clear();
        ss << std::put_time(std::localtime(&t), "%02Y%m%d_%H%M%S_000");
        time_show = ss.str();
    }
    else
    {
        uint16_t year = findbuf.get(0, sizeof(uint16_t)).value<uint16_t>();
        uint8_t mon = findbuf.get(sizeof(uint16_t) * 1, sizeof(uint8_t)).value<uint8_t>();
        uint8_t day = findbuf.get(sizeof(uint8_t) * 3, sizeof(uint8_t)).value<uint8_t>();
        uint8_t hh = findbuf.get(sizeof(uint8_t) * 4, sizeof(uint8_t)).value<uint8_t>();
        uint8_t mm = findbuf.get(sizeof(uint8_t) * 5, sizeof(uint8_t)).value<uint8_t>();
        uint8_t ss = findbuf.get(sizeof(uint8_t) * 6, sizeof(uint8_t)).value<uint8_t>();
        ms = findbuf.get(sizeof(uint8_t) * 7, sizeof(uint16_t)).value<uint16_t>();
        char buf_trans[32];
        sprintf(buf_trans, "%04d%02d%02d%02d%02d%02d", year, mon, day, hh, mm, ss);
        time_trans = buf_trans;
        char buf_show[32];
        sprintf(buf_show, "%02d%02d%02d_%02d%02d%02d_%02d", year, mon, day, hh, mm, ss, ms);
        time_show = buf_show;
    }

    uint32_t DeviceTimeStamp = StrToTimeStamp(time_trans);
    uint32_t s = DeviceTimeStamp;
    // 追加秒信息
    result.append((char*)&DeviceTimeStamp,sizeof(DeviceTimeStamp));
    DeviceTimeStamp = 0;
    // 追加us信息
    result.append((char*)&ms,sizeof(ms));
    // 追加闰秒信息
    result.append((char*)&DeviceTimeStamp,sizeof(DeviceTimeStamp));

    uint16_t QOC = operate;
    int16_t val = status;
    // 添加点表值
    result.append((char*)&fix,sizeof(fix));
    // 添加QOC
    result.append((char*)&QOC,sizeof(QOC));
    // 添加命令状态
    result.append((char*)&val,sizeof(val));
    // 添加保留
    result.append(DTU::buffer(2));
    // 添加报告
    DTU::DSTORE::instance().add_report_data(ReportRMC,s,ms,result);
    // 生成文件
    form_co_file();
}
    
void DCoRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}

void DCoRcd::form_co_file()
{
    pugi::xml_document doc;

    pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
    xml_pre_node.append_attribute("version") = "1.0";
    xml_pre_node.append_attribute("encoding") = "UTF-8";


    pugi::xml_node node_data_file = doc.append_child("DataFile");
    pugi::xml_node node_hdr = node_data_file.append_child("Header");
    node_hdr.append_attribute("fileType") = "CO";
    node_hdr.append_attribute("fileVer") = "1.00";
    node_hdr.append_attribute("devName") = "SDL9200";

    pugi::xml_node node_rec = node_data_file.append_child("DataRec");
    uint32_t serialno = DSTORE::instance().get_cur_report_no(ReportRMC);
    int count = 0;
    for(auto i=0;i<30;i++)
    {
        if (serialno <= 0){
            break;
        }
        uint16_t ioa = 0, val = 0 ,opt = 0;
        std::string tm;
        if (get_co_content(DSTORE::instance().get_report_by_serial(ReportRMC, serialno),
            ioa, tm, opt, val))
        {
            pugi::xml_node node_DI = node_rec.append_child("DI");
            node_DI.append_attribute("ioa") = std::to_string(ioa).c_str();
            node_DI.append_attribute("tm") = tm.c_str();
            node_DI.append_attribute("cmd") = getCmdStr(opt).c_str();
            node_DI.append_attribute("val") = std::to_string(val).c_str();
        }
        serialno--;
        count++;
    }
    node_rec.append_attribute("num") = std::to_string(count).c_str();
    std::string fullPath = get_exec_dir()+"/HISTORY/CO/co.xml";
    doc.save_file(fullPath.c_str());
}

bool DCoRcd::get_co_content(const DTU::buffer& data, 
    uint16_t& ioa, std::string& tm,uint16_t& opt, uint16_t& val)
{
    if (data.size() != 20){
        DTULOG(DTU_ERROR,(char*)"get_co_content数据长度有误%u,应该为20", data.size());
        return false;
    }
    // 获取时间
    uint32_t seconds = data.get(0, sizeof(seconds)).value<uint32_t>();
    uint32_t mircosec = data.get(sizeof(seconds), sizeof(seconds)).value<uint32_t>();

    // uint64_t acttime = ((uint64_t)seconds*1000000+(uint64_t)mircosec);

    // tm = create_time_from_format(acttime, "%02u%02u%02u_%02u%02u%02u_%03u");
    tm = timestamp_to_date(seconds,mircosec);
    tm.erase(0,2);
    ioa = data.get(6*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    opt = data.get(7*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    val = data.get(8*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    return true;
}