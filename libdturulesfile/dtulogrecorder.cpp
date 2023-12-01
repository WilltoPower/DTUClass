/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulogrecorder.cpp
  *Description: 
    保存日志数据
  *History: 
    1, 创建, wangjs, 2021-8-23
**********************************************************************************/
#include "dtulogrecorder.h"
#include <dtulog.h>
#include <dtucommon.h>
using namespace DTU;
void DLogRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}
// 添加日志数据
void DLogRcd::add_log_data(const buffer& data)
{
    // 单个日志长度212字节
    const uint32_t loglength = 212;
    const uint32_t logcount = 1024;

    // 日志长度为212的倍数
    int count = (data.size()) % loglength;
    if(0 != count)
    {
        if((count/loglength) == 0)
        {
            count = 1;
        }
        else
        {
            count = count / loglength;
        }
        DTULOG(DTU_ERROR,(char*)"规约日志长度有误%u, 应该为%u", data.size(), count * loglength);
        return;
    }

    count = (data.size()) / loglength;
    int offset = 0;
    std::vector<DTU::buffer> LogList;
    for(int i = 0;i < count;i++)
    {
        LogList.push_back(DTU::buffer(data.const_data() + offset, loglength));
        offset += loglength;
    }

    std::lock_guard<std::mutex> lock(_queue_lock);
    for(auto const &item : LogList)
    {
        if (_log_queue.size() == logcount){
        // 移除一个旧的日志
        _log_queue.pop_front();
        }
        _log_queue.push_back(item);
    }

    //data.dump(0, data.size());
}

// 生成日志
void DLogRcd::form_log_file()
{
    pugi::xml_document doc;

    pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
    xml_pre_node.append_attribute("version") = "1.0";
    xml_pre_node.append_attribute("encoding") = "UTF-8";


    pugi::xml_node node_data_file = doc.append_child("DataFile");
    pugi::xml_node node_hdr = node_data_file.append_child("Header");
    node_hdr.append_attribute("fileType") = "Ulog";
    node_hdr.append_attribute("fileVer") = "1.00";
    node_hdr.append_attribute("devID") = "SDL9200";

    pugi::xml_node node_rec = node_data_file.append_child("DataRec");
    ////////////////
    std::lock_guard<std::mutex> lock(_queue_lock);
    node_rec.append_attribute("num") = _log_queue.size();
    auto ita = _log_queue.begin();
    while(ita != _log_queue.end())
    {
        uint16_t type = 0, val = 0;
        std::string time, content;
        if (get_log_content(*ita, type, time, content, val))
        {
            pugi::xml_node node_DI = node_rec.append_child("DI");
            node_DI.append_attribute("logType") = std::to_string(type).c_str();
            node_DI.append_attribute("tm") = time.c_str();
            node_DI.append_attribute("text") = content.c_str();
            node_DI.append_attribute("val") = std::to_string(val).c_str();
        }
        ita++;
    }
    std::string fullPath = get_exec_dir()+"/HISTORY/ULOG/ulog.xml";

    doc.save_file(fullPath.c_str());
}
//
bool DLogRcd::get_log_content(const buffer& data, uint16_t& type, 
                                std::string& time, std::string& content, 
                                uint16_t& val)
{
    const uint32_t loglength = 212;
    if (data.size() != loglength){
        DTULOG(DTU_ERROR,(char*)"get_log_content规约日志长度有误%u, 应该为%u", data.size(), loglength);
        return false;
    }
    uint32_t offset = 0;
    type = data.get(offset, sizeof(type)).value<uint16_t>();
    offset+=sizeof(type);
    //
    uint16_t year = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t mon = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t day = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t hour = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t min = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t sec = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);
    uint16_t millsec = data.get(offset, sizeof(year)).value<uint16_t>();
    offset+=sizeof(uint16_t);

    char datetime[64] = {};
    sprintf(datetime, "%02u%02u%02u_%02u%02u%02u_%03u", year, 
        mon, day, hour, min,sec, millsec);
    time = datetime;

    content = data.get(offset, 192).data();
    offset+=192;

    val = data.get(offset, sizeof(uint16_t)).value<uint16_t>();
    

}