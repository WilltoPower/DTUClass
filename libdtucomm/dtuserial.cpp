/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuserial.cpp
  *Description: 
    与MCU的串口通信服务
  *History: 
    1, 创建, wangjs, 2021-8-3
	2, 添加串口任务处理, wangjs, 2021-8-4
**********************************************************************************/
#include "dtuserial.h"
#include <chrono>
#include <dtulog.h>
#include <dtuerror.h>
#include <dtutask_serial.h>
#include "dtuprotocol.h"
#include "dtucmdcode.h"

using namespace DTU;

dtuserial::dtuserial() :_bStop(false)
{
}

bool dtuserial::start_serial(const std::string& comport, const std::uint32_t baudrate)
{
	_comport = comport;
	_baudrate = baudrate;
	DTULOG(DTU_INFO, (char*)"打开串口:%u, %s", _baudrate,_comport.c_str());
	if (!_com.OpenSerial(comport))
	{
		DTULOG(DTU_ERROR, (char*)"打开串口:%u, %s失败!!!", _baudrate, _comport.c_str());
		return false;
	}
#ifndef _WIN32
	_com.serial_SetPara(baudrate, 8, 0, 0);
#endif
	// 注册给下层调用
	// TASK::etask_notify::GetInstance()->registerSerial(&_com);
	// run_serial();
	_pRun = std::make_unique<std::thread>(&dtuserial::run_reactor_task, this);
	return true;
}
void dtuserial::stop_serial()
{
	_bStop = true;
	if (_pRun){
		_pRun->join();
	}
}

void dtuserial::run_reactor_task()
{
	bool bsuccess = true;

	while (!_bStop && bsuccess)
	{
		dtuprotocol recvResult;
		if (DTU_SUCCESS != recvResult.readfromIO(&_com))
		{
			continue;
		}
		//执行串口任务
		auto ret = execute_serial_task(recvResult, &_com);
		if (DTU_SUCCESS != ret)
		{
			DTULOG(DTU_ERROR,(char*)"任务0x%04X执行失败, ERROR:0x%04X", recvResult._cmd, ret);
			continue;
		}
		//防止主界面命令快速填满LOG
		if(recvResult._cmd != TX_PC_YC_DATA)
		{
			DTULOG(DTU_INFO,(char*)"任务0x%04X执行成功", recvResult._cmd);
		}
	}
}
int dtuserial::notify_serial(const dtuprotocol& data)
{
	return _com.SendData(data.package());
}