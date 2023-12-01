#include "pugixml/pugixml.hpp"

#include "dutxmlgenerate.h"
#include "dtulog.h"

using namespace pugi;
using namespace DTU;

bool dtuGooseCFGPlus::load(const std::string& filePath)
{
	DTULOG(DTU_INFO, "加载GoosePlus配置文件:%s", filePath.c_str());
	xml_document doc;
	xml_parse_result res = doc.load_file(filePath.c_str());

	if (res.status != xml_parse_status::status_ok) {
		DTULOG(DTU_ERROR, "加载GoosePlus配置文件%s错误 %s", filePath.c_str(), res.description());
		return false;
	}

	DevGoosePlus.clear();
	auto gooseNode = doc.child("AllConfigure");
	for (auto &dev : gooseNode.children())
	{
		OneDevGoosePlus oneDevGoosePlus;
		// 获取devno、appid、mac
		oneDevGoosePlus.devno = dev.attribute("no").as_int();
		oneDevGoosePlus.appid = dev.attribute("appID").as_int();
		oneDevGoosePlus.mac = dev.attribute("mac").as_string();

		// 内外网卡
		oneDevGoosePlus.ineth = dev.attribute("ineth").as_string();
		oneDevGoosePlus.outeth = dev.attribute("outeth").as_string();

		// MSIDE 
		auto msideNode = dev.child("MSIDE");
		for (auto &net : msideNode.children())
		{
			// 获取enable、appID
			GoosePlusSide gooseSide;
			gooseSide.use = net.attribute("enable").as_bool();
			gooseSide.appid = net.attribute("appID").as_int();
			oneDevGoosePlus.mside.push_back(gooseSide);
		}
		// NSIDE
		auto nsideNode = dev.child("NSIDE");
		for (auto &net : nsideNode.children())
		{
			// 获取enable、appID
			GoosePlusSide gooseSide;
			gooseSide.use = net.attribute("enable").as_bool();
			gooseSide.appid = net.attribute("appID").as_int();
			oneDevGoosePlus.nside.push_back(gooseSide);
		}
		// add to map
		DevGoosePlus[oneDevGoosePlus.devno] = oneDevGoosePlus;
	}
	return true;
}

std::map<int, OneDevGoosePlus>& dtuGooseCFGPlus::config()
{
	return this->DevGoosePlus;
}