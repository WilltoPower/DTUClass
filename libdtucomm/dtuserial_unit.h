/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuserial.h
  *Description: 
    串口通信实现
  *History: 
    1, 创建, wangjs, 2021-8-3
**********************************************************************************/
#ifndef _FESERIAL_H
#define _FESERIAL_H
#ifdef _WIN32
#include <windows.h>
#else
#include <cinttypes>
#include <termios.h>
#endif
#include "dtucomm.h"
#include <string>
#include <mutex>
#include <dtubuffer.h>
namespace DTU
{
	class SerialUnit : public dtubasecomm
	{
		//DECLARE_MEMORY_POOL(FESerial)
	#ifdef _WIN32
	private:
		SerialUnit() {};
		SerialUnit(HANDLE hCom) :m_hCom(hCom) {};
	public:
		static SerialUnit* GetInstance() {
			static SerialUnit instance;
			return &instance;
	}
	#else
	public:
		SerialUnit() {};
		SerialUnit(int fd) : _fd(fd) {};
	#endif
	public:
		SerialUnit(const std::string& comport, const int baudrate);
		virtual void Close();
		virtual int ReadData(DTU::buffer& data, unsigned int dwTimeout = 10000000);
		virtual int SendData(const DTU::buffer& data);

	#ifdef _WIN32
		virtual int GetFd() { return (int)m_hCom; }
	#else
		int serial_SetPara(int speed, int databits, int stopbits, int parity);
		virtual int GetFd() { return _fd; };
	#endif
		bool OpenSerial(const std::string& comport);
	private:
	#ifdef _WIN32
		DWORD m_dwBaudrate;
		HANDLE m_hCom = INVALID_HANDLE_VALUE;//
	#else
		int _fd = -1;
		struct termios original_port_settings_;
		std::string _strSerial;
		int _dwBaudrate = 0;
	#endif
	};
};
#endif