#include "dtugooseconfig.h"
#include <pugixml/pugixml.hpp>
#include "dtulog.h"
#include "dtutask_dsp.h"

using namespace pugi;
using namespace DTU;

bool dtuGooseCfg::load(const std::string& file)
{
	DTULOG(DTU_INFO,"加载Goose配置文件:%s",file.c_str());
	xml_document doc;
	if (!doc.load_file(file.c_str())) {
		DTULOG(DTU_ERROR,"加载GOOSE配置失败");
		return false;
	}

	auto gooseNode = doc.child("config").child("goose");
	// 装置本地GOOSE配置
	Goosecfg.gocbRef = gooseNode.attribute("gocbRef").as_string();
	Goosecfg.dataSet = gooseNode.attribute("dataSet").as_string();
	Goosecfg.appID = gooseNode.attribute("appID").as_uint();

	// 加载网卡信息
	auto devNode = doc.child("config").child("device");
	Goosecfg.netinterface = devNode.attribute("netinterface").as_string();
	Goosecfg.ineth = devNode.attribute("ineth").as_string();
	Goosecfg.outeth = devNode.attribute("outeth").as_string();
	DTULOG(DTU_INFO, "Goose输入网口[%s] 输出网口[%s]", Goosecfg.ineth.c_str(), Goosecfg.outeth.c_str());
	Goosecfg.mac = devNode.attribute("mac").as_string();

	// M侧装置信息
	auto mNode = devNode.child("MSIDE");
	for (auto& dev : mNode.children())
	{
		GCFGItem item;
		item.appID = dev.attribute("appID").as_int();
		item.use = dev.attribute("enable").as_bool();
		// 不使用就不添加
		if(!item.use)
			continue;

		item.faultIndex = dev.attribute("faultIndex").as_int();
		item.checkIndex = dev.attribute("checkIndex").as_int();
		item.isolateIndex = dev.attribute("isolateIndex").as_int();
		item.currentIndex = dev.attribute("currentIndex").as_int();
		item.side = GOOSE_M_SIDE;
		Goosecfg.GItems.insert({item.appID,item});
	}

	// N侧装置信息
	auto nNode = devNode.child("NSIDE");
	for (auto& dev : nNode.children())
	{
		GCFGItem item;
		item.appID = dev.attribute("appID").as_int();
		item.use = dev.attribute("enable").as_bool();
		// 不使用就不添加
		if(!item.use)
			continue;

		item.faultIndex = dev.attribute("faultIndex").as_int();
		item.checkIndex = dev.attribute("checkIndex").as_int();
		item.isolateIndex = dev.attribute("isolateIndex").as_int();
		item.currentIndex = dev.attribute("currentIndex").as_int();
		item.side = GOOSE_N_SIDE;
		Goosecfg.GItems.insert({item.appID,item});
	}

	return true;
}

const GooseCFG& dtuGooseCfg::GetGooseCFG() { return Goosecfg; }

GooseCFG& dtuGooseCfg::ModifyGooseCFG() { return Goosecfg; }

VOFFSET dtuGooseCfg::findOffset(APPID appid, GooseValueType type)
{
	VOFFSET ret = -1;
	switch (type)
	{
		// 节点故障
		case GTYPE_FAULT: {
			ret = Goosecfg.GItems[appid].faultIndex;
		};break;
		// 开关拒跳
		case GTYPE_CHECK: {
			ret = Goosecfg.GItems[appid].checkIndex;
		};break;
		// 故障隔离
		case GTYPE_ISOLATE: {
			ret = Goosecfg.GItems[appid].isolateIndex;
		};break;
		// 过流闭锁
		case GTYPE_CURRENT: {
			ret = Goosecfg.GItems[appid].currentIndex;
		};break;
	}
	return ret + 1;
}

void dtuGooseTableMgr::updateGooseValues(APPID appID, const GooseData& newValue)
{
	bool bUpdate = false;
	for (int i = 0; i < 4; i++)
	{
		auto offset = findOffsetByAppID(appID, static_cast<GooseValueType>(i));
		if(offset > 0) {
			if(GooseTable[offset] != newValue[i]) {
				GooseTable[offset] = newValue[i];
				bUpdate = true;
			}
		}
	}

	if (bUpdate) {
		updateDSPGooseValues();
		DTULOG(DTU_INFO,"下发GOOSE表");
	}
}

bool dtuGooseTableMgr::updateGooseTableFromDSP()
{
	GooseData dspValue;
	bool isRead = false;
	DTU::buffer ret;

	for(int i = 1; i<=3; i++) {
		if(DTU_SUCCESS != dsptask_execute_read(PC_R_GOOSE_TABLE_INFO, ret)) {
			DTULOG(DTU_WARN,"获取Goose表失败,重试%d/3",i);
		}
		else {
			isRead = true;
			break;
		}
	}
	
	if(isRead && (ret.size() == 32)) {
		for(int offset=0; offset<ret.size(); offset++) {
			dspValue.emplace_back(ret.get(offset,sizeof(uint8_t)).value<uint8_t>());
		}
	}
	else {
		DTULOG(DTU_ERROR,"获取GOOSE表失败,使用默认值进行初始化");
		dspValue.clear();
		dspValue.resize(32);
		dspValue[0] = 0xAA;
		dspValue[31] = 0xBB;
	}

	return updateGooseTableFromDSP(dspValue);
}

bool dtuGooseTableMgr::updateGooseTableFromDSP(const GooseAllTable& newValue)
{
	if(newValue.size() != 32) {
		DTULOG(DTU_ERROR,"GOOSE表长度错误应为32,实际为%u", newValue.size());
		return false;
	}
	if(newValue[0] != 0xAA || newValue[31] != 0xBB) {
		DTULOG(DTU_ERROR,"GOOSE格式错误");
		return false;
	}
	GooseTable = newValue;
	isTableReady = true;
	return true;
}

bool dtuGooseTableMgr::updateDSPGooseValues()
{
    if(GooseTable.size() != 32)
        return false;
    DTU::buffer buf;
    for(const auto &item : GooseTable) {
        buf.append((char*)&item,sizeof(uint8_t));
    }
    dsptask_execute_write(PC_W_GOOSE_TABLE_PKG, buf);
}

VOFFSET dtuGooseTableMgr::findOffsetByAppID(APPID appid, GooseValueType type)
{
	return dtuGooseCfg::instance().findOffset(appid,type);
}
