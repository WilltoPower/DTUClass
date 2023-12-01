/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturulesnotifymsg.h
  *Description:
    将规约的消息报文推送到工具
  *History:
    1, 创建, wangjs, 2022-4-2
**********************************************************************************/
#ifndef _DTU_RULES_NOTIFY_MSG_H_
#define _DTU_RULES_NOTIFY_MSG_H_

#include <dtuprotocol.h>

void notifyToolCS(uint8_t type,bool sent,uint8_t proto,uint32_t length,uint8_t *msg,std::string log);

#endif /* _DTU_RULES_NOTIFY_MSG_H_ */