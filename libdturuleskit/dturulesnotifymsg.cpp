/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturulesnotifymsg.cpp
  *Description:
    将规约的消息报文推送到工具
  *History:
    1, 创建, wangjs, 2022-4-2
**********************************************************************************/
#include "dturulesnotifymsg.h"
#include <dtulog.h>
#include <dtunotifymanager.h>
#include <dtustructs.h>
#include <dtucommon.h>

using namespace DTU;

void notifyToolCS(uint8_t type,bool sent,uint8_t proto,uint32_t length,uint8_t *msg,std::string log)
{
	DTU::dtuprotocol dtuproto;
	DTU::buffer buf;
	uint64_t msec = get_current_mills();
	uint8_t flag = type;

	buf.append((char*)(&msec),sizeof(uint64_t));   // 毫秒时间戳
	buf.append((char*)(&proto),sizeof(uint8_t));   // 数据规约来源
	buf.append((char*)(&flag),sizeof(uint8_t));    // 数据是报文还是日志
	buf.append((char*)(&sent),sizeof(bool));	   // 收/发
	buf.append((char*)(&length),sizeof(uint32_t)); // 数据长度

	switch(type)
	{
		case CSLOG: {
			buf.append(log.c_str(),log.size());            // 日志
			// RPC发布
			dtuproto._data = buf;
			DTUNotifyMgr::instance().notify_cstool(CSLOG,dtuproto);
		};break;
		case CSMSG: {
			buf.append((char*)(msg),length);               // 报文
			// RPC发布
			dtuproto._data = buf;
			DTUNotifyMgr::instance().notify_cstool(CSMSG,dtuproto);
		};break;
		default:
			DTULOG(DTU_ERROR,"notifyToolCS() 错误的类型%u",type);
	}
}

