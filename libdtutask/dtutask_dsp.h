/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_dsp.h
  *Description: 
    实现任务与DSP交互的执行流程
  *History: 
    1, 创建, wangjs, 2021-8-3
    2, 修改DSP层的实现，wangjs, 2022-1-10
**********************************************************************************/
#ifndef _DTU_TASK_DSP_H
#define _DTU_TASK_DSP_H
#include <dtuprotocol.h>
#include <thread>
#include <stdlib.h>
#include <dtulog.h>
#include <dtucommon.h>

// 写参数
int dsptask_execute_write(uint16_t cmd, const DTU::buffer& data, uint16_t reboot=1);
// 读参数
int dsptask_execute_read(uint16_t cmd, DTU::buffer& result);
// 控制
int dsptask_execute_control(uint16_t cmd);
// 信息查看，显示遥信，遥测，状态等信息
int dsptask_execute_query(uint16_t cmd, DTU::buffer& result);
// 遥控 这里的fix是内部的映射完之后的点表
int dsptask_execute_rmctrl(uint16_t fix,uint16_t operate,RemoteCtrlInfo info);

void dsptask_execute_cancelrmc();

class AutoTime{
    public:
        static AutoTime &instance() {
        static AutoTime instance;
        return instance;
    }
    public:
    void start(uint32_t interval = 15) {
		_interval = interval;
		_run = true;
		_thread = std::make_unique<std::thread>(&AutoTime::SetTime, this);
    }
    void stop() {
		_run = false;
		if(_thread)
		{
			_thread->join();
		}
	}
	bool CalibrateSystemTimeOnce()
	{
		std::string cmd;
		DTU::buffer _data;
		DTULOG(DTU_INFO,"校准系统时间");
		_data.remove();
		if (DTU_SUCCESS == dsptask_execute_query(0xAAF0, _data))
		{
			uint64_t timer;
			struct tm timeall;

			timeall.tm_year = _data.get(0,sizeof(uint16_t)).value<uint16_t>() - 1900;
			timeall.tm_mon = _data.get(sizeof(uint16_t) * 1, sizeof(uint8_t)).value<uint8_t>() - 1;
			timeall.tm_mday = _data.get(sizeof(uint8_t) * 3, sizeof(uint8_t)).value<uint8_t>();
			timeall.tm_hour = _data.get(sizeof(uint8_t) * 4, sizeof(uint8_t)).value<uint8_t>();
			timeall.tm_min = _data.get(sizeof(uint8_t) * 5, sizeof(uint8_t)).value<uint8_t>();
			timeall.tm_sec = _data.get(sizeof(uint8_t) * 6, sizeof(uint8_t)).value<uint8_t>();
			timeall.tm_isdst = -1;
			uint8_t ms_l = _data.get(sizeof(uint8_t) * 7, sizeof(uint8_t)).value<uint8_t>();
			uint8_t ms_h = _data.get(sizeof(uint8_t) * 8, sizeof(uint8_t)).value<uint8_t>();
			uint16_t ms = (((ms_h & 0x00FF) << 8) & 0xFF00) | (ms_l & 0x00FF);

			timer = mktime(&timeall);

			struct timespec tp;
			tp.tv_sec = timer;
			tp.tv_nsec = ms * 1000000;

			if(clock_settime(CLOCK_REALTIME, &tp) < 0) {
				//perror( "clock_settime");
				DTULOG(DTU_ERROR,"校准系统时间下发失败");
			}

			// cmd = "date -s \"" + std::to_string(year) + "-" + std::to_string(mon) + "-" + std::to_string(day) + " " + std::to_string(hh) + ":" + std::to_string(mm) + ":" + std::to_string(ss) + "\"";
			// system(cmd.c_str());
			return true;
		}
		else
		{
			DTULOG(DTU_ERROR,"校准系统时间失败");
			return false;
		}
	}
	private:
    void SetTime() {
		while(_run) {
			CalibrateSystemTimeOnce();
			// 30分钟校准一次系统时间
			std::this_thread::sleep_for(std::chrono::minutes(_interval));
		}
	}
    private:
    std::atomic<bool> _run;
	std::unique_ptr<std::thread> _thread;
	uint32_t _interval = 15; // 默认15min校时一次
};

// 测试参数
// int dsptask_execute_test(DTU::dtuprotocol& proto);
// // 编辑区固化
// int dsptask_execute_setconfirm();
// // 定值区切换
// int dsptask_execute_setchange(uint16_t dstgroup);
#endif