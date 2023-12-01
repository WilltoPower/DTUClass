/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuprotocol.h
  *Description: 
    私有通信接口
  *History: 
    1, 创建, wangjs, 2021-8-3
**********************************************************************************/
#ifndef _BASE_COM_H
#define _BASE_COM_H
#include <dtubuffer.h>
namespace DTU
{
	class dtubasecomm
	{
	public:
		virtual ~dtubasecomm(){}
		virtual void Close() {};
		virtual int ReadData(buffer& data, unsigned int dwTimeout = 1000000) {
			return 0;
		};
		virtual int SendData(const buffer& data) { return 0; };
		virtual int GetFd() { return -1; };
	};
}

#endif