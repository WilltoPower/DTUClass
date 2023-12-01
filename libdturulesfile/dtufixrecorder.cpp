/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtufixrecorder.cpp
  *Description: 
    保存定点数据
  *History: 
    1, 创建, wangjs, 2021-8-20
**********************************************************************************/
#include "dtufixrecorder.h"
#include <chrono>
#include <dtucommon.h>
#include <dtuparamconfig.h>
#include <dspcomm.h>
#include <dtucmdcode.h>
#include <dtuerror.h>
#include <thread>
#include <dtulog.h>
#include <dtudbmanager.h>

using namespace DTU;

// 要抽取的名称(务必用UTF8编码,在数据库中使用这个进行查找)
static std::vector<std::tuple<std::string,std::string>> vfixValue = {
    {"母线侧A相电压","V"},{"母线侧B相电压","V"},{"母线侧C相电压","V"},{"母线侧零序电压","V"},
    {"母线侧A相电流","A"},{"母线侧B相电流","A"},{"母线侧C相电流","A"},{"母线侧零序电流","A"},
    {"负荷侧A相电压","V"},{"负荷侧B相电压","V"},{"负荷侧C相电压","V"},{"负荷侧零序电压","V"},
    {"负荷侧A相电流","A"},{"负荷侧B相电流","A"},{"负荷侧C相电流","A"},{"负荷侧零序电流","A"},
    {"有功功率","W"},{"无功功率","Var"},{"直流电压1","V"},{"系统频率1","Hz"},
};

static std::vector<std::string> vfmtTime = {
    "001500","003000","004500","010000",
    "011500","013000","014500","020000",
    "021500","023000","024500","030000",
    "031500","033000","034500","040000",
    "041500","043000","044500","050000",
    "051500","053000","054500","060000",
    "061500","063000","064500","070000",
    "071500","073000","074500","080000",
    "081500","083000","084500","090000",
    "091500","093000","094500","100000",
    "101500","103000","104500","110000",
    "111500","113000","114500","120000",
    "121500","123000","124500","130000",
    "131500","133000","134500","140000",
    "141500","143000","144500","150000",
    "151500","153000","154500","160000",
    "161500","163000","164500","170000",
    "171500","173000","174500","180000",
    "181500","183000","184500","190000",
    "191500","193000","194500","200000",
    "201500","203000","204500","210000",
    "211500","213000","214500","220000",
    "221500","223000","224500","230000",
    "231500","233000","234500","240000",
};

uint64_t get_zero_time(){
    time_t t = time(NULL);
    struct tm* tms = localtime(&t);
    tms->tm_hour = 0;
    tms->tm_min = 0;
    tms->tm_sec = 0;
    //
    struct tm tm1;
	tm1.tm_sec = 0;
	tm1.tm_min = 0;
	tm1.tm_hour = 0;
	tm1.tm_mday = 1;
	tm1.tm_mon = 0;
	tm1.tm_year = 100;
	tm1.tm_isdst = 0;
	time_t tt1 = mktime(&tm1);//2000.1.1 0:0:0以前的秒计数

    return (mktime(tms)-tt1);
}

DFixRcd::DFixRcd()
:_bstop(false)
{
    std::stringstream ss;
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&t), "fixpt%Y%m%d.xml");
    auto fileName = ss.str();// create_time_from_format(currentmicro, );

    std::string fullName = get_exec_dir() + "/HISTORY/FIXPT/" + fileName;

    if (!open_fix_file(fullName))
    {
        init_fix_file(fullName);
    }
    _current_file = fullName;
}

void DFixRcd::stop_form_fix_rcd()
{
    _bstop = true;
}

void DFixRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}

void DFixRcd::add_fix_rcd(uint32_t index, std::string fixtime)
{
    try
    {
        // 读取测量值
        buffer dspAck;
        if (DTU_SUCCESS != DTU::DSPComm::GetInstance()->dsp_read_data(PC_R_YC_DATA, dspAck, 100))
        {
            DTULOG(DTU_WARN,(char*)"add_fix_rcd 读取测量值失败");
            dspAck.resize(DBManager::instance().GetInfomationTableByIndex(InfomTelemetry).size);
        }
        // 添加定点数据
        fixtime = fixtime.substr(2, fixtime.size()-2);
        DTULOG(DTU_INFO, (char*)"添加定点数据:%s %s", _current_file.c_str(), fixtime.c_str());
        pugi::xml_node xml_rec = _doc.child("DataFile").append_child("DataRec");
        xml_rec.append_attribute("sect") = std::to_string(index).c_str();
        xml_rec.append_attribute("tm") = fixtime.c_str();
        
        // 上送到公共单元
        buffer fixdata;
        for(const auto& item : vfixValue)
        {
            uint16_t fixValue = DTU::DParamConfig::instance().get_value_fix(std::get<0>(item));
            uint32_t offset = DTU::DParamConfig::instance().get_value_offset(TABLE_INFOM, fixValue);
            uint32_t size = DTU::DParamConfig::instance().get_value_length(TABLE_INFOM, fixValue);
            float value = dspAck.get(offset,size).value<float>();
            pugi::xml_node xml_di = xml_rec.append_child("DI");
            xml_di.append_attribute("val") = std::to_string(value).c_str();
            //////////////////////////////////////////////
            buffer fixItem;
            fixItem.append((char*)&fixValue, sizeof(uint16_t));
            // 类型
            uint8_t typesize = 5;
            fixItem.append((char*)&typesize, sizeof(typesize));
            fixItem.append("float", typesize);
            // 单位
            uint8_t unitsize = std::get<1>(item).size();
            fixItem.append((char*)&unitsize, sizeof(unitsize));
            fixItem.append(std::get<1>(item).c_str(), unitsize);

            uint16_t itemsize = fixItem.size();
            fixdata.append((char*)&itemsize, sizeof(uint16_t));
            fixdata.append(fixItem);
        }

        // 修改总计数(节点个数)
        auto count = _doc.child("DataFile").child("DataAttr").attribute("sectNum").as_int();
        count++;
        _doc.child("DataFile").child("DataAttr").attribute("sectNum") = std::to_string(count).c_str();

        _doc.save_file(_current_file.c_str());
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_ERROR, (char*)"添加定点数据:%s发生错误", e.what());
        return;
    }
}

bool DFixRcd::open_fix_file(const std::string& fileName)
{
    pugi::xml_parse_result res = _doc.load_file(fileName.c_str());
    if (res.status != pugi::status_ok){
        return false;
    }
    return true;
}

void DFixRcd::form_fix_rcd()
{
    std::thread t([&]()
    {
        while(!_bstop)
        {
            std::stringstream ss;
            auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            ss << std::put_time(std::localtime(&t), "%H%M%S");
            auto fmttime = ss.str();

            std::stringstream name;
            name << std::put_time(std::localtime(&t),"%Y%m%d_");

            for(auto i=0;i<vfmtTime.size();i++)
            {
                if (fmttime == "000000")
                {
                    fmttime = "240000";
                }
                if (fmttime == vfmtTime[i])
                {
                    add_fix_rcd(i+1, name.str() + fmttime);
                    if (i == vfmtTime.size()-1){
                         complete_fix_file();
                    }
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    t.detach();
}

void DFixRcd::init_fix_file(const std::string& fileName)
{
    pugi::xml_node xml_pre_node = _doc.prepend_child(pugi::node_declaration);
    xml_pre_node.append_attribute("version") = "1.0";
    xml_pre_node.append_attribute("encoding") = "UTF-8";

    pugi::xml_node xml_data_file = _doc.append_child("DataFile");

    pugi::xml_node xml_header = xml_data_file.append_child("Header");
    xml_header.append_attribute("fileType") = "FIXPT";
    xml_header.append_attribute("fileVer") = "1.00";
    xml_header.append_attribute("devName") = "SDL9200";

    pugi::xml_node xml_data_attr = xml_data_file.append_child("DataAttr");
    xml_data_attr.append_attribute("dataNum") = std::to_string(vfixValue.size()).c_str();
    xml_data_attr.append_attribute("sectNum") = "";
    xml_data_attr.append_attribute("interval") = "15min";
    for(const auto& item : vfixValue)
    {
        pugi::xml_node xml_DI = xml_data_attr.append_child("DI");
        xml_DI.append_attribute("ioa") = 
            std::to_string(DTU::DParamConfig::instance().get_value_fix(std::get<0>(item))).c_str();
        xml_DI.append_attribute("type") = "float";
        xml_DI.append_attribute("unit") = std::get<1>(item).c_str();
    }
    _doc.save_file(fileName.c_str());
}

void DFixRcd::complete_fix_file()
{
    // // 修改节点个数sectNum
    // int count = 0;
    // for(pugi::xml_node node = _doc.child("DataFile").first_child(); node; node = node.next_sibling())
    // {
    //    count++;
    // }
    // //_doc.child("DataAttr").find_attribute("sectNum") = std::to_string(count-2).c_str();

    // 保存文件
    _doc.save_file(_current_file.c_str());
    // 清空原数据
    _doc.reset();

    // _current_file =  
    //     get_exec_dir() + "/HISTORY/FIXPT/" + create_time_from_format(get_current_mirco(), "fixpt%04u%02u%02u.xml");

    std::stringstream ss;
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&t), "fixpt%Y%m%d.xml");
    std::string fullName = get_exec_dir() + "/HISTORY/FIXPT/" + ss.str();
    _current_file = fullName;

    init_fix_file(_current_file);
}

void DFixRcd::get_fix_dir(buffer& files)
{
    files.remove();

    std::string dir = get_exec_dir()+"/HISTORY/FIXPT";
    FILELIST list;

    get_dir_files(dir, list);
    files.resize(20*list.size());
    for(auto i=0;i<list.size();i++)
    {
        files.set(i*20, std::get<0>(list[i]).c_str(), std::get<0>(list[i]).size());
    }     
}