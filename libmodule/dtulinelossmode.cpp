/*********************************************************************************
	*Copyright(C),2021-2025,sddl
	*FileName:  dtulinelossmode.cpp
	*Description: 
		用于实现线损模块功能
	*History: 
		1, 创建, lhy, 2022-08-10
**********************************************************************************/
#include "dtulinelossmode.h"
#include <dtu101master.h>

#include "dtulog.h"

using namespace DTU;

dtuLinelossmode::dtuLinelossmode()
{
	//TODO从配置文件获取 CA地址
	CA = 1;
}

dtuLinelossmode::~dtuLinelossmode() {}

bool dtuLinelossmode::clockSync()
{
	DTULOG(DTU_INFO, "F-PIC100时间校准");
	return D101Master::instance().dtu_clockSyncHandler(CA);
}

void dtuLinelossmode::callElecEnergy()
{
	DTULOG(DTU_INFO, "召唤电能量数据");
	// 电能量总召唤
	D101Master::instance().send_elec_interrogation_cmd(CA);
}