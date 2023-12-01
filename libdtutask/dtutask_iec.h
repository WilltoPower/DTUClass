/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_iec.h
  *Description:
    实现任务与规约的交互
  *History:
    1, 创建, wangjs, 2021-8-4
    2，加入目录获取接口, wangjs, 2021-8-23
**********************************************************************************/
#ifndef _DTU_TASK_IEC_H
#define _DTU_TASK_IEC_H
#include <dtubuffer.h>
#include "dtucommon.h"
// 切换定值区
int iectask_execute_change(uint32_t current, uint32_t dest);
// 查询定值
int iectask_execute_query(uint16_t cmd, DTU::buffer& result);
// 读定值
int iectask_execute_read(uint16_t cmd, uint32_t group, DTU::buffer& result);
// 写定值
int iectask_execute_write(uint16_t cmd,uint32_t group, const DTU::buffer& value, bool isReboot = true);
// 确认定值预设
int iectask_execute_confirm();

int iectask_execute_control(uint16_t fix,uint16_t operate,RemoteCtrlInfo info);

int iectask_execute_time(DTU::buffer& result);

int iectask_read_time(DTU::buffer& result);

#endif
