/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtusoerecorder.cpp
  *Description: 
    soe规约文件形成
  *History: 
    1, 创建, wangjs, 2021-8-23
**********************************************************************************/
#include "dtusoerecorder.h"
#include <dtustorage.h>
#include <dtucmdcode.h>
#include <dtucommon.h>
#include <pugixml/pugixml.hpp>
#include <dtulog.h>

using namespace DTU;

void DSoeRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}
void DSoeRcd::form_soe_file()
{
    pugi::xml_document doc;

    pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
    xml_pre_node.append_attribute("version") = "1.0";
    xml_pre_node.append_attribute("encoding") = "UTF-8";


    pugi::xml_node node_data_file = doc.append_child("DataFile");
    pugi::xml_node node_hdr = node_data_file.append_child("Header");
    node_hdr.append_attribute("fileType") = "SOE";
    node_hdr.append_attribute("fileVer") = "1.00";
    node_hdr.append_attribute("devID") = "SDL9200";

    pugi::xml_node node_rec = node_data_file.append_child("DataRec");
    uint32_t serialno = DSTORE::instance().get_cur_report_no(ReportSOE);
    int count = 0;
    for(auto i=0;i<1024;i++)
    {
        if (serialno <= 0){
            break;
        }
        uint16_t ioa=0, val =0;
        std::string tm;

        // if (get_soe_content(DSTORE::instance().get_report_by_serial(ReportSOE, serialno),
        //     ioa, tm, val))
        // {
        //     pugi::xml_node node_DI = node_rec.append_child("DI");
        //     node_DI.append_attribute("ioa") = std::to_string(ioa).c_str();
        //     node_DI.append_attribute("tm") = tm.c_str();
        //     node_DI.append_attribute("val") = std::to_string(val).c_str();
        // }
        serialno--;
        count++;
    }
    node_rec.append_attribute("num") = std::to_string(count).c_str();
    std::string fullPath = get_exec_dir()+"/HISTORY/SOE/soe.xml";
    doc.save_file(fullPath.c_str());
}

bool DSoeRcd::get_soe_content(const DTU::buffer& data, 
    uint16_t& ioa, std::string& tm, uint16_t& val)
{
    if (data.size() != 20){
        DTULOG(DTU_ERROR,(char*)"get_soe_content数据长度有误%u,应该为20", data.size());
        return false;
    }
    // 获取时间
    uint32_t seconds = data.get(0, sizeof(seconds)).value<uint32_t>();
    uint32_t mircosec = data.get(sizeof(seconds), sizeof(seconds)).value<uint32_t>();

    uint64_t acttime = ((uint64_t)seconds*1000000+(uint64_t)mircosec);

    tm = create_time_from_format(acttime, "%02u%02u%02u_%02u%02u%02u_%03u");

    ioa = data.get(6*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    // 当前状态
    uint16_t status = data.get(7*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    // 
    val = ((status & 0xff00)>>8);

    return true;
}