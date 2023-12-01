/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuelecenergyfro.cpp
  *Description:
    用于实现dtu潮流反向冻结电能量文件生成
  *History:
    1, 创建, lhy, 2022-08-11
**********************************************************************************/
#include "dtuelecenergyfro.h"
#include <chrono>

#include <dtucommon.h>
#include "dtulog.h"

using namespace DTU;

DFroRcd::DFroRcd()
{
	// 获取时间
	std::string flo_fullPath = get_exec_dir() + "/HISTORY/FLOWREV/flowrev.xml";
	//pugi::xml_parse_result flo_res = _doc.load_file(flo_fullPath.c_str());
	
	if (!open_fro_file(flo_fullPath)) {
		init_fro_file(flo_fullPath);
	}

	_CurrentFile = flo_fullPath;
}

bool DFroRcd::open_fro_file(const std::string &fullPath)
{
	bool result = false;
    pugi::xml_parse_result res = _doc.load_file(fullPath.c_str());
	
    if (res.status == pugi::status_ok) {
        result = true;
    }
    return result;
}

void DFroRcd::init_fro_file(const std::string &fullPath)
{
	_doc.reset();

	pugi::xml_node xml_pre_node = _doc.prepend_child(pugi::node_declaration);
	xml_pre_node.append_attribute("version") = "1.0";
	xml_pre_node.append_attribute("encoding") = "UTF-8";
	pugi::xml_node xml_data_file = _doc.append_child("DataFile");
	pugi::xml_node xml_header = xml_data_file.append_child("Header");

	xml_header.append_attribute("fileType") = "FLOWREV";
	xml_header.append_attribute("fileVer") = "1.00";
	xml_header.append_attribute("devName") = "SDL9200";

	pugi::xml_node xml_data_attr_rec = xml_data_file.append_child("DataRec");
	xml_data_attr_rec.append_attribute("num") = "0";
	_doc.save_file(fullPath.c_str());
}

void DFroRcd::set_mode(uint16_t addr, uint16_t mode)
{
    _addr = addr;
    _mode = mode;
}

// buffer 格式:
// 时间(17字节) IOA(2字节) 值float(4字节)
void DFroRcd::add_fro_rcd(DTU::buffer &data)
{
	// if (data.size() != LENGTH) {
	// 	DTULOG(DTU_ERROR, "反向潮汐冻结电能量报文长度错误");
	// }

	int count = _doc.first_child().child("DataRec").attribute("num").as_uint();

	pugi::xml_node xml_data_rec = _doc.child("DataFile").child("DataRec");

	for(int offset; offset<data.size(); offset+=23)
	{
		pugi::xml_node xml_di = xml_data_rec.append_child("DI");
		count++;
		xml_di.append_attribute("ioa") = data.get(offset+17,sizeof(uint16_t)).value<uint16_t>();
		xml_di.append_attribute("tm") =  data.get(offset, 17).const_data();
		xml_di.append_attribute("val") = data.get(offset+19,sizeof(float)).value<float>();
	}

	xml_data_rec.attribute("num") = count;

	_doc.save_file(_CurrentFile.c_str());
	DTULOG(DTU_INFO, "添加反向潮汐冻结电能量记录");
}