#include "dtuconfigure.h"
#include <dtulog.h>
#include "json/json.h"
#include <fstream>
#include "dtucommon.h"

dtutoolcfg::dtutoolcfg()
{
	// ����·��
	_fullPath = get_exec_dir() + "\\config\\dtutool.json";
}

void dtutoolcfg::load()
{
	std::ifstream ifs;
	ifs.open(_fullPath);

	Json::Value root;
	Json::CharReaderBuilder readerbuilder;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(readerbuilder, ifs, &root, &errs)) {
		DTULOG(DTU_ERROR, (char*)"���ز����洢�����ļ�����%s ", errs.c_str());
		return;
	}

	_rpc.ip = root["rpc"]["rpcip"].asString();
	_rpc.port = root["rpc"]["rpcport"].asInt();

	_analyzeToolPath = root["analyze"].asString();

	ifs.close();
}

void dtutoolcfg::save()
{
	Json::Value root;
	root["rpc"]["rpcip"] = _rpc.ip;
	root["rpc"]["rpcport"] = _rpc.port;

	root["analyze"] = _analyzeToolPath;

	// �����ֽ���
	Json::StreamWriterBuilder WriterBuilder;
	std::ostringstream os;
	std::unique_ptr<Json::StreamWriter> JsonWriter(WriterBuilder.newStreamWriter());
	JsonWriter->write(root, &os);
	std::string str = os.str();

	std::ofstream ofs(_fullPath);
	ofs << str;
	ofs.close();
}

const DTUCFG::EthernetCFG &dtutoolcfg::GetRPCCFG() { return _rpc; }

DTUCFG::EthernetCFG &dtutoolcfg::ModifyRPCCFG() { return _rpc; }

const std::string dtutoolcfg::GetAnalyzeToolPath() 
{
	return get_exec_dir() + _analyzeToolPath;
}