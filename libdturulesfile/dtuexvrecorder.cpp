/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtufixrecorder.h
  *Description: 
    保存极值数据
  *History: 
    1, 创建, wangjs, 2021-8-20
**********************************************************************************/
#include "dtuexvrecorder.h"
#include <pugixml/pugixml.hpp>
#include <dtulog.h>
#include <dtucommon.h>

using namespace DTU;

static std::vector<std::tuple<std::string,std::string>> vfixValue = {
    {"母线侧A相电压","V"},{"母线侧B相电压","V"},{"母线侧C相电压","V"},{"母线侧零序电压","V"},
    {"母线侧A相电流","A"},{"母线侧B相电流","A"},{"母线侧C相电流","A"},{"母线侧零序电流","A"},
    {"负荷侧A相电压","V"},{"负荷侧B相电压","V"},{"负荷侧C相电压","V"},{"负荷侧零序电压","V"},
    {"负荷侧A相电流","A"},{"负荷侧B相电流","A"},{"负荷侧C相电流","A"},{"负荷侧零序电流","A"},
    {"有功功率","W"},{"无功功率","Var"},{"直流电压1","V"},{"系统频率1","Hz"},
};

std::string MakeBuffTime(const buffer& data, std::string fmt){
    char chtime[64] = {};
    if (data.size() != 12){
        sprintf(chtime, fmt.c_str(), 0,0,0,0,0,0);
        return std::string(chtime);
    }
    uint16_t year = data.get(0, sizeof(uint16_t)).value<uint16_t>();
    uint16_t mon = data.get(1*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    uint16_t day = data.get(2*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    uint16_t hour = data.get(3*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    uint16_t min = data.get(4*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    uint16_t sec = data.get(5*sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
    sprintf(chtime, fmt.c_str(), year,mon,day,hour,min,sec);
    std::string ret = std::string(chtime);
    return ret.substr(2, ret.size()-2);
}
void DExvRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}
void DExvRcd::add_exv_data(buffer& data)
{
    const uint32_t exvdata_len = 692;
    const uint32_t data_size = 17*sizeof(uint16_t);
    const uint32_t time_len = 12;
    const uint32_t fix_offset = 0;
    const uint32_t maxvalue_offset = sizeof(uint16_t);
    const uint32_t maxtime_offset = 3*sizeof(uint16_t);
    const uint32_t minvalue_offset = 9*sizeof(uint16_t);
    const uint32_t mintime_offset = 11*sizeof(uint16_t);

    if (data.size() != exvdata_len){
        DTULOG(DTU_WARN,(char*)"极值数据长度有误:%u", data.size());
        data.remove();
        data.resize(exvdata_len);
    }
    // 时间
    int offset = 0;
    uint32_t year = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
    offset += sizeof(uint32_t);
    uint32_t mon = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
    offset += sizeof(uint32_t);
    uint32_t day = data.get(offset, sizeof(uint32_t)).value<uint32_t>();
    offset += sizeof(uint32_t);

    char file[64] = {};
    sprintf(file, (char*)"exv%04u%02u%02u.xml", year, mon, day);

    pugi::xml_document doc;

    pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
    xml_pre_node.append_attribute("version") = "1.0";
    xml_pre_node.append_attribute("encoding") = "UTF-8";

    pugi::xml_node datafile_node = doc.append_child("DataFile");
    pugi::xml_node header_node = datafile_node.append_child("Header");
    header_node.append_attribute("fileType") = "EXV";
    header_node.append_attribute("fileVer") = "1.00";
    header_node.append_attribute("devName") = "SDL9200";
    /////////////////////////
    pugi::xml_node dataAttr_node = datafile_node.append_child("DataAttr");
    dataAttr_node.append_attribute("num") = std::to_string(vfixValue.size()).c_str();

    pugi::xml_node dataRec_node = datafile_node.append_child("DataRec");

    for(auto i=0;i<vfixValue.size();i++)
    {
        buffer itemData = data.get(offset + i*data_size, data_size);
        // 点号
        uint16_t fix = 
            itemData.get(fix_offset, sizeof(uint16_t)).value<uint16_t>();
        pugi::xml_node di_node = dataAttr_node.append_child("DI");
        di_node.append_attribute("ioa") = std::to_string(fix).c_str();
        di_node.append_attribute("type") = "float";
        di_node.append_attribute("unit") = std::get<1>(vfixValue[i]).c_str();
        
        pugi::xml_node di_rec_node = dataRec_node.append_child("DI");
        // 最大值
        float maxvalue = 
            itemData.get(maxvalue_offset, sizeof(float)).value<float>();
        di_rec_node.append_attribute("max") = std::to_string(maxvalue).c_str();
        // 最大值时间
        std::string strMaxTime =
             MakeBuffTime(itemData.get(maxtime_offset, time_len), "%04u%02u%02u_%02u%02u%02u");
        di_rec_node.append_attribute("max_tm") = strMaxTime.c_str();
        // 最小值
        float minvalue = 
            itemData.get(minvalue_offset, sizeof(float)).value<float>();
        di_rec_node.append_attribute("min") = std::to_string(minvalue).c_str();
        // 最小值时间
        std::string strMinTime =
             MakeBuffTime(itemData.get(mintime_offset, time_len), "%04u%02u%02u_%02u%02u%02u");
        di_rec_node.append_attribute("min_tm") = strMinTime.c_str();
    }
    std::string fullName = get_exec_dir() + "/HISTORY/EXV/"+std::string(file);
    // 保存文件
    doc.save_file(fullName.c_str());
}

void DExvRcd::get_exv_dir(buffer& data)
{
    data.remove();

    std::string dir = get_exec_dir()+"/HISTORY/EXV";
    FILELIST list;

    get_dir_files(dir, list);
    data.resize(20*list.size());
    for(auto i=0;i<list.size();i++)
    {
        data.set(i*20, std::get<0>(list[i]).c_str(), std::get<0>(list[i]).size());
    }     

}
