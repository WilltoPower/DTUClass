#include "dtutask.h"
#include "dtusysconfig.h"
#include <dtuerror.h>
#include <dtulog.h>
#include <rest_rpc/rest_rpc.hpp>
#include "dtutask_notify.h"
#include "dtucmdcode.h"
#include "dtucommon.h"
#include <atomic>

using namespace DTU;

int execute_write_setting(uint16_t cmd, const DTU::buffer& data, uint16_t group)
{
	int retCode = 0;
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_write_setting 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		retCode = client.call<int>("rpc_write_setting", cmd, data, group);
		return retCode;
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR,(char*)"execute_write 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_read_setting(uint16_t cmd, DTU::buffer& result, uint16_t group)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_read_setting 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		result = client.call <DTU::buffer> ("rpc_read_setting", cmd, group);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_read 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}


int execute_query_data(uint16_t cmd, DTU::buffer& result)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_query_data 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		result = client.call<DTU::buffer>("rpc_read_parameter", cmd);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_query 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	if (result.size() > 0)
	{
		return DTU_SUCCESS;
	}
	return DTU_RPC_ERROR;
}

int execute_read_group(uint32_t& curgroup, uint32_t& maxgroup)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_read_group 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		//
		auto result = client.call<DTU::buffer>("rpc_read_group");
		if (result.size() == 8) {
			curgroup = result.get(0, sizeof(uint32_t)).value<uint32_t>();
			maxgroup = result.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
		}
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_read_group 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_write_data(uint16_t cmd, const DTU::buffer& result)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_write_data 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		return client.call<int>("rpc_write_parameter", cmd, result);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_write_data 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	if (result.size() > 0)
	{
		return DTU_SUCCESS;
	}
	return DTU_RPC_ERROR;
}

int execute_get_file(const std::string& fileName, DTU::buffer& result)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_file 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		result = client.call<DTU::buffer>("rpc_get_file", fileName);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_get_file 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_set_file(const std::string& srcName, const std::string& destName)
{
	try
	{
		DTU::buffer fileContent;
		get_file(srcName, fileContent);

		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_set_file 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		return client.call<int>("rpc_set_file", destName, fileContent);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_set_file 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
	
}

int execute_set_file(const std::string& destName, const DTU::buffer& fileContent)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_set_file 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		return client.call<int>("rpc_set_file", destName, fileContent);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_set_file 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;

}

int execute_get_dir(const std::string& dirName, uint64_t begin, uint64_t end, FILELIST& result)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_dir 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		result = client.call<FILELIST>("rpc_get_dir", dirName, begin, end);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_get_dir 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_rmctrl(uint16_t fix, uint16_t operate,int delay,int from)
{
	int retcode;
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_rmctrl 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		auto result = client.call<int>("rpc_rmctrl", fix, operate, delay, from);
		return result;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_read 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_change_group(uint16_t dst)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_change_group 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		auto result = client.call<int>("rpc_change_group", dst);
		return result;
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_read 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
// 获取报告
int execute_get_report(uint16_t id, int min, int max, DTU::ReportBufferAttr &result)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_report 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		result = client.call<DTU::ReportBufferAttr>("rpc_get_report", id,min,max);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_get_report 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
int execute_get_reportno(uint16_t id, uint32_t &reportno)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_reportno 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		reportno = client.call<uint32_t>("rpc_get_reportno", id);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"execute_get_reportno 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
// 报告清空
int execute_clear_report(uint16_t id)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_clear_report 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
		}
		//
		return client.call<int>("rpc_clear_report", id);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, (char*)"rpctask_execute_clear 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

// 执行升级
int execute_updateprogram(uint16_t tag)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(10000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_updateprogram 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		return client.call<int>("rpc_update", tag);
	}
	catch (std::exception& e)
	{
        QtDTULOG(DTU_ERROR, "execute_updateprogram 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_disksuage(Disk_info &usage, uint16_t tag)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_disksuage 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		usage =  client.call<Disk_info>("rpc_disksuage", tag);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_get_disksuage 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_read_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &cfg)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(100);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_read_goose_cfg 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		cfg = client.call<DTUCFG::DSYSCFG::GooseCFG>("rpc_read_goose_cfg");
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_read_goose_cfg 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_save_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &cfg)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(100);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_save_goose_cfg 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		bool ret = client.call<bool>("rpc_save_goose_cfg",cfg);
		if (ret)
			return DTU_SUCCESS;
		else
			return DTU_RPC_ERROR;
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_save_goose_cfg 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_filepath(std::string &Path)
{
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_get_filepath 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		DTU::buffer ret = client.call<DTU::buffer>("rpc_filepath");
		std::string buf(ret.const_data(),ret.size());
		Path = buf;
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_get_filepath 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

// 检查点表是否可以修改
bool execute_fixno_check(DTU::MapFixno type, uint16_t fixno) 
{
	bool ret = false;
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_fixno_check 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		ret = client.call<bool>("rpc_fixno_check",static_cast<int>(type),fixno);
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_fixno_check 未知错误:%s", e.what());
		return false;
	}
	return ret;
}

// 修改点表
bool execute_fixno_modify(DTU::MapFixno type, uint16_t older, uint16_t newer)
{
	bool ret = false;
	try
	{
		rest_rpc::rpc_client client(DESTIP(), DESTPORT());
		client.set_connect_timeout(1000);
		if (!client.connect()) {
			QtDTULOG(DTU_ERROR, "execute_fixno_check 无法链接:%s %u", DESTIP().c_str(), DESTPORT());
			return DTU_RPC_ERROR;
		}
		ret = client.call<bool>("rpc_fixno_modify", static_cast<int>(type), older, newer);
		if (ret) {
			DTU::DBManager::instance().ModifyFixno(static_cast<DTU::MapFixno>(type), older, newer);
		}
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "execute_fixno_check 未知错误:%s", e.what());
		return false;
	}
	return ret;
}

std::string toLocalStr(QString str)
{
	return str.toLocal8Bit().toStdString();
}


static std::atomic_bool sstate;

bool execute_test_arm_connect()
{
	return sstate;
}

void set_arm_connect_state(bool state)
{
	sstate = state;
}