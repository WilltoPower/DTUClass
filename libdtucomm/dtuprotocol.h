/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuprotocol.h
  *Description: 
    与MCU串口通信协议
  *History: 
    1, 创建, wangjs, 2021-8-3
**********************************************************************************/
#ifndef _PCTPROTOCOL_H
#define _PCTPROTOCOL_H
#include <memory>
#include <mutex>
#include <condition_variable>
#include <msgpack.hpp>
#include "dtucomm.h"

#define PCHEAD 0xaa55
#define ARMHEAD 0xbb66

namespace DTU
{
	class dtuprotocol
	{	
	public:
		dtuprotocol(){};
		dtuprotocol(uint16_t header, uint16_t cmd, uint32_t totalLen,
					uint32_t curLen, uint16_t blockno, uint16_t datano = 0xFFFF);
		dtuprotocol(const dtuprotocol &src);
		dtuprotocol &operator=(const dtuprotocol &src);

		/*
		@func 	: 从缓存中提起协议内容
		@buffer : 要解析的缓存数据
		*/
		void unpackage(const buffer &src);

		/*
		@func	: 得到缓存结构数据
		@data	: 结果数据
		*/
		buffer package() const;

		/*
		@func : 从IO中读取数据,包括网络和串口
		@pIO  : IO指针
		*/
		int readfromIO(dtubasecomm *pIO);
	public:
		uint16_t _header = 0;
		uint16_t _cmd = 0;
		uint32_t _totleLen = 0;
		uint16_t _curLen = 0;
		uint16_t _blockno = 0;
		uint32_t _crc32 = 0x00000000;
		uint16_t _isReboot = 1;
		// 数据部分
		buffer _data;
		// 序列化
		MSGPACK_DEFINE_ARRAY(_header, _cmd, _totleLen, _curLen, _blockno, _data, _crc32, _isReboot);
	protected:
		bool find_frame(buffer &src);
		bool check_header(buffer &src);

	private:
		// 总数据
		buffer _totalData;
		// 用于处理外部接收缓冲
		buffer _iodata;
		const uint32_t _headLen = 16;
	};
} // namespace PROTOCOL
#endif