/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtusystemconfig.cpp
  *Description:
    101/104相关的配置
  *History:
    1, 创建, wangjs, 2021-7-8
    2, 实现对101和104相关参数的读取, wangjs, 2021-7-9
    3, 加入101/104的是否使用开关, wangjs, 2021-8-3
    4, 加入配置工具RPC通信的端口, wangjs, 2021-8-4
**********************************************************************************/
#ifdef _WIN32

#pragma execution_character_set("utf-8")

#endif

#include "dtusystemconfig.h"
#include <dtuerror.h>
#include <fstream>
#include <sstream>
#include "json/json.h"
#include <dtulog.h>
#include <pugixml/pugixml.hpp>
#include "dtucommon.h"
#include "dtudbmanager.h"

using namespace DTUCFG;
using namespace DTU;
using namespace std;

#define DEV_PUBLIC 0

void DSYSCFG::load()
{
    DTULOG(DTU_INFO, (char *)"加载所有配置...");
    std::string fileBase = get_exec_dir();
    // 加载规约配置(一定最先加载)
    this->load_cs_cfg(fileBase + "/config/csprotocol.json");
    // 加载系统配置
    this->load_sys_cfg(fileBase + "/config/syscfg.json");
    // 加载Goose配置
    this->load_goose_cfg(fileBase + "/config/GooseConfig.xml");

    // (等待所有的配置都加载完成)设置点表映射值
    if (this->UCFG.type == DSYSCFG::MODE_PUB)
        DTU::DBManager::instance().SetDevNo(DEV_PUBLIC); // 公共单元默认设置为0
    else
        DTU::DBManager::instance().SetDevNo(DEV_PUBLIC); // 设置设备号现在设备号默认设置为0,更改为硬件HIOA
}

bool DSYSCFG::load_sys_cfg(const std::string &fullPath)
{
    DTULOG(DTU_INFO, (char*)"加载系统配置:%s...", fullPath.c_str());
    DTU::buffer data;
    get_file(fullPath, data);
    sysunpack(data);
    return true;
}

bool DSYSCFG::sysunpack(DTU::buffer &data)
{
    DTU_USER();

    std::istringstream jsonsstream(std::string(data.data(), data.size()));

    Json::Value root;
    Json::CharReaderBuilder readerbuilder;
    JSONCPP_STRING errs;

    if (!Json::parseFromStream(readerbuilder, jsonsstream, &root, &errs)) {
        DTU_THROW((char *)"加载系统配置文件错误 %s", errs.c_str());
    }

    // 单元类型
    UCFG.type = static_cast<UNITTYPE>(root["unittype"].asInt());

    // 读取本地RPC配置
    RPCCFG.port = root["localrpcport"].asInt();     // 端口号

    // 读取设备号(1~7)
    this->devno = root["devno"].asInt();

    // CS101链路地址
    this->linkAddr = root["linkaddr"].asInt();

    this->piclinkaddr = root["piclinkaddr"].asInt();

    // 读取设备IED名称
    this->iedName = root["iedName"].asString();

    // 读取LCD串口配置
    Json::Value Jmcu = root["mcu"];
    LCDCFG.baudrate = Jmcu["baudrate"].asInt();
    LCDCFG.name = Jmcu["serial"].asString();

    // FPIC100线损模块配置
    Json::Value Jpic = root["pic100"];
    Json::Value J101 = Jpic["CS101"];
    LLCFG.use = Jpic["used"].asBool();
    this->parse_cs101_from_Json(LLCFG.proto, J101, JSON_FLAG_MANUAL, LLCFG.use);

    // 自动校时配置
    Json::Value Jsync = root["autotime"];
    STCFG.use = Jsync["used"].asBool();
    STCFG.timeInSec = Jsync["interval"].asInt();

    // 单元配置
    if(isPublic()) {
        // 公共单元模式 --加载间隔单元配置
        Json::Value remoteArray = root["bay"]["remote"];
		printf("-----------------------\n");
        for(auto &item : remoteArray)
        {
			printf("Read [%d]\n", item["asdu"].asInt());
            OneBAYCFG cfg;
            cfg.use = item["used"].asBool();
            cfg.ca = item["asdu"].asInt();
            cfg.proto = static_cast<UNITPROTOCOL>(item["protocol"].asInt());

            this->parse_cs101_from_Json(cfg.ProroCS.CS101, item["CS101"], JSON_FLAG_NO_USE);
            this->parse_cs104_from_Json(cfg.ProroCS.CS104, item["CS104"], JSON_FLAG_NO_USE);
                
            cfg.ProtoRPC.ip = item["RPC"]["ip"].asString();
            cfg.ProtoRPC.port = item["RPC"]["port"].asInt();

            UCFG.info.insert({cfg.ca, cfg});
#ifndef _WIN32
            DTULOG(DTU_INFO, (char*)"间隔单元[%02d] [%s]", cfg.ca, cfg.use ? (const char*)"use" : (const char*)"unuse");
#endif
        }
    }
    else {
        // 间隔单元模式 --加载公共单元配置
        Json::Value Jpub = root["public"];

        PUBCFG.ca = Jpub["destasdu"].asInt();
        PUBCFG.proto = static_cast<UNITPROTOCOL>(Jpub["protocol"].asInt());
        PUBCFG.ProroCS = IECCFG.Slave; // 从规约获取配置
        PUBCFG.ProtoRPC.ip = Jpub["rpcip"].asString();
        PUBCFG.ProtoRPC.port = Jpub["rpcport"].asInt();
    }

    // mac地址
    Json::Value JMac = root["mac"];
    this->MacCFG.clear();
    for (const auto &item : JMac)
    {
        MACCFG cfg;
        cfg.use = item["use"].asBool();
        cfg.mac = item["mac"].asString();
        cfg.eth = item["eth"].asString();

        this->MacCFG.emplace_back(cfg);
    }

#ifndef _WIN32
    fulshMAC();
#endif

    return true;
}

DTU::buffer DSYSCFG::syspack()
{
    Json::Value root;

    Json::Value &Jutype = root["unittype"];
    // 单元类型
    Jutype = static_cast<int>(UCFG.type);

    // 设备标识号
    Json::Value &Jdevno = root["devno"];
    Jdevno = devno;

    // 链路地址
    Json::Value &Jlinkaddr = root["linkaddr"];
    Jlinkaddr = this->linkAddr;

    Json::Value &Jpiclinkaddr = root["piclinkaddr"];
    Jpiclinkaddr = this->piclinkaddr;

    // 设备IED名称
    Json::Value &JiedName = root["iedName"];
    JiedName = this->iedName;

    // 读取本地RPC配置
    Json::Value &Jrpc = root["localrpcport"];
    Jrpc = RPCCFG.port;  // 端口号

    // 读取LCD串口配置
    Json::Value &Jmcu = root["mcu"];
    Jmcu["baudrate"] = LCDCFG.baudrate;
    Jmcu["serial"] = LCDCFG.name;

    // FPIC100线损模块配置
    Json::Value &Jpic = root["pic100"];
    Json::Value &J101 = Jpic["CS101"];
    Jpic["used"] = LLCFG.use;
    this->parse_cs101_to_Json(LLCFG.proto, J101, JSON_FLAG_MANUAL, LLCFG.use);

    // 自动校时配置
    Json::Value &Jsync = root["autotime"];
    Jsync["used"] = STCFG.use;
    Jsync["interval"] = STCFG.timeInSec;

    // 公共单元配置
    Json::Value &Jpub = root["public"];
    Jpub["destasdu"] = PUBCFG.ca;
    Jpub["protocol"] = static_cast<int>(PUBCFG.proto);
    Jpub["rpcip"] = PUBCFG.ProtoRPC.ip;
    Jpub["rpcport"] = PUBCFG.ProtoRPC.port;

    // PUBCFG.ProroCS = IECCFG.Slave; // 从规约获取配置 // FIXME:配置可以同步更改

    // 间隔单元配置
    Json::Value &remoteArray = root["bay"]["remote"];
	printf("-----------------------\n");
    for(auto &item : UCFG.info)
    {
		printf("Write [%d]\n", item.second.ca);
        Json::Value appenditem;

        appenditem["used"] = item.second.use;
        appenditem["asdu"] = item.second.ca;
        appenditem["protocol"] = static_cast<int>(item.second.proto);

        this->parse_cs101_to_Json(item.second.ProroCS.CS101, appenditem["CS101"], JSON_FLAG_NO_USE);
        this->parse_cs104_to_Json(item.second.ProroCS.CS104, appenditem["CS104"], JSON_FLAG_NO_USE);

        appenditem["RPC"]["ip"] = item.second.ProtoRPC.ip;
        appenditem["RPC"]["port"] = item.second.ProtoRPC.port;

        remoteArray.append(appenditem);
    }

    Json::Value &JMac = root["mac"];
    for (const auto& item : this->MacCFG)
    {
        Json::Value appenditem;

        appenditem["use"] = item.use;
        appenditem["eth"] = item.eth;
        appenditem["mac"] = item.mac;

        JMac.append(appenditem);
    }

    // 生成字节流
    Json::StreamWriterBuilder WriterBuilder;
    std::ostringstream os;
    std::unique_ptr<Json::StreamWriter> JsonWriter(WriterBuilder.newStreamWriter());
    JsonWriter->write(root, &os);
    std::string str = os.str();
    DTU::buffer result(str.c_str(), str.size());
    return std::move(result);
}

bool DSYSCFG::syssave(std::string &file)
{
    DTU::buffer data = syspack();
    save_file(file, data);
    return true;
}

bool DSYSCFG::load_goose_cfg(const std::string &fullPath)
{
    DTULOG(DTU_INFO,"加载Goose配置:%s...",fullPath.c_str());
    this->GOOSECFG = read_from_file();
	return true;
}

bool DSYSCFG::load_cs_cfg(const std::string &fullPath)
{
    DTULOG(DTU_INFO,"加载101/104规约配置:%s...", fullPath.c_str());
	DTU::buffer data;
	get_file(fullPath, data);
	csunpack(data);
	return true;
}


const DSYSCFG::GooseCFG &DSYSCFG::read_from_file()
{
    DTULOG(DTU_INFO, (char*)"读取GOOSE配置");
    std::string file = get_exec_dir() + "/config/GooseConfig.xml";

    pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(file.c_str(), pugi::parse_default, pugi::encoding_utf8);

	if (result.status != pugi::xml_parse_status::status_ok) {
        DTULOG(DTU_ERROR,(char*)"read_from_file()加载GOOSE配置文件%s错误 %s", file.c_str(), result.description());
		return GOOSECFG;
	}

    pugi::xml_node root = doc.first_child();

    // 本机信息
    pugi::xml_node localNode = root.child("goose");
 	GOOSECFG.appid = localNode.attribute("appID").as_int();

    pugi::xml_node deviceNode = root.child("device");
    GOOSECFG.mac = deviceNode.attribute("mac").as_string();
	// GOOSECFG.eth = deviceNode.attribute("netinterface").as_string();
    GOOSECFG.ineth = deviceNode.attribute("ineth").as_string();
    GOOSECFG.outeth = deviceNode.attribute("outeth").as_string();
    DTULOG(DTU_INFO, "DSYSTEM::GOOSE配置入网卡[%s] 出网卡[%s]", GOOSECFG.ineth.c_str(), GOOSECFG.outeth.c_str());

    // M侧信息
    pugi::xml_node msideNode = deviceNode.child("MSIDE");
    for(auto &item : msideNode)
    {
        oneGooseItem cfg;
        cfg.use = item.attribute("enable").as_bool();
        cfg.appid = item.attribute("appID").as_int();
        GOOSECFG.mside.emplace_back(cfg);
    }

    // N侧信息
    pugi::xml_node nsideNode = deviceNode.child("NSIDE");
    for(auto &item : nsideNode)
    {
        oneGooseItem cfg;
        cfg.use = item.attribute("enable").as_bool();
        cfg.appid = item.attribute("appID").as_int();
        GOOSECFG.nside.emplace_back(cfg);
    }

    return GOOSECFG;
}

bool DSYSCFG::save_to_file(const GooseCFG &data, bool iskill)
{
    DTULOG(DTU_INFO, (char*)"更新GOOSE配置");
    std::string file = get_exec_dir() + "/config/GooseConfig.xml";

    pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(file.c_str(), pugi::parse_default, pugi::encoding_utf8);

	if (result.status != pugi::xml_parse_status::status_ok) {
        DTULOG(DTU_ERROR,(char*)"save_to_file()加载GOOSE配置文件%s错误 %s", file.c_str(), result.description());
		return false;
	}

    pugi::xml_node root = doc.first_child();

    // 本机信息
    pugi::xml_node localNode = root.child("goose");
    localNode.attribute("appID") = data.appid;

    pugi::xml_node deviceNode = root.child("device");
    deviceNode.attribute("mac") = data.mac.c_str();
    // deviceNode.attribute("netinterface") = data.eth.c_str();
    deviceNode.attribute("ineth") = data.ineth.c_str();
    deviceNode.attribute("outeth") = data.outeth.c_str();

    int i = 0;
    // M侧信息
    pugi::xml_node msideNode = deviceNode.child("MSIDE");
    for(auto &item : msideNode)
    {
        item.attribute("enable") = data.mside[i].use;
        item.attribute("appID") = data.mside[i].appid;
        i++;
    }


    i = 0;
    // N侧信息
    pugi::xml_node nsideNode = deviceNode.child("NSIDE");
    for(auto &item : nsideNode)
    {
        item.attribute("enable") = data.nside[i].use;
        item.attribute("appID") = data.nside[i].appid;
        i++;
    }

    if (!doc.save_file(file.c_str())) {
        DTULOG(DTU_ERROR, (char*)"保存GOOSE配置失败");
        return false;
    }

    if (iskill) {
        // 重启程序以适应改变
        std::string cmd = "ps -ef | grep sdl9200 | grep -v grep | awk '{print $1}' | xargs kill -9;";
        system(cmd.c_str());
    }

    return true;
}

bool DSYSCFG::parse_cs101_from_Json(CS101Cfg &cfg, Json::Value &jitem, JsonParseFlag flag, bool use)
{
    switch(flag)
    {
        case JSON_FLAG_DEFALUT: {
            cfg.use = jitem["used"].asBool();
        };break;
        case JSON_FLAG_NO_USE: {
            cfg.use = true;
        };break;
        case JSON_FLAG_MANUAL: {
            cfg.use = use;
        };break;
    }

    cfg.mode = static_cast<LinkLayerMode>(jitem["mode"].asInt());
    cfg.otheraddr = jitem["otheraddr"].asInt();
    cfg.serial.name = jitem["serial"].asString();
    cfg.serial.name.erase(0,5);
    cfg.serial.baudrate = jitem["baudrate"].asInt();
    cfg.serial.databits = jitem["databits"].asInt();
    cfg.serial.pairty = jitem["pairty"].asInt();
    cfg.serial.stopbits = jitem["stopbits"].asInt();

    cfg.VType.MeasuredValueType = static_cast<MEASURED_TYPE>(jitem["AppLayerParameters"]["MeasuredValueType"].asInt());
    cfg.VType.TelegramValueType = static_cast<TELEGRAM_TYPE>(jitem["AppLayerParameters"]["TelegramValueType"].asInt());

    cfg.ALParam.sizeofCOT = jitem["AppLayerParameters"]["sizeOfCOT"].asInt();
    cfg.ALParam.sizeofCA = jitem["AppLayerParameters"]["sizeOfCA"].asInt();
    cfg.ALParam.sizeofIOA = jitem["AppLayerParameters"]["sizeOfIOA"].asInt();

    cfg.LLParam.LinkAddrLength = jitem["LinkLayerParameters"]["LinkAddrLength"].asInt();
    cfg.LLParam.SingalCharACK = jitem["LinkLayerParameters"]["SingalCharACK"].asBool();
    cfg.LLParam.TimeoutForACK = jitem["LinkLayerParameters"]["TimeoutForACK"].asInt();
    cfg.LLParam.TimeoutForRepeat = jitem["LinkLayerParameters"]["TimeoutForRepeat"].asInt();

    cfg.EXParam.isSequence = jitem["ExtraParameters"]["isSequence"].asBool();

	return true;
}

bool DSYSCFG::parse_cs104_from_Json(CS104Cfg &cfg, Json::Value &jitem, JsonParseFlag flag, bool use)
{
    switch(flag)
    {
        case JSON_FLAG_DEFALUT: {
            cfg.use = jitem["used"].asBool();
        };break;
        case JSON_FLAG_NO_USE: {
            cfg.use = true;
        };break;
        case JSON_FLAG_MANUAL: {
            cfg.use = use;
        };break;
    }

	cfg.eth.ip = jitem["localIP"].asString();
	cfg.eth.port = jitem["port"].asInt();

	cfg.VType.MeasuredValueType = static_cast<MEASURED_TYPE>(jitem["AppLayerParameters"]["MeasuredValueType"].asInt());
	cfg.VType.TelegramValueType = static_cast<TELEGRAM_TYPE>(jitem["AppLayerParameters"]["TelegramValueType"].asInt());

	cfg.ALParam.sizeofCOT = jitem["AppLayerParameters"]["sizeOfCOT"].asInt();
	cfg.ALParam.sizeofCA = jitem["AppLayerParameters"]["sizeOfCA"].asInt();
	cfg.ALParam.sizeofIOA = jitem["AppLayerParameters"]["sizeOfIOA"].asInt();

	cfg.EXParam.isSequence = jitem["ExtraParameters"]["isSequence"].asBool();

	return true;
}

bool DSYSCFG::parse_cs101_to_Json(CS101Cfg &cfg, Json::Value &jitem, JsonParseFlag flag, bool use)
{
    switch(flag)
    {
        case JSON_FLAG_DEFALUT: {
            jitem["used"] = cfg.use;
        };break;
        case JSON_FLAG_NO_USE: {
            // 不做处理(无此项)
        };break;
        case JSON_FLAG_MANUAL: {
            jitem["used"] = use;
        };break;
    }

	jitem["mode"] = static_cast<int>(cfg.mode);
	jitem["otheraddr"] = cfg.otheraddr;
	jitem["serial"] = "/dev/" + cfg.serial.name;
	jitem["baudrate"] = cfg.serial.baudrate;
	jitem["databits"] = cfg.serial.databits;
	jitem["pairty"] = static_cast<int>(cfg.serial.pairty);
	jitem["stopbits"] = cfg.serial.stopbits;

	jitem["AppLayerParameters"]["MeasuredValueType"] = static_cast<int>(cfg.VType.MeasuredValueType);
	jitem["AppLayerParameters"]["TelegramValueType"] = static_cast<int>(cfg.VType.TelegramValueType);

	jitem["AppLayerParameters"]["sizeOfCOT"] = cfg.ALParam.sizeofCOT;
	jitem["AppLayerParameters"]["sizeOfCA"] = cfg.ALParam.sizeofCA;
	jitem["AppLayerParameters"]["sizeOfIOA"] = cfg.ALParam.sizeofIOA;

	jitem["LinkLayerParameters"]["LinkAddrLength"] = cfg.LLParam.LinkAddrLength;
	jitem["LinkLayerParameters"]["SingalCharACK"] = cfg.LLParam.SingalCharACK;
	jitem["LinkLayerParameters"]["TimeoutForACK"] = cfg.LLParam.TimeoutForACK;
	jitem["LinkLayerParameters"]["TimeoutForRepeat"] = cfg.LLParam.TimeoutForRepeat;

	jitem["ExtraParameters"]["isSequence"] = cfg.EXParam.isSequence;

	return true;
}

bool DSYSCFG::parse_cs104_to_Json(CS104Cfg &cfg, Json::Value &jitem, JsonParseFlag flag, bool use)
{
    switch(flag)
    {
        case JSON_FLAG_DEFALUT: {
            jitem["used"] = cfg.use;
        };break;
        case JSON_FLAG_NO_USE: {
            // 不做处理(无此项)
        };break;
        case JSON_FLAG_MANUAL: {
            jitem["used"] = use;
        };break;
    }

	jitem["localIP"] = cfg.eth.ip;
	jitem["port"] = cfg.eth.port;

	jitem["AppLayerParameters"]["MeasuredValueType"] = static_cast<int>(cfg.VType.MeasuredValueType);
	jitem["AppLayerParameters"]["TelegramValueType"] = static_cast<int>(cfg.VType.TelegramValueType);

	jitem["AppLayerParameters"]["sizeOfCOT"] = cfg.ALParam.sizeofCOT;
	jitem["AppLayerParameters"]["sizeOfCA"]	= cfg.ALParam.sizeofCA;
	jitem["AppLayerParameters"]["sizeOfIOA"] = cfg.ALParam.sizeofIOA;

	jitem["ExtraParameters"]["isSequence"] = cfg.EXParam.isSequence;

	return true;
}

bool DSYSCFG::isPublic()
{
    bool result = false;
    if (UCFG.type == MODE_PUB) {
        result = true;
    }
    return result;
}

bool DSYSCFG::useCS101Slave()
{
    return (IECCFG.Slave.CS101.use || LLCFG.use);
}

const DSYSCFG::UNITTYPE &DSYSCFG::GetUnitType() { return UCFG.type; }

const DSYSCFG::PUBLICCFG &DSYSCFG::GetPublicCFG() { return PUBCFG; }

DSYSCFG::OneBAYCFG &DSYSCFG::GetUnitCFG(CA ca, bool &result)
{
    auto ita = this->UCFG.info.find(ca);
    if(ita != UCFG.info.end()) {
        result = ita->second.use;
        return ita->second;
    }
    else {
        static OneBAYCFG bay;
        result = false;
        return bay;
    }
}

const DSYSCFG::UnitCFG &DSYSCFG::GetUnitCFG()
{
    return this->UCFG;
}

const DSYSCFG::RPCCfg &DSYSCFG::GetRPCCFG() { return RPCCFG; }

DSYSCFG::RPCCfg &DSYSCFG::ModifyRPCCFG() { return RPCCFG; }

const DSYSCFG::LCDCfg &DSYSCFG::GetLCDCFG() { return LCDCFG; }

DSYSCFG::LCDCfg &DSYSCFG::ModifyLCDCFG() { return LCDCFG; }

const DSYSCFG::SyncTimeCFG &DSYSCFG::GetSyncCFG() { return STCFG; }

DSYSCFG::SyncTimeCFG &DSYSCFG::ModifySyncCFG() { return STCFG; }

const DSYSCFG::LineLossCfg &DSYSCFG::GetLineCFG() { return LLCFG; }

DSYSCFG:: LineLossCfg &DSYSCFG::ModifyLineCFG() { return LLCFG; }

const DSYSCFG::GooseCFG &DSYSCFG::GetGooseCFG() { return GOOSECFG; }

DSYSCFG::GooseCFG &DSYSCFG::ModifyGooseCFG() { return GOOSECFG; }

DSYSCFG::UnitCFG &DSYSCFG::ModifyUnitCFG() { return this->UCFG; }

DSYSCFG::PUBLICCFG &DSYSCFG::ModifyPublicCFG() { return this->PUBCFG; }

////////////////////////////////////////////////////规约
bool DSYSCFG::cssave(std::string &file)
{
	DTU::buffer data = cspack();
	save_file(file, data);
	return true;
}

int DSYSCFG::ASDU() { return asduaddr; }

void DSYSCFG::ASDU(int ASDUno) { this->asduaddr = ASDUno; }

const DSYSCFG::IEC60870CFG &DSYSCFG::Get_IEC60870_CFG() { return IECCFG; }

DSYSCFG::IEC60870CFG &DSYSCFG::Modify_IEC60870_CFG() { return IECCFG; }

const DSYSCFG::StationCFG &DSYSCFG::Get_IEC60870_Master_CFG() { return IECCFG.Master; }

DSYSCFG::StationCFG &DSYSCFG::Modify_IEC60870_Master_CFG() { return IECCFG.Master; }

const DSYSCFG::StationCFG &DSYSCFG::Get_IEC60870_Slave_CFG() { return IECCFG.Slave; }

DSYSCFG::StationCFG &DSYSCFG::Modify_IEC60870_Slave_CFG() { return IECCFG.Slave; }

DTU::buffer DSYSCFG::cspack()
{
	
	Json::Value root;

	Json::Value &proot = root["protocol"];
	// 本机单元地址
	proot["asduaddr"] = asduaddr;

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 主站端配置 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//
	// CS101协议配置
	Json::Value &CS101_Master_Root = proot["master"]["CS101"];
    this->parse_cs101_to_Json(IECCFG.Master.CS101, CS101_Master_Root);
	
	// CS104协议配置
	Json::Value &CS104_Master_Root = proot["master"]["CS104"];
    this->parse_cs104_to_Json(IECCFG.Master.CS104, CS104_Master_Root);


	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 子站端配置 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//
	// CS101协议配置
	Json::Value &CS101_Slave_Root = proot["slave"]["CS101"];
    this->parse_cs101_to_Json(IECCFG.Slave.CS101, CS101_Slave_Root);
	
	// CS104协议配置
	Json::Value &CS104_Slave_Root = proot["slave"]["CS104"];
    this->parse_cs104_to_Json(IECCFG.Slave.CS104, CS104_Slave_Root);

	// 生成字节流
	Json::StreamWriterBuilder WriterBuilder;
	std::ostringstream os;
	std::unique_ptr<Json::StreamWriter> JsonWriter(WriterBuilder.newStreamWriter());
	JsonWriter->write(root, &os);
	std::string str = os.str();

	DTU::buffer result(str.c_str(),str.size());
	return std::move(result);
}

bool DSYSCFG::csunpack(DTU::buffer &data)
{
    DTU_USER();

	std::istringstream jsonsstream(std::string(data.data(), data.size()));

	Json::Value root;
	Json::CharReaderBuilder readerbuilder;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(readerbuilder, jsonsstream, &root, &errs)) {
		DTU_THROW((char *)"加载规约配置文件错误 %s", errs.c_str());
	}

	Json::Value proot = root["protocol"];
	// 本机单元地址
	asduaddr = proot["asduaddr"].asInt();

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 主站端配置 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//
	// CS101协议配置
	Json::Value CS101_Master_Root = proot["master"]["CS101"];
    this->parse_cs101_from_Json(IECCFG.Master.CS101, CS101_Master_Root);

	// CS104协议配置
	Json::Value CS104_Master_Root = proot["master"]["CS104"];
    this->parse_cs104_from_Json(IECCFG.Master.CS104, CS104_Master_Root);

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 子站端配置 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//
	// CS101协议配置
	Json::Value CS101_Slave_Root = proot["slave"]["CS101"];
    this->parse_cs101_from_Json(IECCFG.Slave.CS101, CS101_Slave_Root);

	// CS104协议配置
	Json::Value CS104_Slave_Root = proot["slave"]["CS104"];
    this->parse_cs104_from_Json(IECCFG.Slave.CS104, CS104_Slave_Root);
    
	return true;
}

bool DSYSCFG::csrecover(std::string file, bool issave)
{
	asduaddr = 1;
    CS101Cfg CS101INCfgNew;
    CS104Cfg CS104INCfgNew;
	IECCFG.Master.CS101 = CS101INCfgNew;
	IECCFG.Master.CS104 = CS104INCfgNew;
	IECCFG.Slave.CS101 = CS101INCfgNew;
    IECCFG.Slave.CS104 = CS104INCfgNew;
	if(issave)
		return this->cssave(file);
	else
		return true;
}

void DSYSCFG::fulshMAC()
{
    DTULOG(DTU_INFO, "刷新MAC配置");

    //修改sh脚本的内容
    std::string fullPath = get_exec_dir() + "/config/DTUMac.sh";

    FILE* fp = fopen(fullPath.c_str(),(char*)"w+");

    if(!fp)
    {
        DTULOG(DTU_ERROR, (char*)"无法打开网络配置脚本DTUMac.sh");
        return;
    }

    for (const auto &item : this->MacCFG)
    {
        fprintf(fp,(char*)"%sifconfig %s down\n", item.use ? "" : "# ", item.eth.c_str());
        fprintf(fp,(char*)"%sifconfig %s hw ether %s\n", item.use ? "" : "# ", item.eth.c_str(), item.mac.c_str());
        fprintf(fp,(char*)"%sifconfig %s up\n\n", item.use ? "" : "# ", item.eth.c_str());
    }

    fclose(fp); //关闭脚本文件

    std::string cmd = "chmod +x " + fullPath;
    system(cmd.c_str());
    cmd = fullPath;
    system(cmd.c_str());
}