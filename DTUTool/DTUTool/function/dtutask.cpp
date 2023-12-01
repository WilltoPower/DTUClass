#pragma execution_character_set("utf-8")

#include "dtutask.h"

#include <atomic>

#include "rest_rpc/rest_rpc.hpp"
#include "dtuerror.h"
#include "dtulog.h"
#include "dtuconfigure.h"
#include "dtucommon.h"

#define EXE_RPC_TIMEOUT 500
// 最大传输帧长度(这里限制为9MB,实际为10MB)
#define MAX_TRANS_SIZE 9437184

using namespace DTUCFG;
using namespace DTU;

int execute_write_setting(uint16_t cmd, const DTU::buffer& data, uint16_t group)
{
	int retCode = 0;
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_write_setting 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		retCode = client.call<int>("rpc_write_setting", cmd, data, group);
		return retCode;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_write 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_read_setting(uint16_t cmd, DTU::buffer& result, uint16_t group)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_read_setting 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		result = client.call <DTU::buffer>("rpc_read_setting", cmd, group);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_read 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_query_data(uint16_t cmd, DTU::buffer& result)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_query_data 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		result = client.call<DTU::buffer>("rpc_read_parameter", cmd);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_query 未知错误:%s", e.what());
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_read_group 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
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
		DTULOG(DTU_ERROR, (char*)"execute_read_group 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_write_data(uint16_t cmd, const DTU::buffer& result)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_write_data 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		return client.call<int>("rpc_write_parameter", cmd, result);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_write_data 未知错误:%s", e.what());
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_file 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		result = client.call<DTU::buffer>("rpc_get_file", fileName);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_get_file 未知错误:%s", e.what());
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_set_file 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		return client.call<int>("rpc_set_file", destName, fileContent);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_set_file 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;

}

int execute_set_file(const std::string& destName, const DTU::buffer& fileContent)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_set_file 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		return client.call<int>("rpc_set_file", destName, fileContent);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_set_file 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_dir(const std::string& dirName, uint64_t begin, uint64_t end, FILELIST& result)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_dir 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		result = client.call<FILELIST>("rpc_get_dir", dirName, begin, end);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_get_dir 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_rmctrl(uint16_t fix, uint16_t operate, int delay, int from)
{
	int retcode;
	try
	{
		RemoteCtrlInfo info;
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_rmctrl 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		info.cmdFrom = from;
		info.delay = delay;
		auto result = client.call<int>("rpc_rmctrl", fix, operate, info);
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_change_group 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		auto result = client.call<int>("rpc_change_group", dst);
		return result;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_read 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
// 获取报告
int execute_get_report(uint16_t id, int min, int max, DTU::ReportBufferAttr &result)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_report 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		result = client.call<DTU::ReportBufferAttr>("rpc_get_report", id, min, max);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_get_report 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_reportno(uint16_t id, uint32_t &reportno)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_reportno 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		reportno = client.call<uint32_t>("rpc_get_reportno", id);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_get_reportno 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
// 报告清空
int execute_clear_report(uint16_t id)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_clear_report 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		//
		return client.call<int>("rpc_clear_report", id);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"rpctask_execute_clear 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}
// 执行升级
int execute_updateprogram(uint16_t tag)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_updateprogram 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		return client.call<int>("rpc_update", tag);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_updateprogram 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_disksuage(Disk_info &usage, uint16_t tag)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_disksuage 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		usage = client.call<Disk_info>("rpc_disksuage", tag);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_get_disksuage 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_read_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_read_goose_cfg 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		gcfg = client.call<DTUCFG::DSYSCFG::GooseCFG>("rpc_read_goose_cfg");
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_read_goose_cfg 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_save_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_save_goose_cfg 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		bool ret = client.call<bool>("rpc_save_goose_cfg", gcfg);
		if (ret)
			return DTU_SUCCESS;
		else
			return DTU_RPC_ERROR;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_save_goose_cfg 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_filepath(std::string &Path)
{
	try
	{
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_filepath 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		DTU::buffer ret = client.call<DTU::buffer>("rpc_filepath");
		std::string buf(ret.const_data(), ret.size());
		Path = buf;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_get_filepath 未知错误:%s", e.what());
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_fixno_check 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		ret = client.call<bool>("rpc_fixno_check", static_cast<int>(type), fixno);
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_fixno_check 未知错误:%s", e.what());
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
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_fixno_check 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
			return DTU_RPC_ERROR;
		}
		ret = client.call<bool>("rpc_fixno_modify", static_cast<int>(type), older, newer);
		if (ret) {
			DTU::DBManager::instance().ModifyFixno(static_cast<DTU::MapFixno>(type), older, newer);
		}
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "execute_fixno_check 未知错误:%s", e.what());
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

int execute_set_file_plus(const std::string& srcName, const std::string& destName)
{
	try
	{
		DTU::buffer fileContent;
		get_file(srcName, fileContent);
		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_set_file_plus 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}
		uint64_t fileSize = fileContent.size();
		uint64_t fileCount = 0;
		for (;;fileCount += MAX_TRANS_SIZE)
		{
			if (fileCount + MAX_TRANS_SIZE < fileSize) {
				// 未传输完成
				client.call<int>("rpc_set_file_plus", destName, fileContent.get(fileCount, MAX_TRANS_SIZE), false);
			}
			else {
				// 传输完成
				client.call<int>("rpc_set_file_plus", destName, fileContent.get(fileCount, fileSize - fileCount), true);
				break;
			}
		}
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_set_file_plus 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}

int execute_get_file_plus(const std::string& srcName, const std::string& destName)
{
	try
	{
		DTU::buffer fileContent;

		auto cfg = Get_RPC_CFG();
		rest_rpc::rpc_client client(cfg.ip, cfg.port);
		client.set_connect_timeout(EXE_RPC_TIMEOUT);
		if (!client.connect()) {
			DTULOG(DTU_ERROR, "execute_get_file_plus 无法链接:%s %u", cfg.ip.c_str(), cfg.port);
		}

		// 获取文件大小
		uint64_t fileSize = client.call<uint64_t>("rpc_get_file_size", srcName);
		uint64_t fileCount = 0;
		
		// 获取文件路径
		DTU::buffer ret = client.call<DTU::buffer>("rpc_filepath");
		std::string filePath(ret.const_data(), ret.size());
		
		for (;; fileCount += MAX_TRANS_SIZE)
		{
			if (fileCount + MAX_TRANS_SIZE < fileSize) {
				// 未传输完成
				DTU::buffer tempdata = client.call<DTU::buffer>("rpc_get_file_plus", srcName, fileCount, MAX_TRANS_SIZE);
				fileContent.append(tempdata);
			}
			else {
				// 传输完成
				DTU::buffer tempdata = client.call<DTU::buffer>("rpc_get_file_plus", srcName, fileCount, fileSize-fileCount);
				fileContent.append(tempdata);
				break;
			}
		}

		if (fileContent.size() == fileSize) {
			save_file(destName, fileContent);
			DTULOG(DTU_INFO, "文件[%s]接收成功", destName.c_str());
		}
		else {
			DTULOG(DTU_ERROR, "文件[%s]接收失败,文件大小[%lu],实际接收大小[%lu]", destName.c_str(),fileSize, fileContent.size());
		}

	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, (char*)"execute_get_file_plus 未知错误:%s", e.what());
		return DTU_UNKNOWN_ERROR;
	}
	return DTU_SUCCESS;
}