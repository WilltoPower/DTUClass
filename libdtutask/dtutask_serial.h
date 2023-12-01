/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_serial.h
  *Description: 
    实现任务与串口交互的执行流程
  *History: 
    1, 创建, wangjs, 2021-8-3
**********************************************************************************/
#ifndef _DTU_TASK_SERIAL_H
#define _DTU_TASK_SERIAL_H
#include <dtuprotocol.h>
#include <thread>
#include <tuple>

int execute_serial_task(DTU::dtuprotocol& proto, DTU::dtubasecomm* pIO);

#endif