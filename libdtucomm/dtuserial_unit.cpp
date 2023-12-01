/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuserial.h
  *Description: 
    串口通信实现
  *History: 
    1, 创建, wangjs, 2021-8-3
**********************************************************************************/
#include "dtuserial_unit.h"
#include <iostream>
#ifdef _WIN32
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include <dtulog.h>
#include <thread>
#include <chrono>

using namespace DTU;
SerialUnit::SerialUnit(const std::string& comport, const int baudrate)
{
	
}

void SerialUnit::Close()
{
#ifdef _WIN32
	if (m_hCom != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCom);
	m_hCom = INVALID_HANDLE_VALUE;
#else
	close(_fd);
	//tcsetattr(_fd, TCSANOW, &original_port_settings_);
	_fd = -1;
#endif
}

int SerialUnit::ReadData(buffer& data, unsigned int dwTimeout /*= 10000000*/)
{
#ifdef _WIN32
	if (m_hCom == INVALID_HANDLE_VALUE) {
		return -1;
	}
#else
	if (_fd == -1)
		return -1;
#endif
	const int length = 65535;
	char buffer[length] = {};
#ifdef _WIN32
	DWORD dwBytesRead;
	DWORD dwErrors;
	COMSTAT Stat;
	ReadFile(m_hCom, (char*)buffer, length, &dwBytesRead, NULL);	
	auto len = dwBytesRead;
#else

	//if (fd < 0) { return -1; }
	int len = 0;
	memset(buffer, 0, length);

	int max_fd = 0;
	fd_set readset = { 0 };
	struct timeval tv = { 0 };

	FD_ZERO(&readset);
	FD_SET((unsigned int)_fd, &readset);
	max_fd = _fd + 1;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	if (select(max_fd, &readset, NULL, NULL, &tv) < 0)
	{
		//printf("ReadData: select error\n");
	}
	int nRet = FD_ISSET(_fd, &readset);
	if (nRet)
	{
		len = read(_fd, buffer, length);
	}
	//return len;

#endif
	
	if (len < 0) {
		return -1;
	}
	else if (len == 0) {

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	else
	{
		data.append(buffer, len);
	}

	return  data.size();// Returns the number of bytes read
}

int SerialUnit::SendData(const buffer& data)
{
#ifdef _WIN32
	DWORD dwBytesWritten = 0;
	DWORD dwErrors;
	COMSTAT Stat;

	ClearCommError(m_hCom, &dwErrors, &Stat);
	if (!WriteFile(m_hCom, (char*)data.data(), data.size(), &dwBytesWritten, NULL))
	{
		return -1;
	}
	return dwBytesWritten;
#else
	//auto logger = spdlog::get("sdl9601b");
	if (_fd == -1)
		return -1;

	auto chdata = data.const_data();
	auto datalength = data.size();
	int len = 0, total_len = 0;//modify8.
	
	int nRetry = 0;
	for (total_len = 0; total_len < data.size();)
	{
		len = write(_fd, &chdata[total_len], datalength - total_len);
		if (len > 0)
		{
			total_len += len;
		}
		else if (len <= 0)
		{
			if (errno == EINTR) /* 中断错误 继续写*/
			{
				DTULOG(DTU_WARN, (char*)"serial wirte errno == EINTR continue");
				continue;
			}
			else if (errno == EAGAIN) /* EAGAIN : Resource temporarily unavailable*/
			{
				DTULOG(DTU_WARN, (char*)"serial wirte errno == EAGAIN continue");
				if (nRetry > 10)
				{
					break;
				}
				nRetry++;
				std::this_thread::sleep_for(std::chrono::seconds(1));//等待一秒，希望发送缓冲区能得到释放  
				continue;
			}
			else /* 其他错误 没有办法,只好退了*/
			{
				DTULOG(DTU_ERROR, (char*)"serial wirte errno==%d, %s continue",errno, strerror(errno));
				return(-1);
			}
		}
	}
	return len;
#endif
}
#ifndef _WIN32
int serial_BaudRate(int baudrate)
{
	switch (baudrate)
	{
	case 2400:
		return (B2400);
	case 4800:
		return (B4800);
	case 9600:
		return (B9600);
	case 19200:
		return (B19200);
	case 38400:
		return (B38400);
	case 57600:
		return (B57600);
	case 115200:
		return (B115200);
	default:
		return (B9600);
	}
}
int SerialUnit::serial_SetPara(int speed, int databits, int stopbits, int parity)
{
	struct termios termios_new;
	bzero(&termios_new, sizeof(termios_new));//等价于memset(&termios_new,sizeof(termios_new));
	cfmakeraw(&termios_new);//就是将终端设置为原始模式
	termios_new.c_cflag = serial_BaudRate(speed);
	termios_new.c_cflag |= CLOCAL | CREAD;
	//  termios_new.c_iflag = IGNPAR | IGNBRK;
	_dwBaudrate = speed;
	termios_new.c_cflag &= ~CSIZE;
	switch (databits)
	{
	case 0:
		termios_new.c_cflag |= CS5;
		break;
	case 1:
		termios_new.c_cflag |= CS6;
		break;
	case 2:
		termios_new.c_cflag |= CS7;
		break;
	case 3:
		termios_new.c_cflag |= CS8;
		break;
	default:
		termios_new.c_cflag |= CS8;
		break;
	}

	switch (parity)
	{
	case 0:  				//as no parity
		termios_new.c_cflag &= ~PARENB;    //Clear parity enable
	  //  termios_new.c_iflag &= ~INPCK; /* Enable parity checking */  //add by fu
		break;
	case 1:
		termios_new.c_cflag |= PARENB;     // Enable parity
		termios_new.c_cflag &= ~PARODD;
		break;
	case 2:
		termios_new.c_cflag |= PARENB;
		termios_new.c_cflag |= ~PARODD;
		break;
	default:
		termios_new.c_cflag &= ~PARENB;   // Clear parity enable
		break;
}
	switch (stopbits)// set Stop Bit
	{
	case 1:
		termios_new.c_cflag &= ~CSTOPB;
		break;
	case 2:
		termios_new.c_cflag |= CSTOPB;
		break;
	default:
		termios_new.c_cflag &= ~CSTOPB;
		break;
	}
	tcflush(_fd, TCIFLUSH); // 清除输入缓存
	tcflush(_fd, TCOFLUSH); // 清除输出缓存
	termios_new.c_cc[VTIME] = 1;   // MIN与 TIME组合有以下四种：1.MIN = 0 , TIME =0  有READ立即回传 否则传回 0 ,不读取任何字元
	termios_new.c_cc[VMIN] = 1;  //    2、 MIN = 0 , TIME >0  READ 传回读到的字元,或在十分之一秒后传回TIME 若来不及读到任何字元,则传回0
	tcflush(_fd, TCIFLUSH);  //    3、 MIN > 0 , TIME =0  READ 会等待,直到MIN字元可读
	return tcsetattr(_fd, TCSANOW, &termios_new);  //    4、 MIN > 0 , TIME > 0 每一格字元之间计时器即会被启动 READ 会在读到MIN字元,传回值或
}
#endif
bool SerialUnit::OpenSerial(const std::string& serialport)
{
#ifdef _WIN32
	if (m_hCom != INVALID_HANDLE_VALUE)
		return true;

    m_hCom = CreateFile(serialport.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;

	DCB		dcb;		//串口设置
	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hCom, &dcb);
	dcb.BaudRate = m_dwBaudrate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	SetCommState(m_hCom, &dcb);
	
	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 50;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 5;
	CommTimeOuts.ReadTotalTimeoutConstant = 100;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(m_hCom, &CommTimeOuts);

	PurgeComm(m_hCom, PURGE_TXCLEAR);
	PurgeComm(m_hCom, PURGE_RXCLEAR);

	return TRUE;
#else
	_fd = open(serialport.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);//O_RDWR | O_NOCTTY | O_NDELAY   //O_NONBLOCK
	if (_fd < 0) {
		return -1;
	}
	_strSerial = serialport;

	struct termios termios_old;
#endif
	return true;
}
