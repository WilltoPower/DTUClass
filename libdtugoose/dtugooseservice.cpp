#include <iostream>
#include "dtugooseservice.h"
#include "dtugooseconfig.h"
#include "dtugoosepublisher.h"
#include "dtugoosesubscriber.h"
#include "dtucommon.h"
#include "dtulog.h"

using namespace DTU;

bool dtuGooseService::init()
{
	DTULOG(DTU_INFO,"初始化Goose表");
	dtuGooseTableMgr::instance().updateGooseTableFromDSP();
	// 初始化配置
	std::string strexepath = get_exec_dir();
	dtuGooseCfg::instance().load(strexepath + "/config/GooseConfig.xml");
	// 加载参数
	DTULOG(DTU_INFO,"初始化Goose发布服务");
	if(!dtuGoosePublisherMgr::instance().load(dtuGooseCfg::instance().GetGooseCFG())) {
		DTULOG(DTU_ERROR,"初始化Goose发布服务失败");
		return false;
	}
	DTULOG(DTU_INFO,"初始化Goose订阅服务");
	if(!dtuGooseSubscriber::instance().init(dtuGooseCfg::instance().GetGooseCFG())) {
		DTULOG(DTU_ERROR,"初始化Goose订阅服务失败");
		return false;
	}
	return true;
}

bool dtuGooseService::run()
{
	DTULOG(DTU_INFO,"运行Goose服务");
	dtuGoosePublisherMgr::instance().run();
	dtuGooseSubscriber::instance().run();
	return true;
}

bool dtuGooseService::stop()
{
	DTULOG(DTU_INFO,"GOOSE服务退出");
	dtuGoosePublisherMgr::instance().stop();
	dtuGooseSubscriber::instance().stop();
	return true;
}
