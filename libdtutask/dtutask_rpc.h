/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_rpc.h
  *Description:
    实现任务与配置工具的交互
  *History:
    1, 创建, wangjs, 2021-8-4
    2，加入目录获取接口, wangjs, 2021-8-23
    3，加入公共单元接收间隔单元主动上送的消息功能, wangjs, 2021-9-6
    4, 加入公共单元与间隔单元的通信接口，包括定值读写，文件，目录功能, wangjs, 2021-9-10
**********************************************************************************/
#ifndef _DTU_TASK_RPC_H
#define _DTU_TASK_RPC_H
#include <dtuprotocol.h>
#include <rest_rpc/rest_rpc.hpp>
#include <dtustructs.h>
#include <dtucommon.h>
#include <dtusystemconfig.h>
#include "dtudbmanager.h"
#include "dtudatastruct.h"

//=======================定值参数操作=========================//
// 写定值
int rpc_write_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, const DTU::buffer& settings, uint16_t group);
// 写参数
int rpc_write_parameter(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, const DTU::buffer& settings);
// 读定值
DTU::buffer rpc_read_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, uint16_t group);
// 读参数
DTU::buffer rpc_read_parameter(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd);
// 读取定值区信息
DTU::buffer rpc_read_group(
    rest_rpc::rpc_service::rpc_conn conn);
// 切换定值区
int rpc_change_group(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t group);
// 选择编辑区
int rpc_select_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t group);
// 定值预设
int rpc_preset_setting(
    rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer& data, uint16_t group);
// 定值撤销
int rpc_revert_setting(
    rest_rpc::rpc_service::rpc_conn conn);
// 定值写入
int rpc_confirm_setting(
    rest_rpc::rpc_service::rpc_conn conn);
//=======================报告档案操作=========================//
// 按范围读取报告内容
DTU::ReportBufferAttr rpc_get_report(rest_rpc::rpc_service::rpc_conn conn, uint16_t reportid, int min, int max);
// 获取最新报告的编号
uint32_t rpc_get_reportno(rest_rpc::rpc_service::rpc_conn conn, uint16_t id);
// 清空报告内容
int rpc_clear_report(rest_rpc::rpc_service::rpc_conn conn, uint16_t reportid);
//=======================控制操作============================//
// 控制
int rpc_rmctrl(
    rest_rpc::rpc_service::rpc_conn conn,uint16_t fix,uint16_t operate,RemoteCtrlInfo info);
// 升级操作
int rpc_update(
  rest_rpc::rpc_service::rpc_conn conn,uint16_t tag);
// 获取磁盘容量
Disk_info rpc_disksuage(rest_rpc::rpc_service::rpc_conn conn,uint16_t tag);
// 获取文件路径
DTU::buffer rpc_filepath(rest_rpc::rpc_service::rpc_conn conn);
// DSP写控制
int rpc_control(rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd);
////////////////////////////////
//=======================文件操作============================//
// 读取目录
FILELIST
rpc_get_dir(rest_rpc::rpc_service::rpc_conn conn,const std::string& dir, uint64_t tbegin, uint64_t tend);
// 读取文件信息
FILEINFO rpc_get_fileinfo(rest_rpc::rpc_service::rpc_conn conn,const std::string& dir);
// 读取文件大小
uint64_t rpc_get_file_size(rest_rpc::rpc_service::rpc_conn conn, const std::string& file);
// 读取文件
DTU::buffer rpc_get_file(rest_rpc::rpc_service::rpc_conn conn,const std::string& file);
// 读取大文件(10M以上文件需要调用此函数)
DTU::buffer rpc_get_file_plus(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, uint64_t offset, uint64_t size);
// 上传文件
int rpc_set_file(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, const DTU::buffer& content);
// 上传文件(10M以上文件需要调用此函数)
int rpc_set_file_plus(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, const DTU::buffer& content, bool transOK);

//======================= 间隔单元通知 ==================================//
int rpc_bay_notify(rest_rpc::rpc_service::rpc_conn conn,const DTU::buffer& src, int bayca);


//======================= GOOSE配置相关 ==================================//
DTUCFG::DSYSCFG::GooseCFG rpc_read_goose_cfg(rest_rpc::rpc_service::rpc_conn conn);

bool rpc_save_goose_cfg(rest_rpc::rpc_service::rpc_conn conn,const DTUCFG::DSYSCFG::GooseCFG &data);

//========================= 点表修改相关 ==================================//
bool rpc_fixno_check(rest_rpc::rpc_service::rpc_conn conn, int type, uint16_t fixno);
bool rpc_fixno_modify(rest_rpc::rpc_service::rpc_conn conn, int type, uint16_t older, uint16_t newer);


int rpc_async_time(rest_rpc::rpc_service::rpc_conn conn, DTU::buffer result);
//========================= 间隔单元通信相关 ==================================//
std::vector<DTU::buffer> rpc_proto_read_param(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo);
std::vector<DTU::buffer> rpc_proto_write_param(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo);
std::vector<DTU::buffer> rpc_proto_file_request(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data);
std::vector<DTU::buffer> rpc_proto_change_current_group(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data);
std::vector<DTU::buffer> rpc_proto_read_current_group(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo);
std::vector<DTU::buffer> rpc_proto_romate_ctrl(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo);
std::vector<DTU::buffer> rpc_proto_IMasterConnect(rest_rpc::rpc_service::rpc_conn conn, int type, bool curdev);

std::map<DTU::IOA, bool> rpc_proto_get_yx(rest_rpc::rpc_service::rpc_conn conn);
std::map<DTU::IOA, float> rpc_proto_get_yc(rest_rpc::rpc_service::rpc_conn conn);

// 遥调附录A
std::map<DTU::IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> rpc_proto_readParam_A(rest_rpc::rpc_service::rpc_conn conn, std::vector<DTU::IOA> ioavec);
void rpc_proto_preset_A(rest_rpc::rpc_service::rpc_conn conn, std::map<uint16_t, DTU::buffer> presetmap);
void rpc_proto_confirm_A(rest_rpc::rpc_service::rpc_conn conn, bool isConfirm);


// 遥调 附录B
std::map<DTU::IOA, DTU::buffer> rpc_proto_readParam_B(rest_rpc::rpc_service::rpc_conn conn, std::vector<DTU::IOA> ioavec);
void rpc_proto_preset_B(rest_rpc::rpc_service::rpc_conn conn, std::map<uint16_t, DTU::buffer> presetmap);
void rpc_proto_confirm_B(rest_rpc::rpc_service::rpc_conn conn, bool isConfirm);

#endif
