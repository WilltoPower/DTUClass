/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  DTUDef.h
  *Description: 
    定义一些常用的宏，常量等
  *History: 
    1, 创建, wangjs, 2021-7-6
    2, 加入部分错误码, wangjs, 2021-7-30
**********************************************************************************/
#ifndef _DTU_DEF_H
#define _DTU_DEF_H

#include <stdexcept>

#define DTU_USER() \
    char outputbuf[128] = {};
#ifdef _WIN32
#define DTU_THROW(...) \
    sprintf_s(outputbuf, __VA_ARGS__); \
    throw std::runtime_error(std::string(outputbuf));
#else
#define DTU_THROW(...) \
    sprintf(outputbuf, __VA_ARGS__); \
    throw std::runtime_error(std::string(outputbuf));
#endif

#define THROW_RUNTIME_ERROR_IF(expr, what) \
  if ((expr)) throw std::runtime_error(what)

#define INF_FILE_PATH ("infcfg.json")

#define SAFE_ARRAY_DEL(ptr) if(ptr){delete[] ptr;ptr=nullptr;}

#define DTU_SUCCESS		     (0x00)
#define DTU_INVALID_CMD    (0x01)
#define DTU_DSP_ERROR      (0x02)
#define DTU_DSP_DATA_ERROR (0x03)
#define DTU_FPGA_ERROR     (0x04)
#define DTU_ARM_ERROR      (0x05)
#define DTU_CONFIG_ERROR   (0x06)
#define DTU_REBOOT_ERROR   (0x07)
#define DTU_GOOSE_ERROR    (0x08)

#define DTU_TCP_ERROR      (0x10)
#define DTU_TCP_DATA_ERROR (0x11)
#define DTU_TCP_DISCONNECT (0x12)
#define DTU_TCP_TIMEOUT	   (0x13)

#define DTU_SERIAL_ERROR   (0x20)

#define DTU_FILE_ERROR     (0x30)

#define DTU_RPC_ERROR	     (0x40)

#define DTU_UNKNOWN_ERROR  (0xFF)

#define DTU_FAILED(a)	(a != DTU_SUCCESS)
#endif