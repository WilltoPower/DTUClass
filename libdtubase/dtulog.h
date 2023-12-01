/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description: 
    定义基于SPDLOG的日志功能接口
  *History: 
    1, 创建, wangjs, 2021-7-2
**********************************************************************************/
#include <dtuerror.h>

#define DTU_WARN  0
#define DTU_INFO  1
#define DTU_ERROR 2
#define DTU_DEBUG 3

#define LOG_PREFIX ("DTU")

void DTULOG(int level, const char* format, ...);