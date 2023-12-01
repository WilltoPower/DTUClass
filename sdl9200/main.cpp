/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_dsp.h
  *Description:
    DTU可执行程序实现
  *History:
    1, 创建, wangjs, 2021-8-10
        2, 加入串口功能,lhy 2021-9-2
**********************************************************************************/
#include <unistd.h>
#include <dtusystemconfig.h>
#include <dtulog.h>
#include <iostream>
#include <dtunetconfig.h>
#include <dspcomm.h>
#include <dtuserial.h>
#include <dturpcmanager.h>
#include <dtu104server.h>
#include <dtu101slave.h>
#include <dtu101slave_a.h>
#include <dtu101master.h>
#include <dtucommon.h>
#include <dtustorage.h>
#include <dturulesfile.h>
#include <dtuparamconfig.h>
#include <dtu101master.h>
#include <dtutask_dsp.h>
#include <dtuversion.h>
#include <dtusystemconfig.h>
#include <dtugooseservice.h>

#include <dtuconnect.h>

#include "dtuioamap.h"
#include "dtusoemap.h"

void waitcommand();
void create_directory();

// 信号处理
// void sighandle(int sig)
// {
//     printf("捕获到信号%d", sig);
//     /*释放资源*/
//     exit(0);
// }


using namespace DTU;
using namespace DTUCFG;

int main(int argc, char *argv[]) 
{
    Lib60870_enableDebugOutput(false);

    // 注册CTRL-C信号处理函数
    // signal(SIGINT, sighandle);
    try 
    {
        DTULOG(DTU_INFO, (char *)"系统磁盘已使用[%u%%]", GetDiskUsage()._used);
        std::string strexepath = get_exec_dir();
        // 创建目录
        create_directory();
        // 加载IOAMAP
        DTU::IOAMap::instance()->readIOAFromFile();
        // 加载所有配置
        DSYSCFG::instance().load();
        // 设置网络参数
        DTUCFG::netconfigure::instance().load(strexepath + "/config/netcfg.json");
        // 加载参数文件
        DTU::DSTORE::instance().load(strexepath + "/config/dtu.db");
        // 初始化SOE
        DTU::SOEMap::instance()->init();

        DTU::DSTORE::instance()._address = DSYSCFG::instance().ASDU();

        // 启动模式(公共单元/间隔单元)
        auto DTUtype = DSYSCFG::instance().GetUnitType();
        int ASDUaddr = DSYSCFG::instance().ASDU();


        // 启动模式
        if (DTUtype == DSYSCFG::MODE_PUB) {
            DTULOG(DTU_INFO, (char *)"[公共单元模式]单元地址[%d]",ASDUaddr);
        } else if (DTUtype == DSYSCFG::MODE_BAY) {
            DTULOG(DTU_INFO, (char *)"[间隔单元模式]单元地址[%d]",ASDUaddr);
        } else {
            DTULOG(DTU_INFO, (char *)"配置文件sysconfig.json错误, 未知模式%u, 程序退出...", DTUtype);
            return 0;
        }

        ////////////////////////装置相关功能//////////////////////////
        // 启动DSP服务
        DTULOG(DTU_INFO, (char *)"启动DSP服务...");
        if (!DTU::DSPComm::GetInstance()->dsp_start()) {
            DTULOG(DTU_INFO, (char *)"启动DSP服务失败, 程序退出...");
            return 0;
        }
        // 初始化定值区号
        DTULOG(DTU_INFO,(char*)"初始化定值区信息...");
        /*
        从前置机读取公共定值，获取前置机器当前的定值区号,根据这个区号配置本地,
        1，确保前后定值区一致
        2，采用后台的定值区，避免再次下发定值
        */
        DTU::DSTORE::instance().change_default_setting_num(dsptask_execute_write);
        // 读取整定定值
        try
        {
            // 这个读取并解析adj 如果解析错误会抛出异常
            DTU::DSTORE::instance().write_adj_data(DTU::DSPComm::GetInstance()->init_adjust_param());
            // 如果定值读取并解析成功 生成xml文件
            DTU::DSTORE::instance().save_adj_data();
        }
        catch(const std::exception& e)
        {
            DTULOG(DTU_WARN, (char *)"%s,从文件整定定值",e.what());
            // 从文件中获取整定定值
            DTU::DSTORE::instance().load_adj_data();
        }
        
        ////////////////////////通信服务/////////////////////////////
        // 加载RPC静态通信变量
        // DTU::dtuRPCConnectMgr::instance().init(DTUtype);

        // LCD串口初始化
        DTULOG(DTU_INFO, (char *)"启动MCU串口...");
        auto LCDCfg = DSYSCFG::instance().GetLCDCFG();
        if (!DTU::dtuserial::instance().start_serial(LCDCfg.name, LCDCfg.baudrate)) {
            DTULOG(DTU_ERROR, (char *)"启动MCU串口失败, 程序退出...");
            return 0;
        }

        // 启用连接检测
        DTU::DTUConnectTest::instance()->init();

        // 启动101服务
        if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use) {
            DTULOG(DTU_INFO, (char *)"启动101服务...");
            DTU::D101Slave::instance().dtu101_init_slave();
            DTU::D101Slave::instance().dtu101_run_slave();
            D101SlaveSecond::instance().dtu101_init_slave();
            D101SlaveSecond::instance().dtu101_run_slave();
        }
        // 启动104服务
        if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use) {
            DTULOG(DTU_INFO, (char *)"启动104服务...");
            DTU::D104Server::instance().dtu104_init_server();
            DTU::D104Server::instance().dtu104_run_server();
        }

        // 自动校准时间(首项执行,校准系统时间)
        auto synccfg = DSYSCFG::instance().GetSyncCFG();
        if(synccfg.use)
        {
            DTULOG(DTU_INFO, (char *)"启动自动校准系统时间,时间间隔[%u]分钟...", synccfg.timeInSec);
            AutoTime::instance().start(synccfg.timeInSec);
        }

        // 初始化Goose服务
        DTU::dtuGooseService::instance().init();
        DTU::dtuGooseService::instance().run();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 配置规约文件系统
        DTU::DRULESFILE::instance().set_mode(DTUtype, ASDUaddr);

        // 定点文件生成 (放在校时后面)
        DTU::DRULESFILE::instance().start_fix_service();

        // PIC100线损模块
        auto FPIC100CFG = DSYSCFG::instance().GetLineCFG();
        if(FPIC100CFG.use)
        {
            DTU::D101Master::instance().dtu101_init_master(FPIC100CFG.proto.serial.name,FPIC100CFG.proto.serial.baudrate,1);
            DTU::D101Master::instance().dtu101_run_master();
            DRULESFILE::instance().start_frz_service();
        }

        // 启动配置工具服务
        DTULOG(DTU_INFO, (char *)"启动RPC服务...");
        if (!DTU::DTURPCMgr::instance().start_dtu_rpc(DSYSCFG::instance().GetRPCCFG().port)) {
            DTULOG(DTU_WARN, (char *)"启动公共单元RPC服务失败, 配置工具将无法使用...");
        }

        //===============
        // waitcommand();
        //===============

        // 停止连接检测
        DTU::DTUConnectTest::instance()->stop();
        // 关闭定点文件
        DTU::DRULESFILE::instance().stop_fix_service();
        // 关闭RPC
        DTULOG(DTU_INFO, (char *)"断开PRC连接...");
        DTU::DTURPCMgr::instance().stop_dtu_rpc();
        // 关闭串口
        DTULOG(DTU_INFO, (char *)"关闭MCU串口...");
        DTU::dtuserial::instance().stop_serial();
        // 关闭101协议(从站端)
        if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use) {
            DTULOG(DTU_INFO, (char *)"退出101服务...");
            DTU::D104Server::instance().dtu104_stop_server();
        } 
        // 关闭104协议(从站端)
        if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use) {
            DTULOG(DTU_INFO, (char *)"退出104服务...");
            DTU::D101Slave::instance().dtu101_stop_slave();
            D101SlaveSecond::instance().dtu101_stop_slave();
        }
        // 关闭Goose发送
        DTU::dtuGooseService::instance().stop();
        // 关闭PIC100线损模块
        if(FPIC100CFG.use)
        {
            DTULOG(DTU_INFO, (char *)"关闭PIC100线损模块...");
            DTU::D101Master::instance().dtu101_stop_master();
        }
        // 关闭校准时间
        if(synccfg.use)
        {
            DTULOG(DTU_INFO, (char *)"关闭校准时间...");
            AutoTime::instance().stop();
        }
        // 关闭DSP
        DTU::DSPComm::GetInstance()->dsp_stop();
    } 
    catch (std::exception &e)
    {
        DTULOG(DTU_ERROR, (char *)"系统发生未知错误:%s", e.what());
    }
    return 0;
}

void waitcommand() {
    std::string inputStr; 
    while(1)
    {
        std::cin >> inputStr;
        char fchar = inputStr.c_str()[0];
        if(fchar == 'q' || fchar == 'Q')
        {
            std::cout << "是否退出? Y/N:";
            std::cin >> fchar;
            if (fchar == 'y' || fchar == 'Y') {
                break;
            }
        }
        else if(inputStr.find("cmd-fix") != std::string::npos)
        {
            uint16_t findfix = 0;
            if(inputStr.size() > 7)
            {
                inputStr.erase(0,8);
                findfix = strtol(inputStr.c_str(), NULL, 10);
            }
            read_fix_table(findfix);
        }
        else if(inputStr.find("cmd-v") != std::string::npos)
        {
            ARMVersion version;
            printf("\nARM版本号:[%s]\nCMAKE时间:[%s]\n编译时间:[%s]\n",version.armversion.c_str(),
                                        version.cmaketime.c_str(),version.compiletime.c_str());
        }
        else if(inputStr.find("t") != std::string::npos)
        {
            // 测试RPC客户端是否连接
            // printf("TEST >>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            // bool test = DTU::dtuRPCConnectMgr::instance().ping();
            // if(test)
            // {
            //     printf("TEST YES\n");
            // }
            // else
            // {
            //     printf("TEST NO\n");
            // }
            // printf("TEST <<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        }
        else
        {
            //DTULOG(DTU_WARN, (char *)"未知的命令");
        }
    }
}

void create_directory() {
    // 创建保护目录
    create_dir_in_exec("/protect/comtrade/");
    create_dir_in_exec("/protect/factory/");
    create_dir_in_exec("/protect/protect/");
    create_dir_in_exec("/protect/selfcheck/");// 自检日志目录
    // 创建定点文件
    create_dir_in_exec("/HISTORY/FIXPT/");
    // 创建极值文件
    create_dir_in_exec("/HISTORY/EXV/");
    // SOE
    create_dir_in_exec("/HISTORY/SOE/");
    // 日志
    create_dir_in_exec("/HISTORY/ULOG/");
    // 遥控操作
    create_dir_in_exec("/HISTORY/CO/");
    //
    create_dir_in_exec("/FACTORY/");
    //
    create_dir_in_exec("/COMTRADE/");
    // 日冻结电能量
    create_dir_in_exec("/HISTORY/FRZ/");
    // 功率反向电能量冻结
    create_dir_in_exec("/HISTORY/FLOWREV/");
    // 升级文件夹
    create_dir_in_exec("/update/");
    create_dir_in_exec("/update/config/");
    create_dir_in_exec("/update/system/");
    create_dir_in_exec("/update/rulekit/"); // 规约传送文件位置
}
