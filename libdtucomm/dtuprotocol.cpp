/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuprotocol.h
  *Description: 
    与MCU串口通信协议
  *History: 
    1, 创建, wangjs, 2021-8-3
	2, 修改了当前长度类型,
**********************************************************************************/
#include "dtuprotocol.h"
#include <dtulog.h>
#include <dtuerror.h>
#include <cassert>
using namespace DTU;
dtuprotocol::dtuprotocol(std::uint16_t header, std::uint16_t cmd, std::uint32_t totalLen,
						 std::uint32_t curLen, std::uint16_t blockno, std::uint16_t datano)
	: _header(header), _cmd(cmd), _totleLen(totalLen), _curLen(curLen),_blockno(blockno)//??, _datano(datano)
{
}
dtuprotocol::dtuprotocol(const dtuprotocol &src)
{
	_header = src._header;
	_cmd = src._cmd;
	_totleLen = src._totleLen;
	_curLen = src._curLen;
	_blockno = src._blockno;
	//??_datano = src._datano;
	_crc32 = src._crc32;
	_data.remove();
	_data = src._data;
}
dtuprotocol &dtuprotocol::operator=(const dtuprotocol &src)
{
	if (this != &src)
	{
		_header = src._header;
		_cmd = src._cmd;
		_totleLen = src._totleLen;
		_curLen = src._curLen;
		_blockno = src._blockno;
		//??_datano = src._datano;
		_crc32 = src._crc32;
		_data.remove();
		_data = src._data;
	}
	return *this;
}
void dtuprotocol::unpackage(const buffer &src)
{
	DTU_USER()
	if (src.size() < _headLen){
		DTU_THROW((char*)"dtuprotocol parse invlaid length %u", src.size());
	}
	uint32_t offset = 0;
	_header = src.get(offset, sizeof(_header)).value<uint16_t>();
	offset += sizeof(_header);
	_cmd = src.get(offset, sizeof(_cmd)).value<uint16_t>();
	offset += sizeof(_cmd);
	_totleLen = src.get(offset, sizeof(_totleLen)).value<uint32_t>();
	offset += sizeof(_totleLen);
	_curLen = src.get(offset, sizeof(_curLen)).value<uint16_t>();
	offset += sizeof(_curLen);
	_blockno = src.get(offset, sizeof(_blockno)).value<uint16_t>();
	offset += sizeof(_blockno);
	_crc32 = src.get(offset, sizeof(_crc32)).value<uint32_t>();
	offset += sizeof(_crc32);
	//
	if (_curLen > 0)
	{
		_data.remove();
		_data.append(src.get(offset, _curLen));
	}
	
}

buffer dtuprotocol::package() const
{
	buffer data;

	data.append((char*)&_header, sizeof(_header));
	data.append((char*)&_cmd, sizeof(_cmd));
	data.append((char*)&_totleLen, sizeof(_totleLen));
	data.append((char*)&_curLen, sizeof(_curLen));
	data.append((char*)&_blockno, sizeof(_blockno));
	data.append((char*)&_crc32, sizeof(_crc32));
	data.append(_data);
	
	return std::move(data);
}

bool dtuprotocol::find_frame(buffer &src)
{
	if (src.size() == 0){
		return false;
	}
	if (!check_header(src))
	{
		return false;
	}
	uint32_t offset = 0;
	_header = src.get(offset, sizeof(_header)).value<uint16_t>();
	offset += sizeof(_header);
	_cmd = src.get(offset, sizeof(_cmd)).value<uint16_t>();
	offset += sizeof(_cmd);
	_totleLen = src.get(offset, sizeof(_totleLen)).value<uint32_t>();
	offset += sizeof(_totleLen);
	_curLen = src.get(offset, sizeof(_curLen)).value<uint16_t>();
	offset += sizeof(_curLen);
	
	if (src.size() < _curLen + _headLen)
	{
		DTULOG(DTU_ERROR,(char*)"dtuprotocol数据长度:%u 小于当前长度值:%u", src.size(), _curLen+_headLen);
		return false;
	}
	// 设置头
    unpackage(src);
	return true;
}

bool dtuprotocol::check_header(buffer &src)
{
	if (src.size() < _headLen)
	{
		return false;
	}
	// 查找头部标识
	bool bFindHead = false;
	uint32_t index = 0;
	for (; index < src.size() / 2; index += 2)
	{
		uint16_t headFlag = 0;
		headFlag = src.get(index, 2).value<uint16_t>();
		if (headFlag == 0xaa55 || headFlag == 0xbb66)
		{
			bFindHead = true;
			break;
		}
	}
	src.remove(0, index);
	return bFindHead;
}

int dtuprotocol::readfromIO(dtubasecomm *pIO)
{
	/*
	有三种情况需要处理：
	1,网络断开
	2,超时
	3,正常接收
	*/
	//sdlbuff recvdata;
	bool isFrame = false;
	int nFalseCount = 0; // 一个计数器，如果10次仍然没有正确数据接收，则放弃
	int bRet = 0;
	while (nFalseCount < 10)
	{
		bRet = pIO->ReadData(_iodata);
		if (bRet < 0)
		{
			if (_iodata.size() == 0)
			{
				// 网络已经断开, 且已经没有残留数据
				return DTU_TCP_DISCONNECT;
			}
			if (find_frame(_iodata))
			{
				bRet = _data.size();
				_iodata.remove(0, _headLen + _curLen);
				break;
			}
		}
		if (find_frame(_iodata))
		{
			bRet = _data.size();
			_iodata.remove(0, _headLen + _curLen);
			break;
		}
		nFalseCount++;
	}
	if (nFalseCount == 10)
	{
		// 超时
		return DTU_TCP_TIMEOUT;
	}
	return DTU_SUCCESS;
}