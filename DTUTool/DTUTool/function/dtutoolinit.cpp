#include "dtutoolinit.h"

#define WIN32_LEAN_AND_MEAN

#include "dtuconfigure.h"
#include "dturpcclient.h"
#include "dtustorage.h"

using namespace DTUTool;

dtuToolinit::dtuToolinit()
{
	// ���캯��
}

bool dtuToolinit::init()
{
	// ����DTUTool��������
	dtutoolcfg::instance().load();
	// RPC�ͻ�������
	auto &cfg = Get_RPC_CFG();
	dturpcclient::instance().init(cfg.ip, cfg.port);
	dturpcclient::instance().run();
	// �������ݿ����ú�������ֵ����
	DTU::DSTORE::instance().load(get_exec_dir() + "\\config\\dtu.db");

	return true;
}