/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuelecenergyfrz.cpp
  *Description:
    用于实现dtu日冻结电能量生成
  *History:
    1, 创建, lhy, 2022-08-11
**********************************************************************************/
#include "dtuelecenergyfrz.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>

#include <dtucommon.h>
#include <dtulinelossmode.h>
#include "dtulog.h"


// 数据长度
#define ELECTRIC_ENERGY_DATA_LENGTH 8
// 信息体地址起始
#define IOA_START 0x6401

using namespace DTU;

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

DFrzRcd::DFrzRcd() : _bstop(false)
{
    // 获取时间
	std::stringstream fileName;
	auto nowdate = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	fileName << std::put_time(std::localtime(&nowdate), "frz%Y%m%d.xml");

	// 获取文件路径
	std::string frz_fullPath = get_exec_dir() + "/HISTORY/FRZ/" + fileName.str();

    if (!open_frz_file(frz_fullPath)) {
        init_frz_file(frz_fullPath);
    }

	_CurrentFile  = frz_fullPath;
}

bool DFrzRcd::open_frz_file(const std::string fullPath)
{
    bool result = false;
    pugi::xml_parse_result res = _doc.load_file(fullPath.c_str());
    if (res.status == pugi::status_ok) {
        result = true;
    }
    return result;
}

void DFrzRcd::init_frz_file(const std::string fullPath)
{
    DTULOG(DTU_INFO,"初始化冻结电能量文件:%s",fullPath.c_str());

    _doc.reset();

	pugi::xml_node xml_pre_node = _doc.prepend_child(pugi::node_declaration);
	xml_pre_node.append_attribute("version") = "1.0";
	xml_pre_node.append_attribute("encoding") = "UTF-8";
	pugi::xml_node xml_data_file = _doc.append_child("DataFile");
	pugi::xml_node xml_header = xml_data_file.append_child("Header");

	xml_header.append_attribute("fileType") = "FRZ";
	xml_header.append_attribute("fileVer") = "1.00";
	xml_header.append_attribute("devName") = "SDL9200";

	pugi::xml_node xml_data_attr_fix = xml_data_file.append_child("DataAttr");
	xml_data_attr_fix.append_attribute("type") = "FixFrz";
	xml_data_attr_fix.append_attribute("dataNum") = "8";
	xml_data_attr_fix.append_attribute("sectNum") = "0";
	xml_data_attr_fix.append_attribute("interval") = "15min";

    // 按国网的标准0x6049起始
	uint32_t ioa = IOA_START + ELECTRIC_ENERGY_DATA_LENGTH;

	for (int i = 0; i < ELECTRIC_ENERGY_DATA_LENGTH; i++)
	{
		pugi::xml_node xml_DI = xml_data_attr_fix.append_child("DI");
		xml_DI.append_attribute("ioa") = std::to_string(ioa).c_str();
		xml_DI.append_attribute("type") = "float";
		if (i < 2)
			xml_DI.append_attribute("uint") = "kWh";
		else
			xml_DI.append_attribute("uint") = "kVarh";
		ioa++;
	}

	pugi::xml_node xml_data_attr_day = xml_data_file.append_child("DataAttr");
	xml_data_attr_day.append_attribute("type") = "DayFrz";
	xml_data_attr_day.append_attribute("dataNum") = "8";
	for (int i = 0; i < ELECTRIC_ENERGY_DATA_LENGTH; i++)
	{
		pugi::xml_node xml_DI = xml_data_attr_day.append_child("DI");
		xml_DI.append_attribute("ioa") = std::to_string(ioa).c_str();
		xml_DI.append_attribute("type") = "float";
		if (i < 2)
			xml_DI.append_attribute("uint") = "kWh";
		else
			xml_DI.append_attribute("uint") = "kVarh";
		ioa++;
	}
	
    _doc.save_file(fullPath.c_str());
}

void DFrzRcd::complete_frz_file(DTU::buffer &data)
{
    pugi::xml_node xml_data_file = _doc.child("DataFile");
    pugi::xml_node xml_rec = xml_data_file.append_child("DataRec");
    
    // 日冻结数据
    xml_rec.append_attribute("type") = "DayFrz";
    // 偏移8组元素后 从日冻结数据中获取内容 (17 + 2 + 4) X 8
    std::string time(data.get(184, 17).const_data(),17);
	xml_rec.append_attribute("tm") = time.c_str();

    // 生成日冻结
    for(int offset=184; offset<(data.size()/2); offset+=23)
    {
        float value = data.get(offset+19, sizeof(float)).value<float>();
		pugi::xml_node xml_di = xml_rec.append_child("DI");
		xml_di.append_attribute("val") = value;
    }

    int count = _doc.first_child().child("DataAttr").attribute("sectNum").as_uint();
    count++;
	xml_data_file.child("DataAttr").attribute("sectNum") = count;

    // 保存文件
    _doc.save_file(_CurrentFile.c_str());
    // 重置文件
    _doc.reset();

    std::stringstream ss;
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&t), "frz%Y%m%d.xml");
    std::string fullName = get_exec_dir() + "/HISTORY/FRZ/" + ss.str();
    _CurrentFile = fullName;
    // 初始化新的电能量文件并将上一天最后一条添加到本天第一条
    init_frz_file(_CurrentFile);
    add_first_frz_rcd(data);
}

void DFrzRcd::add_first_frz_rcd(DTU::buffer &data)
{
    try
    {
        int count = 1;
		pugi::xml_node xml_data_file = _doc.child("DataFile");

		// 添加新的节点
		pugi::xml_node xml_rec = xml_data_file.append_child("DataRec");
		xml_rec.append_attribute("sect") = count;
        std::string time(data.get(0, 17).const_data(), 17);
		xml_rec.append_attribute("tm") = time.c_str();

        for(int offset=0; offset<(data.size()/2); offset+=23)
        {
            float value = data.get(offset+19, sizeof(float)).value<float>();
            pugi::xml_node xml_di = xml_rec.append_child("DI");
            xml_di.append_attribute("val") = value;
        }

        // 修改总计数
        xml_data_file.child("DataAttr").attribute("sectNum") = count;

        _doc.save_file(_CurrentFile.c_str());
		DTULOG(DTU_INFO,"添加冻结电能量记录:%s",_CurrentFile.c_str());
    }
    catch(std::exception &e)
    {
        DTULOG(DTU_ERROR, "添加冻结电能量记录发生错误2:%s", e.what());
    }
}

// frzBuffer 时间17字节 IOA 2字节 float 四字节
void DFrzRcd::add_frz_rcd(DTU::buffer &frzBuffer)
{
    try
    {
        // 获取当前的序号
        int count = _doc.first_child().child("DataAttr").attribute("sectNum").as_uint();
        count++;

		pugi::xml_node xml_data_file = _doc.child("DataFile");

		// 添加新的节点
		pugi::xml_node xml_rec = xml_data_file.append_child("DataRec");
		xml_rec.append_attribute("sect") = count;
        std::string time(frzBuffer.get(0, 17).const_data(),17);
        time.erase(time.size()-4);
		xml_rec.append_attribute("tm") = time.c_str();

        // 这里除以2是因为前八个元素是15min冻结 后8个元素是日冻结
        for(int offset=0; offset<(frzBuffer.size()/2); offset+=23)
        {
            float value = frzBuffer.get(offset+19, sizeof(float)).value<float>();
            pugi::xml_node xml_di = xml_rec.append_child("DI");
            xml_di.append_attribute("val") = value;
        }

        // 修改总计数
        xml_data_file.child("DataAttr").attribute("sectNum") = count;

        if (_index != 96) {
            _doc.save_file(_CurrentFile.c_str());
		    DTULOG(DTU_INFO,"添加冻结电能量记录:%s",_CurrentFile.c_str());
        }
        else {
            // 生成日冻结电能量
            complete_frz_file(frzBuffer);
            DTULOG(DTU_INFO,"生成日冻结电能量:%s",_CurrentFile.c_str());
        }
    }
    catch(std::exception &e)
    {
        DTULOG(DTU_ERROR, "添加冻结电能量记录发生错误:%s", e.what());
    }
}

void DFrzRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}

void DFrzRcd::form_frz_rcd()
{
    DTULOG(DTU_INFO, (char *)"启动F-PIC100线损模块电能量报文生成...");
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
                    fmttime = "240000";

                if (fmttime == vfmtTime[i]) {
                    _index = i + 1;
                    dtuLinelossmode::instance().callElecEnergy();
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    t.detach();
}

void DFrzRcd::stop_form_frz_rcd()
{
    _bstop = true;
}