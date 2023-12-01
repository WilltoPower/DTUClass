#include "dtusysconfig.h"
#include <dtulog.h>
#include "json/json.h"
#include <QFile>
#include <QDir>
#include <fstream>
#include <windows.h>
#include <dtucommon.h>

void dtusysconfig::load()
{
	char exeFullPath[MAX_PATH] = {}; // Full path 
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::string strFullPath = (std::string)(exeFullPath);
	int nStart = strFullPath.find_last_of(TEXT("\\"));
	std::string strexepath = strFullPath.substr(0, nStart);
	std::string fullpath = (strexepath + "\\config\\dtutool.json");
	std::ifstream ifs;
	ifs.open(fullpath);

	Json::Value info_root;
	Json::CharReaderBuilder readerbuilder;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(readerbuilder, ifs, &info_root, &errs)) {
		DTULOG(DTU_ERROR, (char*)"加载参数存储配置文件错误%s ", errs.c_str());
		return;
	}

	_tool_id = info_root["id"].asLargestUInt();
	_rpc_dest_ip = info_root["rpc"]["destip"].asString();
	_rpc_dest_port = info_root["rpc"]["destport"].asInt();
	_rpc_local_port = info_root["rpc"]["localport"].asInt();
	_rpc_local_ip = info_root["rpc"]["localip"].asString();

	_analyze_tools = info_root["analyze"].asString();

	ifs.close();
}

void dtusysconfig::save()
{
	char exeFullPath[MAX_PATH] = {}; // Full path 
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::string strFullPath = (std::string)(exeFullPath);
	int nStart = strFullPath.find_last_of(TEXT("\\"));
	std::string strexepath = strFullPath.substr(0, nStart);
	std::string fullpath = (strexepath + "\\config\\dtutool.json");

	Json::Value info_root;
	info_root["id"] = _tool_id;
	info_root["rpc"]["destip"] = _rpc_dest_ip;
	info_root["rpc"]["destport"] = _rpc_dest_port;
	info_root["rpc"]["localport"] = _rpc_local_port;
	info_root["rpc"]["localip"] = _rpc_local_ip;

	info_root["analyze"] = _analyze_tools;

	Json::StyledWriter  swriter;

	std::string str = swriter.write(info_root);
	std::ofstream ofs(fullpath);
	ofs << str;
	ofs.close();
}

const std::string& dtusysconfig::get_local_ip() const
{
	return _rpc_local_ip;
}

void dtusysconfig::set_local_ip(const std::string& ip)
{
	_rpc_local_ip = ip;
}

uint16_t dtusysconfig::get_local_port() const
{
	return _rpc_local_port;
}

void dtusysconfig::set_local_port(uint16_t port)
{
	_rpc_local_port = port;
}

const std::string& dtusysconfig::get_dest_ip() const
{
	return _rpc_dest_ip;
}

void dtusysconfig::set_dest_ip(const std::string& ip)
{
	_rpc_dest_ip = ip;
}

uint16_t dtusysconfig::get_dest_port() const
{
	return _rpc_dest_port;
}

void dtusysconfig::set_dest_port(uint16_t port)
{
	_rpc_dest_port = port;
}

uint64_t dtusysconfig::get_tool_id()
{
	return _tool_id;
}

std::string dtusysconfig::get_analyze_tools()
{
	return get_exec_dir() + "\\" + _analyze_tools;
}
