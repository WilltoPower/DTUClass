/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuserial.h
  *Description: 
    与MCU的串口通信服务
  *History: 
    1, 创建, wangjs, 2021-8-3
	2, 加入主动通知MCU的功能, wangjs, 2021-8-17
**********************************************************************************/
#ifndef _SDL_SERIAL_H
#define _SDL_SERIAL_H
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <dtuprotocol.h>
#include "dtuserial_unit.h"
namespace DTU
{
	class dtuserial
	{
	private:
		dtuserial();
	public:
		static dtuserial& instance() {
			static dtuserial serial;
			return serial;
		}
		bool start_serial(const std::string& comport, const std::uint32_t baudrate);
		// 发送数据
		int notify_serial(const dtuprotocol& data);

		void stop_serial();
	public:
		SerialUnit _com;
	protected:
		//处理响应式任务数据
		void run_reactor_task();
	private:
		std::unique_ptr<std::thread> _pRun;
		std::atomic<bool> _bStop;
		std::string _comport;
		std::uint32_t _baudrate;		
	};
}
#endif