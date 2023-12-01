#include "dtutoolinit.h"

#define WIN32_LEAN_AND_MEAN

#include "dtuconfigure.h"
#include "dturpcclient.h"
#include "dtustorage.h"

using namespace DTUTool;

dtuToolinit::dtuToolinit()
{
	// 构造函数
}

bool dtuToolinit::init()
{
	// 载入DTUTool工具配置
	dtutoolcfg::instance().load();
	// RPC客户端配置
	auto &cfg = Get_RPC_CFG();
	dturpcclient::instance().init(cfg.ip, cfg.port);
	dturpcclient::instance().run();
	// 载入数据库配置和整定定值配置
	DTU::DSTORE::instance().load(get_exec_dir() + "\\config\\dtu.db");

	return true;
}