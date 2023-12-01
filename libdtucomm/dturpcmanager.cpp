/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturpcmanager.cpp
  *Description:
    RPC连接客户端管理
  *History:
    1, 创建, wangjs, 2021-8-4
    2, 加入ping测试函数,用于展示配置工具通信状态, wangjs, 2021-8-9
    3, 加入档案文件清空函数, wangjs, 2021-8-19
**********************************************************************************/
#include "dturpcmanager.h"
#include <dtulog.h>
#include <dtutask_rpc.h>
#include <chrono>
#include "dtunotifymanager.h"

using namespace DTU;

// 启动RPC服务
bool DTURPCMgr::start_dtu_rpc(uint16_t port) 
{
    if (!DTURPC::instance().init(port)) 
    {
        return false;
    }
    // 功能函数注册
    DTURPC::instance().registerfunc("rpc_write_setting", rpc_write_setting);
    DTURPC::instance().registerfunc("rpc_read_setting", rpc_read_setting);
    DTURPC::instance().registerfunc("rpc_rmctrl", rpc_rmctrl);
    DTURPC::instance().registerfunc("rpc_get_dir", rpc_get_dir);
    DTURPC::instance().registerfunc("rpc_write_parameter", rpc_write_parameter);
    DTURPC::instance().registerfunc("rpc_read_parameter", rpc_read_parameter);
    DTURPC::instance().registerfunc("rpc_read_group", rpc_read_group);
    DTURPC::instance().registerfunc("rpc_change_group", rpc_change_group);
    DTURPC::instance().registerfunc("rpc_select_setting", rpc_select_setting);
    DTURPC::instance().registerfunc("rpc_preset_setting", rpc_preset_setting);
    DTURPC::instance().registerfunc("rpc_revert_setting", rpc_revert_setting);
    DTURPC::instance().registerfunc("rpc_update", rpc_update);
    DTURPC::instance().registerfunc("rpc_get_fileinfo", rpc_get_fileinfo);
    DTURPC::instance().registerfunc("rpc_get_file_size", rpc_get_file_size);
    DTURPC::instance().registerfunc("rpc_get_file", rpc_get_file);
    DTURPC::instance().registerfunc("rpc_get_file_plus", rpc_get_file_plus);
    DTURPC::instance().registerfunc("rpc_set_file", rpc_set_file);
    DTURPC::instance().registerfunc("rpc_set_file_plus", rpc_set_file_plus);
    DTURPC::instance().registerfunc("rpc_get_report", rpc_get_report);
    DTURPC::instance().registerfunc("rpc_get_reportno", rpc_get_reportno);
    DTURPC::instance().registerfunc("rpc_clear_report", rpc_clear_report);
    DTURPC::instance().registerfunc("rpc_disksuage", rpc_disksuage);
    DTURPC::instance().registerfunc("rpc_filepath", rpc_filepath);
    DTURPC::instance().registerfunc("rpc_control", rpc_control);
    DTURPC::instance().registerfunc("rpc_read_goose_cfg", rpc_read_goose_cfg);
    DTURPC::instance().registerfunc("rpc_save_goose_cfg", rpc_save_goose_cfg);
    DTURPC::instance().registerfunc("rpc_confirm_setting", rpc_confirm_setting);
    
    // 间隔单元通信函数
    DTURPC::instance().registerfunc("rpc_async_time", rpc_async_time);
    DTURPC::instance().registerfunc("rpc_proto_read_param", rpc_proto_read_param);
    DTURPC::instance().registerfunc("rpc_proto_write_param", rpc_proto_write_param);
    DTURPC::instance().registerfunc("rpc_proto_file_request", rpc_proto_file_request);
    DTURPC::instance().registerfunc("rpc_proto_change_current_group", rpc_proto_change_current_group);
    DTURPC::instance().registerfunc("rpc_proto_read_current_group", rpc_proto_read_current_group);
    DTURPC::instance().registerfunc("rpc_proto_romate_ctrl", rpc_proto_romate_ctrl);
    DTURPC::instance().registerfunc("rpc_proto_IMasterConnect", rpc_proto_IMasterConnect);

    DTURPC::instance().registerfunc("rpc_proto_get_yx", rpc_proto_get_yx);
    DTURPC::instance().registerfunc("rpc_proto_get_yc", rpc_proto_get_yc);

    DTURPC::instance().registerfunc("rpc_proto_readParam_B", rpc_proto_readParam_B);
    DTURPC::instance().registerfunc("rpc_proto_preset_B", rpc_proto_preset_B);
    DTURPC::instance().registerfunc("rpc_proto_confirm_B", rpc_proto_confirm_B);

    DTURPC::instance().registerfunc("rpc_proto_readParam_A", rpc_proto_readParam_A);
    DTURPC::instance().registerfunc("rpc_proto_preset_A", rpc_proto_preset_A);
    DTURPC::instance().registerfunc("rpc_proto_confirm_A", rpc_proto_confirm_A);

    // 间隔单元上送
    DTURPC::instance().registerfunc("rpc_bay_notify", rpc_bay_notify);

    // 点表修改
    DTURPC::instance().registerfunc("rpc_fixno_check", rpc_fixno_check);
    DTURPC::instance().registerfunc("rpc_fixno_modify", rpc_fixno_modify);

    // RPC服务运行
    DTURPC::instance().async_run();

    return true;
}

void DTURPCMgr::stop_dtu_rpc() {
    std::lock_guard<std::mutex> lock(_lock);
    DTURPC::instance().stop();
}

/////////// 主动推送
// 数据主动上送到配置工具
void DTURPCMgr::notify_dtu_client(const dtuprotocol &proto) {
    DTURPC::instance().publish("report", proto);
    DTULOG(DTU_INFO, "向配置工具发送数据");
}

// 规约内容主动推送到工具
void DTURPCMgr::notify_cs_client(uint8_t cmd,const dtuprotocol& proto) {
    switch(cmd)
    {
        case CSLOG:DTURPC::instance().publish("CSLOG", proto);break;
        case CSMSG:DTURPC::instance().publish("CSMSG", proto);break;
        default:
            DTULOG(DTU_ERROR, "notify_cs_client() 错误的命令%u",cmd);
    }
}

void DTURPCMgr::notify_goose_client(const dtuprotocol& proto) {
    DTURPC::instance().publish("GOOSE", proto);
}