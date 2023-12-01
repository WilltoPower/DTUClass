/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtucommon.h
  *Description: 
    通用功能定义
  *History: 
    1, 创建, wangjs, 2021-8-10
    2, 加入获取目录下文件列表的功能, wangjs, 2021-8-23
    3, 加入UTF8转GBK功能, wangjs, 2021-8-27
**********************************************************************************/
#ifndef _DTU_COMMON_H
#define _DTU_COMMON_H
#include <string>
#include <vector>
#include <bitset>
#include <complex>
#include <string>
#include <dtucmdcode.h>
#include "dtustructs.h"
#include "dtubuffer.h"

uint32_t GBKToUTF8(char* chGBK, size_t lenGBK, char* chUTF8, size_t lenUTF8);
uint32_t UTF8ToGBK(char* chUTF8, size_t lenUTF8, char* chGBK, size_t lenGBK);


// 获取程序运行目录
std::string get_exec_dir();
// 在运行目录下创建目录
bool create_dir_in_exec(std::string dir);
// 当前微妙数
uint64_t get_current_mirco(bool bUTC = false);
// 当前秒数
uint64_t get_current_seconds(bool bUTC = false);
// 当前的毫秒
uint64_t get_current_mills();
// 根据微妙时间戳，生成一个文件名
std::string create_filename_from_mirco(uint64_t llMicrosecond);
// 
std::string create_time_from_mirco(uint64_t llMicosecond);
//
std::string create_comtradetime_from_mirco(uint64_t llMicosecond);
//
std::string create_time_from_format(uint64_t llMicoseond, std::string format);
// 获取目录下文件
void get_dir_files(const std::string& dir, FILELIST& files);
// 获取文件的大小
uint64_t get_file_size(const std::string& fileName);
// 获取文件
void get_file(const std::string& fileName, DTU::buffer& data);
// 获取文件
void get_file(const std::string& fileName, DTU::buffer& data, uint64_t offset, uint64_t size);
// 获取文件信息
FILEINFO get_file_info(const std::string &fileName);
// 保存文件
int save_file(const std::string& fileName, const DTU::buffer& data);
// 保存大文件
int save_file(const std::string& fileName, const DTU::buffer& data, bool transOK);
// 目录管理
FILELIST get_file_list(const std::string& dir);
// 主界面获取点表值
void read_fix_table(uint16_t fix_val = 0);
//计算CRC32
uint32_t crc32(uint8_t *buf, uint32_t len);
//计算CRC16
uint16_t crc16(uint8_t *data, uint16_t len);
// 按字符分割字符串
std::vector<std::string> split_str(const std::string input, const std::string& regex);
// MAC地址转数组
void transMacStr(const std::string str,const std::string& cspi,uint8_t result[]);
///////////////////////////////////////////////////////////////////////////////////////////////
bool IsEqual(double A, double B);
std::vector<double> LeastSquare(int PointNumber, const double X[], const double Y[]);
std::complex<double> Dft(int PointNumber, const double X[], int Rank);
double Argument(const std::complex<double>& Value);
double NormalizeArgument(double Argument);
///////////////////////////////////////////////////////////////////////////////////////////////
template<uint32_t MAX_BIT_SET_SIZE>
void bitset_2_array(const std::bitset<MAX_BIT_SET_SIZE> &bits, uint32_t n_set_size, 
  char *buf, uint32_t &n_bytes)
{
  n_bytes = 0;
  for (uint32_t i = 0; i < n_set_size; i += 8)
  {
    char ch;
    for (int j = 0; j < 8; ++j)
    {
      if (bits.test(i + j))	// 第i + j位为1
        ch |= (1 << j);
      else
        ch &= ~(1 << j);
    }
    buf[n_bytes++] = ch;
  }
}
template<uint32_t MAX_BIT_SET_SIZE>
void array_2_bitset(const char *buf, uint32_t n_bytes, 
  std::bitset<MAX_BIT_SET_SIZE> &bits, uint32_t &n_set_size)
{
  n_set_size = 0;
  int n_bits = n_bytes * 8;
  for (int i = 0; i < n_bytes; ++i)
  {
    char ch = buf[i];
    int n_offset = i * 8;
    for (uint32_t j = 0; j < 8; ++j)
    {
      bits.set(n_offset + j, ch & (1 << j));	// 第j位为是否为1
      ++n_set_size;
    }
  }
}
uint8_t set_bit(uint8_t data, int index, bool flag);
uint8_t get_mod(uint8_t* data, uint32_t size);
////////////////////////////////////////////////////////////////////////

// 获取当前微秒数
int64_t GetSystemTime();

#define EXECTIME(fun,...) { \
				int64_t funtime = 0; \
				int64_t time1 = 0,time2 = 0,time3 = 0; \
				time1 = GetSystemTime(); \
				time2 = GetSystemTime(); \
				fun(__VA_ARGS__); \
				time3 = GetSystemTime(); \
				funtime = (time3 - time2) - (time2 - time1); \
				std::cout << "函数执行时间" << funtime << std::endl; \
			}


// 磁盘信息传输结构体
struct Disk_info{
	int _used;
	std::string _size_s;
	std::string _used_s;
	std::string _available_s;
	MSGPACK_DEFINE(_used, _size_s, _used_s, _available_s);
};

#ifndef _WIN32
// 执行linux命令并获取返回结果
std::string ExecCmd(std::string cmd);
//获取磁盘大小
Disk_info GetDiskUsage();

// 获取系统配置
DTU::buffer GetSysconfig(uint16_t cmd);

#endif
////////////////////////////////////////////////////////////////////////
struct RemoteCtrlInfo {
	uint16_t cmdFrom;
	uint16_t delay = 20;
  int conn = 0;
  MSGPACK_DEFINE(cmdFrom, delay, conn);
};

#define RC_CMD_101  0x00
#define RC_CMD_104  0x01
#define RC_CMD_TOOL 0x02
#define RC_CMD_PRE  0x03
#define RC_CMD_EXE  0x04
#define RC_CMD_CAN  0x05
#define RC_CMD_LCD  0x06
////////////////////////////////////////////////////////////////////////

// String转uint32
uint32_t IPToInt(const std::string &strIP);
// uint32转String
std::string IntToIP(const uint32_t &value);

float strtofloat(std::string str);

#endif