/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_dsp.h
  *Description: 
    实现任务与DSP交互的执行流程
  *History: 
    1, 创建, wangjs, 2021-8-3
    2, 实现定值区切换任务, wangjs, 2021-8-16
    3, 加入读取当前定值区号任务, wangjs, 2021-8-16
    4, 加入软遥信，极值等的读取, wangjs, 2021-8-19
    5, 定值区号发送修正,lhy,2021-9-2
**********************************************************************************/
#include "dtutask_dsp.h"
#include <dtuerror.h>
#include <dtulog.h>
#include <dspcomm.h>
#include <dtucmdcode.h>
#include <dtuparamconfig.h>
#include <dturulesfile.h>
#include <dtustorage.h>
#include <dtunotifymanager.h>
#include <map>
#include <dtuversion.h>
#include <asm/ptrace.h>
#include <iostream>
#include <string>
#include <string.h>
#include <dturecorder.h>
#include <dtucmdcode.h>
#include <dtuversion.h>
#include <atomic>
#include <dtucommon.h>
#include <dtudbmanager.h>
#include <dtunetconfig.h>
#include <dtusystemconfig.h>
#include "dturpcmanager.h"
#include <dtusystemconfig.h>

using namespace DTU;
using namespace DTUCFG;

// 写参数
int dsptask_execute_write(uint16_t cmd, const DTU::buffer& data, uint16_t reboot)
{
    // 暂定，写参数只会返回成功或者不成功
    uint16_t errCode = DTU_SUCCESS;
    try
    {
        switch(cmd)
        {
            case PC_W_COMM_FIX: {
                if(data.size() != 256)    //通信定值长度
                {
                    errCode = DTU_DSP_ERROR; // 长度不一致直接返回
                    DTULOG(DTU_ERROR,"通信定值长度不一致,当前长度[%d],期望长度[%d]",data.size(),256);
                }
                else
                {
                    // 网络配置
                    errCode = DTUCFG::netconfigure::instance().set_net_param(data.get(0,sizeof(uint32_t)*3*4));
                }
            }break;
            case PC_W_FIX_AREA_INFO: {
                DTULOG(DTU_INFO, "执行写定值区操作");
                if (data.size() != 4) {
                    break;
                }
                uint16_t groupno = data.get(0, 4).value<uint32_t>();
                DBManager::instance().ChangeCurGroup(groupno, dsptask_execute_write);
                break;
            }
            default: {
                DTULOG(DTU_INFO,(char*)"执行写命令0x%04X, 数据长度:%u", cmd, data.size());
                uint16_t retCode = 0;
                buffer dspAck;
                if (DTU_SUCCESS != (DTU::DSPComm::GetInstance()->dsp_write_data(cmd, data, 50, reboot)))
                {
                    DTULOG(DTU_ERROR, (char*)"task_write_parameter DSP执行命令[0x%04x]失败", cmd);
                    retCode = TX_PC_NACK;
                    errCode = DTU_DSP_ERROR;
                }
            }break;
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"dsptask_execute_write unknow error:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return errCode;
}

// 读参数
int dsptask_execute_read(uint16_t cmd, DTU::buffer& result)
{
    // 暂定，发生错误时，返回的数据长度为0
    DTULOG(DTU_INFO,(char*)"执行读命令0x%04X", cmd);
    uint16_t retCode = DTU_SUCCESS;
    try
    {
        switch(cmd)
        {
            case PC_R_COMM: {
                result.remove();
                result.resize(256);
                int offset = 0;
                // 网络配置
                DTU::buffer buf = DTUCFG::netconfigure::instance().get_net_param();
                result.set(offset,buf);
                offset += buf.size();

                // 串口波特率配置
                offset += 24;

                // 101配置
                DTU::buffer cs101use((char*)&(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use),sizeof(uint8_t));
                result.set(offset,cs101use);
                offset += cs101use.size();
                DTU::buffer cs101mode((char*)&(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.mode),sizeof(uint8_t));
                result.set(offset,cs101mode);
                offset += cs101mode.size();

                offset += 10;

                // 104配置
                DTU::buffer cs104use((char*)&(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use),sizeof(uint32_t));
                result.set(offset,cs104use);
                offset += cs104use.size();
                uint32_t cip = IPToInt(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.ip);
                DTU::buffer cs104ip((char*)&cip,sizeof(uint32_t));
                result.set(offset,cs104ip);
                offset += cs104ip.size();
                DTU::buffer cs104port((char*)&(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.port),sizeof(uint32_t));
                result.set(offset,cs104port);
            };break;
            case PC_R_FIX_AREA_INFO: {
                result.remove();
                uint32_t currentGroup = DTU::DSTORE::instance().get_current_group();
                result.append((char*)&currentGroup,sizeof(currentGroup));
                break;
            }
            default: {
                if (DTU_SUCCESS != (DTU::DSPComm::GetInstance()->dsp_read_data(cmd, result,100)))
                {
                    DTULOG(DTU_ERROR, (char*)"dsptask_execute_read DSP执行命令[0x%04X]失败", cmd);
                    retCode = DTU_DSP_ERROR;
                }
            };break;
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"dsptask_execute_read unknow error:%s", e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return retCode;
}

int dsptask_execute_control(uint16_t cmd)
{
    int retCode = DTU_SUCCESS;
    retCode = DTU::DSPComm::GetInstance()->dsp_write_ctrl(cmd);
    if (DTU_SUCCESS != retCode)
    {
        DTULOG(DTU_ERROR, (char*)"dsptask_execute_control DSP执行命令[0x%04x]失败", cmd);
        retCode = DTU_DSP_ERROR;
    }

    switch(cmd)
    {
        case PC_W_GOOSE_COMM_RST :
        case PC_W_GOOSE_COMM_YC  : {
            DTU::dtuprotocol proto;
            proto._cmd = cmd;
            proto._data.append((char*)&cmd,sizeof(cmd));
            proto._data.append((char*)&retCode,sizeof(retCode));
            DTURPCMgr::instance().notify_goose_client(proto);
        };break;
            default:break;
    }

    return retCode;
}

int dsptask_execute_query(uint16_t cmd, DTU::buffer& result)
{
    // 暂定，发生错误时，返回的数据长度为0
    try
    {
        // 读取定值区号操作
        if (cmd == PC_R_FIX_AREA_INFO)
        {
            uint32_t curGroup = DTU::DSTORE::instance().get_current_group();
            result.append((char*)&curGroup, sizeof(curGroup));
            //dspAck.append((char*)&curGroup, sizeof(curGroup));
            //MakeRetAckMsg(proto, TX_PC_ACK, dspAck.size(), dspAck.size());
            //命令修改
            // MakeRetAckMsg(proto, TX_PC_FIX_AREA_INFO, dspAck.size(), dspAck.size());
            // proto._data = dspAck;
        }
        else if(cmd == PC_R_PRI_PRO_INFO)
        {/* 查询版本信息 */
            DTU::buffer dspAck;
            if (DTU_SUCCESS != (DTU::DSPComm::GetInstance()->dsp_read_data(PC_R_PRI_PRO_INFO, dspAck, 100)))
            {
                DTULOG(DTU_ERROR, (char*)"dsptask_execute_query DSP执行命令[0x%04x]失败", cmd);
                return DTU_DSP_ERROR;
            }
            // 覆盖860版本信息
            std::vector<char> CK860v = {STAGE_VERSION,INNER_VERSION,SUB_VERSION,MAIN_VERSION};
            uint32_t offset = 8;

            for(auto item:CK860v)
            {
                dspAck.set(offset,(char*)&item,sizeof(uint8_t));
                offset++;
            }
            result = dspAck;
        }
        else
        {/* 查询其他 */
            //uint16_t retCode = get_ack(cmd);
            if (DTU_SUCCESS != (DTU::DSPComm::GetInstance()->dsp_read_data(cmd, result,100)))
            {
                DTULOG(DTU_ERROR, (char*)"dsptask_execute_query DSP执行命令[0x%04x]失败", cmd);
                //MakeRetAckMsg(proto, TX_PC_NACK);
                return DTU_DSP_ERROR;
            }
            else
            {
                // MakeRetAckMsg(proto, retCode, dspAck.size(), dspAck.size());
                // proto._data = dspAck;
            }
        }
    }
    catch(std::exception& e)
    {
        //dump_stack();
        DTULOG(DTU_ERROR,(char*)"dsptask_execute_query unknow error:%s", e.what());
        //MakeRetAckMsg(proto, TX_PC_NACK);
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_SUCCESS;
}

// 遥控操作类
class RmctrlClass{
public:
    static RmctrlClass *instance()
    {
        static RmctrlClass ins;
        return &ins;
    }

    int start(uint16_t fix, uint16_t operate, RemoteCtrlInfo info)
    {
        try
        {
            int ret = DTU_DSP_ERROR; // 函数返回值
            // 查找fix
            auto fret = rmctrlMap.find(transfixToStrint(fix));
            if(fret == rmctrlMap.end())
            {
                DTULOG(DTU_ERROR,"遥控操作错误的点表号[0x%04X]",fix);
                return DTU_DSP_ERROR;
            }
            // 检查本次预设源
            auto fromret = CMD_FROM.find(info.cmdFrom);
            if(fromret == CMD_FROM.end())
            {
                DTULOG(DTU_ERROR,"遥控操作错误的预设源");
                return DTU_DSP_ERROR;
            }
            std::string fromstr = (*fromret).second;
            // 检查上次预设源
            bool his_ispre = std::get<5>((*fret).second);
            std::string his_prestr;
            if(his_ispre)
            {
                auto his_from = std::get<6>((*fret).second);
                his_prestr = CMD_FROM[his_from];
            }
            switch(operate)
            {
                // 预设
                case RC_CMD_PRE: {
                    // 检查是否已经预设
                    if(his_ispre)
                    {
                        // 已经预设 返回
                        DTULOG(DTU_WARN,"遥控点表[0x%04X]已预设,请勿重复预设,上次预设源:%s",fix,his_prestr.c_str());
                        return DTU_DSP_ERROR;
                    }
                    // 执行预设命令
                    ret = Pushcmd(std::get<0>((*fret).second));
                    // 重置标志位为true 
                    setflag(fix,true,info.cmdFrom,info.conn);
                    DTULOG(DTU_INFO,"遥控点表[0x%04X]预设,预设源:%s, 预设源设备地址[0x%08X] 成功标志[%s]",
                                                                fix,CMD_FROM[info.cmdFrom].c_str(), info.conn, (ret == DTU_SUCCESS) ? "T" : "F");
                    // 延时执行
                    std::thread t([&, info, fix]() {
                        auto tfret = rmctrlMap.find(transfixToStrint(fix));
                        std::this_thread::sleep_for(std::chrono::seconds(info.delay));
                        // 时间到如果未置位则下发取消
                        bool isExec = false;
                        {
                            std::lock_guard<std::mutex> LOCK(_lock); // 加{}限制作用域
                            isExec = std::get<7>((*tfret).second);
                        }
                        if (isExec)
                        {
                            Pushcmd(std::get<2>((*tfret).second));
                            // 将预设状态清除
                            std::get<5>((*tfret).second) = false;
                            DTULOG(DTU_WARN,"遥控点表[0x%04X]超时取消",fix);
                        }
                    });
                    t.detach();
                };break;
                // 执行
                case RC_CMD_EXE: {
                    // 检查是否预设
                    if(!his_ispre)
                    {
                        DTULOG(DTU_ERROR,"遥控点表[0x%04X]未预设",fix);// 未预设
                        return DTU_DSP_ERROR;
                    }
                    // 检查是否同源
                    if(std::get<6>((*fret).second) != info.cmdFrom)
                    {
                        DTULOG(DTU_ERROR,"遥控非同源操作,预设操作源[%s],本次操作源[%s]",
                            his_prestr.c_str(),CMD_FROM[info.cmdFrom].c_str());// 不同源
                            return DTU_DSP_ERROR;
                    }
                    // 检查是否来源于同一地址
                    if(std::get<8>((*fret).second) != info.conn)
                    {
                        DTULOG(DTU_ERROR,"遥控非共同地址操作,预设操作源地址[0x%08X],本次操作源[0x%08X]",
                            std::get<8>((*fret).second), info.conn);// 不同源地址
                            return DTU_DSP_ERROR;
                    }
                    // 执行执行命令
                    DTULOG(DTU_INFO,"遥控点表[0x%04X]执行,预设源:%s",fix,CMD_FROM[info.cmdFrom].c_str());
                    setflag(fix); // 置标志位 取消自动下发撤销命令
                    ret = Pushcmd(std::get<1>((*fret).second));
                };break;
                // 取消
                case RC_CMD_CAN: {
                    // 检查是否预设
                    if(!his_ispre)
                    {
                        DTULOG(DTU_ERROR,"遥控点表[0x%04X]未预设",fix);// 未预设
                        return DTU_DSP_ERROR;
                    }
                    // 检查是否同源
                    if(std::get<6>((*fret).second) != info.cmdFrom)
                    {
                        DTULOG(DTU_ERROR,"遥控非同源操作,预设操作源[%s],本次操作源[%s]",
                            his_prestr.c_str(),CMD_FROM[info.cmdFrom].c_str());// 不同源
                            return DTU_DSP_ERROR;
                    }
                    // 检查是否为同一主站操作
                    if(std::get<8>((*fret).second) != info.conn)
                    {
                        DTULOG(DTU_ERROR,"遥控非同源操作,预设操作源[0x%08X],本次操作源[0x%08X]",
                            std::get<8>((*fret).second), info.conn);// 不同源
                            return DTU_DSP_ERROR;
                    }
                    // 执行取消命令
                    DTULOG(DTU_INFO,"遥控点表[0x%04X]撤销,预设源:%s",fix,CMD_FROM[info.cmdFrom].c_str());
                    setflag(fix); // 置标志位 取消自动下发撤销命令
                    ret = Pushcmd(std::get<2>((*fret).second));
                };break;
                default:
                    DTULOG(DTU_ERROR, "错误的操作符[%02d]", operate);
            }
            return ret;
        }
        catch(const std::exception& e)
        {
            DTULOG(DTU_ERROR,"遥控发生未知错误%s",e.what());
            return DTU_DSP_ERROR;
        }

        return DTU_DSP_ERROR;
    }
    // 取消所有预设 (用于规约断开时立即取消预设)
    void cancelAll()
    {
        for(auto &item : rmctrlMap)
        {
            // 是否已经预设
            if(std::get<5>(item.second)) {
                auto from = std::get<6>(item.second);
                if((from == RC_CMD_101) || (from == RC_CMD_104)) {
                    // 已经预设
                    setflag(std::get<3>(item.second)); // 置标志位 取消自动下发撤销命令
                    Pushcmd(std::get<2>(item.second));
                }
            }
        }
    }
private:
    void setflag(uint16_t fix,bool pre = false, int from = -1, int devno = 0)
    {
        auto fret = rmctrlMap.find(transfixToStrint(fix));
        if(fret == rmctrlMap.end())
            return;
        std::lock_guard<std::mutex> LOCK(_lock);
        std::get<5>((*fret).second) = pre;      // 重置预设状态
        std::get<6>((*fret).second) = from;     // 重置预设源
        std::get<7>((*fret).second) = pre;      // 重置执行状态
        std::get<8>((*fret).second) = devno;    // 重置预置地址
    }
    std::string transfixToStrint(uint16_t fix)
    {
        for(auto item:rmctrlMap)
        {
            if(std::get<3>(item.second) == fix)
            {
                return item.first;
            }
        }
        DTULOG(DTU_ERROR,"遥控查找错误");
        return std::string();
    }
    int Pushcmd(uint16_t cmd)
    {
        if(cmd == PC_W_NO_CMD)
            return DTU_SUCCESS;
        try
        {
            if (DTU_SUCCESS != dsptask_execute_control(cmd))
            {
                return DTU_DSP_ERROR;
            }
        }
        catch(const std::exception& e)
        {
            DTULOG(DTU_ERROR,(char*)"RmctrlClass 未知错误:%s", e.what());
            return DTU_UNKNOWN_ERROR;
        }
        return DTU_SUCCESS;
    }
private:
    // 构造函数
    RmctrlClass()
    {
        auto rmtable = DBManager::instance().GetRmctrlTable();
        for(auto item:rmtable)
        {
            auto ret = rmctrlMap.find(item.second.desc);
            if(ret != rmctrlMap.end())
            {
                std::get<3>((*ret).second) = item.second.fixid;  // 放入点表
                std::get<4>((*ret).second) = item.second.needPre;// 放入配置
            }
        }
    };
    ~RmctrlClass(){};
    private:
    // PC_W_NO_CMD表示不执行动作
    // tuple: 0预设命令 1执行命令 2取消命令 3点表值(来自配置文件) 4是否需要预设(来自配置文件)  5是否预设 6预设源 7是否执行(原子操作) 8预设设备地址
    std::map<std::string,std::tuple<uint16_t,uint16_t,uint16_t,uint16_t,bool,bool,int,bool,int>> rmctrlMap = {
        {std::string("电池活化启动"),{PC_W_NO_CMD,PC_W_YK_HH_QD,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("电池活化退出"),{PC_W_NO_CMD,PC_W_YK_HH_TC,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("遥控分闸(就地)"),{PC_W_YK_SET,PC_W_YK_FZ_JD,PC_W_YK_CANCLE,0x0000,false,false,-1,false,0}},
        {std::string("遥控合闸(就地)"),{PC_W_YK_SET,PC_W_YK_HZ_JD,PC_W_YK_CANCLE,0x0000,false,false,-1,false,0}},
        {std::string("保护复归(就地)"),{PC_W_NO_CMD,PC_W_RST_SIG_JD,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("遥控分闸(远方)"),{PC_W_YK_SET,PC_W_YK_FZ_YF,PC_W_YK_CANCLE,0x0000,false,false,-1,false,0}},
        {std::string("遥控合闸(远方)"),{PC_W_YK_SET,PC_W_YK_HZ_YF,PC_W_YK_CANCLE,0x0000,false,false,-1,false,0}},
        {std::string("保护复归(远方)"),{PC_W_NO_CMD,PC_W_RST_SIG_YF,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("测试继电器"),{PC_W_NO_CMD,PC_W_TEST_RELAY,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("测试LED"),{PC_W_NO_CMD,PC_W_TEST_LED,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("手动录波"),{PC_W_NO_CMD,PC_W_TEST_LB,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("闭锁复归(就地)"),{PC_W_NO_CMD,PC_W_BLK_RST,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("开关传动软压板投入"),{PC_W_NO_CMD,PC_W_POWER_DRIVER_SYB_ON,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
        {std::string("开关传动"),{PC_W_POWER_DRIVER_PRE,PC_W_POWER_DRIVER,PC_W_NO_CMD,0x0000,false,false,-1,false,0}},
    };
    std::map<uint16_t,std::string> CMD_FROM = {
        {RC_CMD_101,"101规约"},
        {RC_CMD_104,"104规约"},
        {RC_CMD_TOOL,"TOOL工具"},
        {RC_CMD_LCD,"LCD屏幕"},
    };
    std::mutex _lock;
};

int dsptask_execute_rmctrl(uint16_t fix,uint16_t operate,RemoteCtrlInfo info)
{
    auto ret = RmctrlClass::instance()->start(fix,operate,info);
    // 为了添加CO操作记录,所以将点表映射回外部点表
    fix = DTU::DBManager::instance().GetRMCMapFixidByinID(fix);
    if(ret != DTU_SUCCESS)
        DTU::DRULESFILE::instance().add_co_file(fix,operate,1);// 失败
    else
        DTU::DRULESFILE::instance().add_co_file(fix,operate,0);// 成功
    return ret;
}

void dsptask_execute_cancelrmc()
{
    // 取消所有预设
    RmctrlClass::instance()->cancelAll();
    DTULOG(DTU_INFO,"取消所有预设");
}