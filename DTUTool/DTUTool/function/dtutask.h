#pragma once

#include <QString>
#include <QFile>
#include <QString>
#include "dtuprotocol.h"
#include "dtucommon.h"
#include "dtudbmanager.h"
#include "dtusystemconfig.h"

// 写定值
int execute_write_setting(uint16_t cmd, const DTU::buffer& data, uint16_t group);
// 读取定值
int execute_read_setting(uint16_t cmd, DTU::buffer& result, uint16_t group);
// 读取信息
int execute_query_data(uint16_t cmd, DTU::buffer& result);
// 写入参数
int execute_write_data(uint16_t cmd, const DTU::buffer& result);
// 下载文件
int execute_get_file(const std::string& fileName, DTU::buffer& result);
// 读文件列表
int execute_get_dir(const std::string& dirName, uint64_t begin, uint64_t end, FILELIST& result);
// 遥控
int execute_rmctrl(uint16_t fix, uint16_t operate, int delay, int from);
// 获取报告
int execute_get_report(uint16_t id, int min, int max, DTU::ReportBufferAttr& result);
int execute_get_reportno(uint16_t id, uint32_t &reportno);
// 报告清空
int execute_clear_report(uint16_t id);
// 上传文件(打开文件上传)
int execute_set_file(const std::string& srcName, const std::string& destName);
// 上传文件(直接传文件内容)
int execute_set_file(const std::string& destNameName, const DTU::buffer& fileContent);
// 切换定值区
int execute_change_group(uint16_t dst);
// 读取定值区信息
int execute_read_group(uint32_t& curgroup, uint32_t& maxgroup);
// 执行升级程序
int execute_updateprogram(uint16_t tag);
// 获取磁盘容量
int execute_get_disksuage(Disk_info &usage, uint16_t tag);
// 读取GOOSE配置
int execute_read_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg);
// 写入GOOSE配置
int execute_save_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg);
// 设置连接状态
void set_arm_connect_state(bool state);
// 测试是否已经连接
bool execute_test_arm_connect();
// 获取ARM程序路径
int execute_get_filepath(std::string &Path);
// 检查点表是否可以修改
bool execute_fixno_check(DTU::MapFixno type, uint16_t fixno);
// 修改点表
bool execute_fixno_modify(DTU::MapFixno type, uint16_t older, uint16_t newer);

// 向装置传输大文件(10M以上)
int execute_set_file_plus(const std::string& srcName, const std::string& destName);
// 从装置传输文件到PC(10M以上)
int execute_get_file_plus(const std::string& srcName, const std::string& destName);