/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_rpc.h
  *Description:
    实现任务与配置工具的交互
  *History:
    1, 创建, wangjs, 2021-8-4
    2，加入定值区切换任务, wangjs, 2021-8-16
    3，加入目录获取接口, wangjs, 2021-8-23
    4，加入公共单元接收间隔单元主动上送的消息功能, wangjs, 2021-9-6
**********************************************************************************/
#include "dtutask_rpc.h"
#include "dtutask_dsp.h"
#include "dtutask_iec.h"
#include <dtuerror.h>
#include <dtucmdcode.h>
#include <dtucommon.h>
#include <fstream>
#include <sstream>
#include <dtulog.h>
#include <dtustorage.h>
#include <dturulesfile.h>
#include <dturuleshandle.h>
#include <dtusystemconfig.h>
#include <string>
#include <unordered_map>
#include "dspcomm.h"
#include <dtunetconfig.h>
#include <dtuparamconfig.h>
#include <vector>
#include <tuple>
#include <dturulesnotify.h>
#include "dtuhalasdu.h"
#include "dtudatastruct.h"

using namespace DTUCFG;

// 升级DSP
int update_dsp();
// 升级ARM
int update_arm();
// 升级FPGA
int update_system();
// 更新配置文件
int update_config();
// 更新GOOSE文件
int update_goose();
// 执行重启
int update_restart();

// FPGA编码转换
uint8_t *apx_ram_selected(uint8_t *buffer, uint32_t length_byte, uint32_t &desc_byte_len);

int update_system() {
    int retCode = DTU_SUCCESS;
    DTULOG(DTU_INFO, (char *)"正在更新系统相关程序...");
    
    pid_t status;
    std::string exePath = get_exec_dir();
    std::string cmd = "ls " + exePath + "/update/system/";
    std::string fileList = ExecCmd(cmd);
    cmd.clear();

    // 系统文件 u-boot-860.img u-boot-spl.brn linux-dtu.itb
    cmd = "cd " + exePath + "/update/system/;";

    auto u860_pos = fileList.find("u-boot-860.img");
    auto uspl_pos = fileList.find("u-boot-spl.brn");
    auto ldtu_pos = fileList.find("linux-dtu.itb");

    if(u860_pos != fileList.npos)
    {
        cmd = cmd + "dd if=u-boot-860.img of=/dev/mtdblock0;";
    }
    if(uspl_pos != fileList.npos)
    {
        cmd = cmd + "dd if=u-boot-spl.brn of=/dev/mtdblock0;";
    }
    if(ldtu_pos != fileList.npos)
    {
        cmd = cmd + "dd if=linux-dtu.itb of=/dev/mmcblk0p1;";
    }

    cmd = cmd + "rm -rf " + exePath + "/update/system/*;";

    status = system(cmd.c_str());

    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_DSP_ERROR;
    }

    return retCode;
}

// 升级DSP
int update_dsp() {
    int retCode = DTU_SUCCESS;
    DTULOG(DTU_INFO, (char *)"正在更新DSP相关程序...");
    pid_t status;
    std::string exePath = get_exec_dir();
    std::string cmd = "ls " + exePath + "/update/ | grep .img";
    std::string fileList = ExecCmd(cmd);
    cmd.clear();

    cmd = "cd " + exePath + "/update/;";

    auto ck8100_pos = fileList.find("taeho-8100-9200a.img");
    auto ck8101_pos = fileList.find("taeho-8101-9200b.img");
    
    if(ck8100_pos != fileList.npos)
    {
        cmd = cmd + "dd if=taeho-8100-9200a.img of=/dev/mtdblock7;";
    }
    if(ck8101_pos != fileList.npos)
    {
        cmd = cmd + "dd if=taeho-8101-9200b.img of=/dev/mtdblock6;";
    }
    cmd = cmd + "rm -rf " + exePath + "/update/*.img;";
    status = system(cmd.c_str());

    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_DSP_ERROR;
    }
    return DTU_SUCCESS;
}

int update_arm() {
    int retCode = DTU_SUCCESS;
    DTULOG(DTU_INFO, (char *)"正在更新ARM程序...");
    pid_t status;
    std::string exePath = get_exec_dir();
    // ps -ef | grep fcuwatchdog | grep -v grep | awk '{print $1}' | xargs kill -9;
    std::string cmd = "mv -f " + exePath + "/update/arm " + exePath + "/sdl9200;\
                       chmod +x " + exePath + "/sdl9200;";

    status = system(cmd.c_str());

    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_ARM_ERROR;
    }

    return DTU_SUCCESS;
}

//
int update_config() {
    int retCode = DTU_SUCCESS;
    DTULOG(DTU_INFO, (char *)"正在更新配置文件...");
    pid_t status;
    std::string exePath = get_exec_dir();
    // 1.把所有配置文件移动到config中
    // 2.重启程序
    std::string cmd = "mv " + exePath + "/update/config/* " + exePath + "/config/; \
                    ps -ef | grep sdl9200 | grep -v grep | awk '{print $1}' | xargs kill -9;";

    status = system(cmd.c_str());

    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_CONFIG_ERROR;
    }
    return DTU_SUCCESS;
}

int update_goose() {
    int retCode = DTU_SUCCESS;
    DTULOG(DTU_INFO, (char *)"正在更新GOOSE程序...");
    pid_t status;
    std::string exePath = get_exec_dir();
    // ps -ef | grep fcuwatchdog | grep -v grep | awk '{print $1}' | xargs kill -9;
    std::string cmd = "rm -f " + exePath + "/sdl9200G; \
                      chmod +x " + exePath + "/update/goose; \
                      cp -f " + exePath + "/update/goose " + exePath + "/sdl9200G;";

    status = system(cmd.c_str());

    // 重启GOOSE程序
    cmd = "ps -ef | grep sdl9200G | grep -v grep | awk '{print $1}' | xargs kill -9;";
    status = system(cmd.c_str());

    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_GOOSE_ERROR;
    }
    return retCode;
}

// 系统重启
int update_restart() {
    int retCode = DTU_SUCCESS;
    pid_t status;
    system("sync");
    status = system("reboot -f;");
    if (-1 == status) {
        DTULOG(DTU_ERROR, (char *)"系统命令执行失败 error:%s", strerror(errno));
        return DTU_REBOOT_ERROR;
    }
    return DTU_SUCCESS;
}

uint8_t *apx_ram_selected(uint8_t *buffer, uint32_t length_byte, uint32_t &desc_byte_len) {
    for (uint32_t i = 0; i < length_byte; i++) {
        if ('E' == buffer[i]) {
            if (i + 5 >= length_byte) { //防止未找到字符串越界
                break;
            }
            if (('P' == buffer[i + 1] && 'C' == buffer[i + 2] && 'S' == buffer[i + 3] && '4' == buffer[i + 4]) ||
                ('P' == buffer[i + 1] && 'C' == buffer[i + 2] && 'S' == buffer[i + 3] && '1' == buffer[i + 4] &&
                 '6' == buffer[i + 5]) ||
                ('P' == buffer[i + 1] && 'C' == buffer[i + 2] && 'S' == buffer[i + 3] && '6' == buffer[i + 4] &&
                 '4' == buffer[i + 5])) {
                uint32_t length = 0;
                uint32_t index = 0;
                if ('4' == buffer[i + 4]) {
                    index = i + 4 + 20;
                    length = length_byte - 61 - (i + 4 + 20);
                } else if ('1' == buffer[i + 4]) {
                    index = i + 5 + 20;
                    length = length_byte - 66 - (i + 5 + 20);
                } else {
                    index = i + 5 + 20;
                    length = length_byte - 62 - (i + 5 + 20);
                }
                uint8_t *desc_buffer = new uint8_t[length];
                memset(desc_buffer, 0, length);
                memcpy(desc_buffer, &buffer[index], length);
                for (uint32_t j = 0; j < length; j++) {
                    desc_buffer[j] = ((desc_buffer[j] & 0x01) << 7) | (((desc_buffer[j] >> 1) & 0x01) << 6) |
                                     (((desc_buffer[j] >> 2) & 0x01) << 5) | (((desc_buffer[j] >> 3) & 0x01) << 4) |
                                     (((desc_buffer[j] >> 4) & 0x01) << 3) | (((desc_buffer[j] >> 5) & 0x01) << 2) |
                                     (((desc_buffer[j] >> 6) & 0x01) << 1) | (((desc_buffer[j] >> 7) & 0x01));
                }
                desc_byte_len = length;
                return desc_buffer;
            } else {
                continue;
            }
        } else {
            continue;
        }
    }
    desc_byte_len = 0;
    return NULL;
}

int rpctask_execute_restart(uint16_t tag) {
    if (tag) {
        return update_restart();
    } else {
        return DTU_SUCCESS;
    }
}

//=======================定值参数操作=========================//
// 写定值
int rpc_write_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, const DTU::buffer& settings, uint16_t group)
{
    int ret = DTU_SUCCESS;
    try
    {
        uint16_t currentGroup = DTU::DSTORE::instance().get_current_group();
        switch(cmd)
        {
            case PC_W_FIX_AREA_INFO:{
                DTU::buffer destbuf = settings;
                uint32_t dest = destbuf.value<uint32_t>();
                if(dest == currentGroup)
                    return ret;
                DTU::DSTORE::instance().change_setting_num(dest,dsptask_execute_write);
            };break;
            case PC_W_COMM_FIX:{
                ret = dsptask_execute_write(cmd, settings);
                break;
            }
            default:{
                if (currentGroup == group)
                {
                    // 如果是当前定值区，则需要下发到装置
                    /////////////////////////////////////////////////
                    ret = dsptask_execute_write(cmd, settings);
                    if (ret != DTU_SUCCESS) {
                        DTULOG(DTU_ERROR, (char *)"rpc_write_setting 写入参数执行失败");
                        // 直接返回，不写入文件
                        return DTU_DSP_ERROR;
                    }
                }
                // 写入保存
                DTU::DSTORE::instance().write_setting_data(GET_PARAM_ID_BY_CMD(cmd), settings, group);
            }
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"rpc_write_setting 执行错误:%s", e.what());
    }
    return ret;
}

// 写参数
int rpc_write_parameter(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, const DTU::buffer& settings)
{
    if (cmd == PC_W_COMM_FIX)
    {
        // 重新设置网络配置
        int nret = DTUCFG::netconfigure::instance().save_net(settings.get(0,sizeof(uint32_t)*4*3));
        if( nret != DTU_SUCCESS )
        {
            DTULOG(DTU_ERROR, (char*)"rpc_write_parameter 设置网络参数错误");
            return nret;
        }
    }
    else
    {
        if (DTU_SUCCESS != dsptask_execute_write(cmd, settings)) {
            return DTU_DSP_ERROR;
        }

        // 如果是写时钟命令则更改系统时间
        if(cmd == PC_W_CLK)
        {
            AutoTime setTime;
            setTime.CalibrateSystemTimeOnce();
        }
    }
    // 保存整定参数
    if (cmd == PC_W_ADJ_FIX)
    {
        DTU::DSTORE::instance().write_adj_data(settings);
        DTU::DSTORE::instance().save_adj_data();
    }
    return DTU_SUCCESS;
}
// 读定值
DTU::buffer rpc_read_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd, uint16_t group)
{
    DTU::buffer result;
    try
    {
        uint32_t currentGroup = DTU::DSTORE::instance().get_current_group();
        switch(cmd)
        {
            // 读取定值区命令特殊处理
            case PC_R_FIX_AREA_INFO:{
                result.append((char*)&currentGroup,sizeof(currentGroup));
                return result;
            };break;
            case PC_R_COMM:{
                dsptask_execute_read(cmd, result);
                return result;
            };break;
            case PC_R_ADJ_LCD_FIX:{
                dsptask_execute_read(cmd, result);
                return result;
            };break;
            default:{
                if (currentGroup == group)
                {
                    dsptask_execute_read(cmd, result);
                    if (result.size() != 0)
                    {
                        DTU::DSTORE::instance().write_setting_data(GET_PARAM_ID_BY_CMD(cmd), result, group);
                    }
                }
                else
                {
                    DTULOG(DTU_WARN,"当前索要定制区[%u],当前设备定值区[%u],已从存储加载",group,currentGroup);
                    DTU::DSTORE::instance().read_setting_data(GET_PARAM_ID_BY_CMD(cmd), result, group);
                }
            }
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"rpc_read_setting 执行错误:%s", e.what());
        result.remove();
    }
    return result;
}

// 读参数
DTU::buffer rpc_read_parameter(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd)
{
    DTU::buffer result;
    if(cmd == PC_R_COMM)
    {
        // 网络配置文件内容
        return DTUCFG::netconfigure::instance().get_net_param_from_file();
    }
    // 整定结果
    else if (cmd == PC_R_ADJ_FIX)
    {
        return DTU::DSTORE::instance().read_adj_data();
    }
    // 读取软件版本
    else if (cmd == PC_R_SOFT_PROG)
    {
        dsptask_execute_query(cmd, result);
    }
    // 获取系统配置
    else if(cmd == DTU_GET_SYS_CONFIG)
    {
        return GetSysconfig(DTU_GET_SYS_CONFIG);
    }
    // 获取规约配置
    else if(cmd == DTU_GET_PROTO_CONFIG)
    {
        return GetSysconfig(DTU_GET_PROTO_CONFIG);
    }
    else
    {
        dsptask_execute_query(cmd, result);
    }
    return result;
}
// 读取定值区信息
DTU::buffer rpc_read_group(
    rest_rpc::rpc_service::rpc_conn conn)
{
    DTU::buffer result;
    uint32_t currentgroup = DTU::DSTORE::instance().get_current_group();
    uint32_t maxgroup = DTU::DSTORE::instance().get_max_group();

    result.append((char*)&currentgroup, sizeof(uint32_t));
    result.append((char*)&maxgroup, sizeof(uint32_t));

    return result;
}
// 切换定值区
int rpc_change_group(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t group)
{
    try
    {
        if (!DTU::DSTORE::instance().change_setting_num(group,dsptask_execute_write))
            return DTU_UNKNOWN_ERROR;
        else
            return DTU_SUCCESS;
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"rpc_change_group 执行失败:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}
// 选择编辑区
int rpc_select_setting(
    rest_rpc::rpc_service::rpc_conn conn, uint16_t group)
{
    DTU::DSTORE::instance().seletc_edit_sg(group);
    return 0;
}
// 定值预设
int rpc_preset_setting(
    rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer& data, uint16_t group)
{
    // 只有规约会使用
    DTU::DSTORE::instance().seletc_edit_sg(group);
    DTU::DSTORE::instance().preset_setting_data(data);
    return DTU_SUCCESS;
}
// 定值撤销
int rpc_revert_setting(
    rest_rpc::rpc_service::rpc_conn conn)
{
    DTU::DSTORE::instance().revert_setting_data();
    return 0;
}
// 定值写入
int rpc_confirm_setting(
    rest_rpc::rpc_service::rpc_conn conn)
{
    // 确认预设定值并下发
    DTU::DSTORE::instance().save_presetting_data(dsptask_execute_write);
    return DTU_SUCCESS;
}

// 读取报告内容
DTU::ReportBufferAttr rpc_get_report(rest_rpc::rpc_service::rpc_conn conn, uint16_t reportid, int min ,int max)
{
    DTU::ReportBufferAttr data;
    try
    {
        DTU::DSTORE::instance().get_report_range(reportid,min,max,data);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, "rpc_get_report 发生错误:%s", e.what());
    }
    return data;
}

uint32_t rpc_get_reportno(rest_rpc::rpc_service::rpc_conn conn, uint16_t id)
{
    try
    {
        return DTU::DSTORE::instance().get_cur_report_no(id);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, "rpc_get_reportno 发生错误:%s", e.what());
    }
    return 0;
}

// 清空报告内容
int rpc_clear_report(rest_rpc::rpc_service::rpc_conn conn, uint16_t reportid)
{
    try
    {
        bool ret = DTU::DSTORE::instance().clear_report(reportid);
        if(ret)
            return DTU_SUCCESS;
        else
            return DTU_UNKNOWN_ERROR;
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR, "rpc_clear_report 发生错误:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}
//=======================控制操作============================//
// 遥控
int rpc_rmctrl(
    rest_rpc::rpc_service::rpc_conn conn,uint16_t fix,uint16_t operate,RemoteCtrlInfo info)
{
    // 包括测试命令，启动命令，程序更新
    switch(operate)
    {
        case PC_W_DSP_REBOOT: {
        };break;
        case PC_W_ARM_REBOOT: {
            update_restart();
        };break;
        default:
        {
            return dsptask_execute_rmctrl(fix, operate, info);
        }
    }
    return DTU_SUCCESS;
}

int rpc_update(
    rest_rpc::rpc_service::rpc_conn conn,uint16_t tag)
{
     try {
        //升级系统程序
        if ((tag & 0x001) == 0x001) {
            if (DTU_FAILED(update_system())) {
                DTULOG(DTU_ERROR, (char *)"升级系统程序失败");
                return DTU_DSP_ERROR;
            }
        }
        // 升级DSP
        if ((tag & 0x010) == 0x010) {
            if (DTU_FAILED(update_dsp())) {
                DTULOG(DTU_ERROR, (char *)"升级DSP程序失败");
                return DTU_DSP_ERROR;
            }
        }

        // 升级arm
        if ((tag & 0x100) == 0x100) {
            if (DTU_FAILED(update_arm())) {
                DTULOG(DTU_ERROR, (char *)"升级ARM程序失败");
                return DTU_ARM_ERROR;
            }
        }

        // 更新配置文件
        if ((tag & 0x1000) == 0x1000) {
            if (DTU_FAILED(update_config())) {
                DTULOG(DTU_ERROR, (char *)"更新配置文件失败");
                return DTU_CONFIG_ERROR;
            }
        }

        // 更新配置文件
        if ((tag & 0x02) == 0x02) {
            if (DTU_FAILED(update_goose())) {
                DTULOG(DTU_ERROR, (char *)"更新GOOSE文件失败");
                return DTU_GOOSE_ERROR;
            }
        }

        // 延时防止有的指令未执行完就重启
        std::this_thread::sleep_for(std::chrono::seconds(3));

        //设置需要重启的标志位,任何一个标志位存在系统都将重启
        //需要重启:ARM升级 DSP升级 system升级
        if ((tag & 0x100) == 0x100  || (tag & 0x010) == 0x010 || (tag & 0x001) == 0x001) {
            if (DTU_FAILED(update_restart())) {
                DTULOG(DTU_ERROR, (char *)"重启失败");
                return DTU_REBOOT_ERROR;
            }
        }
    } catch (const std::exception &e) {
        DTULOG(DTU_ERROR, (char *)"rpctask_execute_update 异常:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}
// 获取磁盘容量
Disk_info rpc_disksuage(rest_rpc::rpc_service::rpc_conn conn, uint16_t tag)
{
    return GetDiskUsage();
}
// 获取文件路径
DTU::buffer rpc_filepath(rest_rpc::rpc_service::rpc_conn conn)
{
    std::string path = get_exec_dir();
    DTU::buffer buf(path.c_str(),path.size());
    return std::move(buf);
}
// DSP写控制
int rpc_control(rest_rpc::rpc_service::rpc_conn conn, uint16_t cmd)
{
    return dsptask_execute_control(cmd);
}
////////////////////////////////
//=======================文件操作============================//
// 读取目录
FILELIST
rpc_get_dir(
    rest_rpc::rpc_service::rpc_conn conn,const std::string& dir,uint64_t tbegin, uint64_t tend)
{
    std::string fullDir = get_exec_dir() + dir;
    
    DTULOG(DTU_INFO, "rpc_get_dir 获取目录%s", fullDir.c_str());
    // 获取目录
    FILELIST fileList;
    
    get_dir_files(fullDir, fileList);
    //
    if (tbegin != 0 || tend != 0){
        auto ita = fileList.begin();
        while (ita != fileList.end()) 
        {
            if (std::get<2>(*ita) < tbegin || std::get<2>(*ita) > tend) 
            {
                fileList.erase(ita);
                continue;
            }
            ita++;
        }
    }
    return fileList;
}
// 读取文件信息
FILEINFO rpc_get_fileinfo(
  rest_rpc::rpc_service::rpc_conn conn,const std::string& dir)
{
    std::string fullName = get_exec_dir() + dir;

    return get_file_info(fullName);
}

uint64_t rpc_get_file_size(rest_rpc::rpc_service::rpc_conn conn, const std::string& file)
{
    std::string fullName = get_exec_dir() + file;
    return get_file_size(fullName);
}

// 读取文件
DTU::buffer rpc_get_file(
    rest_rpc::rpc_service::rpc_conn conn,const std::string& file)
{
    std::string fullName = get_exec_dir() + file;
    DTU::buffer fileContent;
    get_file(fullName, fileContent);

    return fileContent;
}

DTU::buffer rpc_get_file_plus(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, uint64_t offset, uint64_t size)
{
    std::string fullName = get_exec_dir() + file;
    DTU::buffer fileContent;
    get_file(fullName, fileContent, offset, size);

    return fileContent;
}

// 上传文件
int rpc_set_file(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, const DTU::buffer& content)
{
    return save_file(file, content);
}

int rpc_set_file_plus(rest_rpc::rpc_service::rpc_conn conn,const std::string& file, const DTU::buffer& content, bool transOK)
{
    return save_file(file, content, transOK);
}

int rpc_bay_notify(rest_rpc::rpc_service::rpc_conn conn, 
   const DTU::buffer& src, int bayCA)
{
    DTU::dtuprotocol proto;
    proto.unpackage(src);
    if(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use)
    {
        DTU::DRULESNotify::instance().notify(proto, 101, true, bayCA);
    }

    if(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use)
    {
        DTU::DRULESNotify::instance().notify(proto, 104, true, bayCA);
    }
    
    return DTU_SUCCESS;
}

//======================= GOOSE配置相关 ==================================//
DSYSCFG::GooseCFG rpc_read_goose_cfg(rest_rpc::rpc_service::rpc_conn conn)
{
    auto data = DSYSCFG::instance().read_from_file();
    return data;
}

bool rpc_save_goose_cfg(rest_rpc::rpc_service::rpc_conn conn, const DSYSCFG::GooseCFG &data)
{
    return DSYSCFG::instance().save_to_file(data);;
}

bool rpc_fixno_check(rest_rpc::rpc_service::rpc_conn conn, int type, uint16_t fixno)
{
    return DTU::DBManager::instance().CheckFixnoReady(static_cast<DTU::MapFixno>(type),fixno);
}

bool rpc_fixno_modify(rest_rpc::rpc_service::rpc_conn conn, int type, uint16_t older, uint16_t newer)
{
    return DTU::DBManager::instance().ModifyFixno(static_cast<DTU::MapFixno>(type), older, newer);
}

int rpc_async_time(rest_rpc::rpc_service::rpc_conn conn, DTU::buffer result)
{
    return iectask_execute_time(result);
}

//========================= 间隔单元通信相关 ==================================//
std::vector<DTU::buffer> rpc_proto_read_param(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo)
{
    DTU::buffer datat = data;
    return DTU::dtuHALhandler::readParam(datat, rinfo);
}

std::vector<DTU::buffer> rpc_proto_write_param(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo)
{
    return DTU::dtuHALhandler::writeParam(data, rinfo);
}

std::vector<DTU::buffer> rpc_proto_file_request(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data)
{
    return DTU::dtuHALhandler::fileRequest(data);
}

std::vector<DTU::buffer> rpc_proto_change_current_group(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data)
{
    return DTU::dtuHALhandler::changeCurrentGroup(data);
}

std::vector<DTU::buffer> rpc_proto_read_current_group(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo)
{
    return DTU::dtuHALhandler::readCurrentGroup(data, rinfo);
}

std::vector<DTU::buffer> rpc_proto_romate_ctrl(rest_rpc::rpc_service::rpc_conn conn, const DTU::buffer &data, RemoteCtrlInfo rinfo)
{
    return DTU::dtuHALhandler::remoteCtrl(data, rinfo);
}

std::vector<DTU::buffer> rpc_proto_IMasterConnect(rest_rpc::rpc_service::rpc_conn conn, int type, bool curdev)
{
    return DTU::dtuHALhandler::IMasterConnect(type, curdev);
}

std::map<DTU::IOA, bool> rpc_proto_get_yx(rest_rpc::rpc_service::rpc_conn conn)
{
    return DTU::dtuHALhandler::GetYXValue();
}

std::map<DTU::IOA, float> rpc_proto_get_yc(rest_rpc::rpc_service::rpc_conn conn)
{
    return DTU::dtuHALhandler::GetYCValue();
}

std::map<DTU::IOA, DTU::buffer> rpc_proto_readParam_B(rest_rpc::rpc_service::rpc_conn conn, std::vector<DTU::IOA> ioavec)
{
    return DTU::dtuHALhandler::readParam_B(ioavec);
}

void rpc_proto_preset_B(rest_rpc::rpc_service::rpc_conn conn, std::map<uint16_t, DTU::buffer> presetmap)
{
    DTU::dtuHALhandler::Preset_B(presetmap);
}

void rpc_proto_confirm_B(rest_rpc::rpc_service::rpc_conn conn, bool isConfirm)
{
    DTU::dtuHALhandler::Confirm_B(isConfirm);
}

std::map<DTU::IOA, std::tuple<uint8_t, uint8_t, DTU::buffer>> rpc_proto_readParam_A(rest_rpc::rpc_service::rpc_conn conn, std::vector<DTU::IOA> ioavec)
{
    return DTU::dtuHALhandler::readParam_A(ioavec);
}

void rpc_proto_preset_A(rest_rpc::rpc_service::rpc_conn conn, std::map<uint16_t, DTU::buffer> presetmap)
{
    DTU::dtuHALhandler::Preset_A(presetmap);
}

void rpc_proto_confirm_A(rest_rpc::rpc_service::rpc_conn conn, bool isConfirm)
{
    DTU::dtuHALhandler::Confirm_A(isConfirm);
}