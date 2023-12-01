/*********************************************************************************
	*Copyright(C),2021-2025,sddl
	*FileName:  dtu101slave.cpp
	*Description: 
		用于实现dtu间隔单元的101服务
	*History: 
		1, 创建, lhy, 2022-01-06
**********************************************************************************/
#include "dtu101master.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <dtulog.h>
#include <dtucommon.h>

#include "dturulesasdu.h"

using namespace DTU;

D101Master::~D101Master()
{
	dtu101_stop_master();
}

// 回调处理函数
bool D101Master::dtu_asduHandler(int address, CS101_ASDU asdu)
{
	bool result = false;
	IEC60870_5_TypeID type = CS101_ASDU_getTypeID(asdu);
	DTULOG(DTU_INFO, (char *)"CS101主站端接收到101请求:%u", type);

    // 从线损模块接受电能量信息 Ti进行添加
	switch (type)
	{
		case M_IT_NB_1: {
			// 实时电能量信息 待处理
			result = true;
		};break;
		case M_IT_TC_1: {
			// 形成电能量报文
			if(CS101_ASDU_getCOT(asdu) == CS101_COT_SPONTANEOUS) {
				// 潮流变化电能量
				elecEnergyFormTideFile(asdu);
			}
			else {
				elecEnergyFormFile(asdu);
			}
			result = true;
		}; break;
	}
	CS101_Master_getAppLayerParameters(_master);
	return result;
}

void D101Master::dtu_linkLayerStateChanged(int address, LinkLayerState state)
{
	std::string ret = "101Master[" + std::to_string(address) + "]链路状态: ";
	int retno;
	switch (state) {
	case LL_STATE_IDLE:
			ret += "IDLE";
			retno = DTU_INFO;
			break;
	case LL_STATE_ERROR:
			ret += "ERROR";
			retno = DTU_ERROR;
			break;
	case LL_STATE_BUSY:
			ret += "BUSY";
			retno = DTU_WARN;
			break;
	case LL_STATE_AVAILABLE:
			ret += "AVAILABLE";
			retno = DTU_INFO;
			break;
	}
	DTULOG(retno,"%s",ret.c_str());
}

void D101Master::dtu_RawMessageHandler(uint8_t* msg, int msgSize, bool sent)
{
    // if (sent)
    //     printf("SEND: ");
    // else
    //     printf("RCVD: ");

    // int i;
    // for (i = 0; i < msgSize; i++) 
    // {
    //     printf("%02x ", msg[i]);
    // }
    // printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void
linkLayerStateChanged(void* parameter, int address, LinkLayerState state)
{
	if (parameter)
		((D101Master*)parameter)->dtu_linkLayerStateChanged(address, state);
}

static bool
asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu)
{
	if(parameter)
		return ((D101Master*)parameter)->dtu_asduHandler(address, asdu);
	return true;
}

static
void RawMessageHandler(void* parameter, uint8_t* msg, int msgSize, bool sent)
{
	if(parameter)
		return ((D101Master*)parameter)->dtu_RawMessageHandler(msg, msgSize, sent);
}

static void install_hanlder(CS101_Master master, void* param)
{
	// 设置通用ASDU的回调处理程序
	CS101_Master_setASDUReceivedHandler(master, asduReceivedHandler, param);
	// 设置链接层状态更改的回调处理程序
	CS101_Master_setLinkLayerStateChanged(master, linkLayerStateChanged, param);
	// 设置消息回调
	CS101_Master_setRawMessageHandler(master, RawMessageHandler, param);
}

void D101Master::dtu101_init_master(std::string serialPort, uint32_t baudrate, int address)
{
	//TODO:从配置获取链路地址
	slaveAddress.push_back(address);

	_port = SerialPort_create(serialPort.c_str(), baudrate, 8, 'N', 1);

	// 非平衡模式传输
	_master = CS101_Master_create(_port, NULL, NULL, IEC60870_LINK_LAYER_UNBALANCED);

	// 配置应用层配置
	CS101_AppLayerParameters AlParam = CS101_Master_getAppLayerParameters(_master);
	AlParam->originatorAddress = 1;//TODO:确认是否需要这个参数(默认是1)
	AlParam->sizeOfCA = 2;
	AlParam->sizeOfCOT = 2;
	AlParam->sizeOfIOA = 3;

	// 配置链路层配置
	LinkLayerParameters llParam = CS101_Master_getLinkLayerParameters(_master);
	llParam->addressLength = 2;
	llParam->timeoutForAck = 2000;// TODO(如果有线损模块应大于400)
	llParam->timeoutRepeat = 2000;
	llParam->useSingleCharACK = true;


	// 安装回调处理函数
	install_hanlder(_master, this);

	// 添加从机链路地址
	for(auto &oneAddr : slaveAddress) {
		CS101_Master_addSlave(_master, oneAddr);
	}
	
	SerialPort_open(_port);
}

void D101Master::dtu101_run_master()
{
	_runthread = std::make_unique<std::thread>([&]() {
		while(!_bStop) {
			// 多链路轮询
			for(auto &oneAddr : slaveAddress) {
				CS101_Master_pollSingleSlave(_master, oneAddr);
				CS101_Master_run(_master);
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	});
}

void D101Master::dtu101_stop_master()
{
	if (!_bStop && _runthread) {
		_bStop = true;
		_runthread->join();
		CS101_Master_destroy(_master);
		SerialPort_close(_port);
		SerialPort_destroy(_port);
	}
}

CS101_ASDU 
D101Master::dtu_create_asdu(bool isSequence, CS101_CauseOfTransmission cot, int oa, int ca, bool isTest, bool isNegative)
{
	CS101_ASDU result = nullptr;
	if(_master != nullptr) {
		CS101_AppLayerParameters AlParam = CS101_Master_getAppLayerParameters(_master);
		result = CS101_ASDU_create(AlParam, isSequence, cot, oa, ca, isTest, isNegative);
	}
	return result;
}

bool D101Master::send_asdu(int address, CS101_ASDU asdu)
{
	bool result = false;
	if(asdu != nullptr) {
		if (CS101_Master_isChannelReady(_master, address)) {
			CS101_Master_useSlaveAddress(_master, address);
			CS101_Master_sendASDU(_master, asdu);
			result = true;
		}
		else {
			DTULOG(DTU_ERROR,"101Master链路地址[%d]未准备就绪", address);
		}
	}
	return result;
}

void D101Master::send_elec_interrogation_cmd(CA addr)
{
	if (CS101_Master_isChannelReady(_master, addr)) {
    	CS101_Master_useSlaveAddress(_master, addr);
		CS101_Master_sendCounterInterrogationCommand(_master, CS101_COT_ACTIVATION, addr, IEC60870_QCC_RQT_GENERAL + IEC60870_QCC_FRZ_READ);
		CS101_Master_run(_master);
	}
}

const CS101_AppLayerParameters D101Master::GetAlParamter()
{
	return CS101_Master_getAppLayerParameters(_master);
}

bool D101Master::dtu_clockSyncHandler(int ca)
{
	auto AlParam = GetAlParamter();
	return ClockSyncStation<bool(int,CS101_ASDU)>(AlParam, AlParam->originatorAddress, ca, 
				std::bind(&DTU::D101Master::send_asdu, this, std::placeholders::_1, std::placeholders::_2));
}