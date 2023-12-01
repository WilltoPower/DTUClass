/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  sdlioamap.cpp
  *Description:
    读写配置
  *History:
    1, 创建, lhy, 2023-01-16
**********************************************************************************/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "dtucommon.h"
#include "dtuioamap.h"
#include "dtulog.h"

using namespace DTU;
using namespace pugi;

bool IOAMap::readIOAFromFile(const std::string& filePath)
{
    bool result = false;

    xml_document doc;
	std::string fullPath = get_exec_dir() + filePath;

    auto ret = doc.load_file(fullPath.c_str());

    if (ret.status != status_ok) {
        DTULOG(DTU_ERROR, "加载配置文件%s失败", filePath.c_str());
        return false;
    }

	try
	{
        xml_node root = doc.child("informationObject");
        
        this->ioamap.clear();

        for (const auto& item : root)
        {
            oneIOAMapItem one;
            one.ioa = item.attribute("IOA").as_int();
            one.hioa = item.attribute("HIOA").as_int();
            one.type = static_cast<IOAMAPTYPE>(item.attribute("TYPE").as_int());
            one.spare = item.attribute("SPARE").as_bool();
            one.active = item.attribute("ACTIVE").as_bool();
            one.ca = item.attribute("CA").as_int();

            this->ioamap.insert({one.ioa, one});
        }
    }
	catch(const std::exception& e)
	{
		DTULOG(DTU_ERROR, "读取IOA配置文件发生错误:%s", e.what());
		result = false;
	}

    return result;
}

bool IOAMap::saveIOAToFile(const std::string& filePath)
{
    bool result = false;

    xml_document doc;

	try
	{
		// 头部编码信息
		xml_node PreNode = doc.prepend_child(pugi::node_declaration);
		PreNode.append_attribute("version") = "1.0";
		PreNode.append_attribute("encoding") = "utf-8";
		// 文档类型
		doc.append_child(node_doctype).set_value("informationObjectTable");

		xml_node root = doc.append_child("informationObject");

        root.append_child(node_comment).set_value(" IOA:信息体地址 HIOA:内部硬件地址 TYPE:信息体类型 SAPRE:是否备用 ACTIVE:是否主动上送(仅为遥测使用) ");
    
        char buff[16] = {};
        for (auto& item : this->ioamap)
        {
            xml_node one = root.append_child("IO");
            sprintf(buff, "0x%04X", item.second.ioa);
            one.append_attribute("IOA") = buff;
            sprintf(buff, "0x%04X", item.second.hioa);
            one.append_attribute("HIOA") = buff;
            one.append_attribute("TYPE") = static_cast<int>(item.second.type);
            one.append_attribute("SPARE") = item.second.spare;
            one.append_attribute("ACTIVE") = item.second.active;
            one.append_attribute("CA") = item.second.ca;
        }

        std::string fullPath = get_exec_dir() + filePath;
        doc.save_file(fullPath.c_str());
    }
	catch(const std::exception& e)
	{
		DTULOG(DTU_ERROR, "保存IOA配置文件发生错误:%s", e.what());
		result = false;
	}

    return result;
}

bool IOAMap::mapHIOAtoIOA(const HIOA& hioa, IOA& ioa, CA ca)
{
    bool result = false;

    for (const auto& item : this->ioamap)
    {
        if (item.second.hioa == hioa && ca == item.second.ca) {
            ioa = item.second.ioa;
            result = true;
            break;
        }
    }

    return result;
}

bool IOAMap::mapIOAtoHIOA(const IOA& ioa, HIOA& hioa, CA ca)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);
    if (ita != this->ioamap.end()) {
        if (ita->second.ca == ca) {
            hioa = ita->second.hioa;
            result = true;
        }
    }

    return result;
}

bool IOAMap::mapIOAtoHIOA(const IOA& ioa, HIOA& hioa)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);

    if (ita != this->ioamap.end()) {
        hioa = ita->second.hioa;
        result = true;
    }

    return result;
}

bool IOAMap::whereIOAFrom(const IOA& ioa, CA& ca)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);
    if (ita != this->ioamap.end()) {
        ca = ita->second.ca;
        result = true;
    }

    return result;
}

bool IOAMap::isHIOAExist(const HIOA& hioa)
{
    bool result = false;

    for (const auto& item : this->ioamap)
    {
        if (item.second.hioa == hioa) {
            result = true;
            break;
        }
    }

    return result;
}

bool IOAMap::isIOAExist(const IOA& ioa)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);
    if (ita != this->ioamap.end()) {
        result = true;
    }

    return result;
}

bool IOAMap::isIOASpare(const IOA& ioa)
{
    bool result = true;

    auto ita = this->ioamap.find(ioa);
    if (ita != this->ioamap.end()) {
        result = ita->second.spare;
    }

    return result;
}

bool IOAMap::isIOAActive(const IOA& ioa)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);
    if (ita != this->ioamap.end()) {
        result = ita->second.active;
    }

    return result;
}

std::vector<IOA> IOAMap::findIOASpare(CA ca)
{
    std::vector<IOA> result;

    for (const auto& item : this->ioamap)
    {
        if (item.second.spare)
            result.emplace_back(item.first);
    }

    return result;
}

std::vector<IOA> IOAMap::findIOASpareWithType(CA ca, IOAMAPTYPE type)
{
    std::vector<IOA> result;

    for (const auto& item : this->ioamap)
    {
        if (item.second.spare && (item.second.type == type) && (item.second.ca == ca))
            result.emplace_back(item.first);
    }

    return result;
}

std::vector<IOA> IOAMap::findIOASpareWithType(IOAMAPTYPE type)
{
    std::vector<IOA> result;

    for (const auto& item : this->ioamap)
    {
        if (item.second.spare && (item.second.type == type))
            result.emplace_back(item.first);
    }

    return result;
}

AllIOAMap &IOAMap::getAllMAPIOA()
{
    return this->ioamap;
}

bool IOAMap::testYTSpare(const IOA& ioa)
{
    bool result = false;

    auto ita = this->ioamap.find(ioa);

    if (ita != this->ioamap.end() && (ita->second.type == IMAPT_YT)) {
        result = ita->second.spare;
    }

    return result;
}