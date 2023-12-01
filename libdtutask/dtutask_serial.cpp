/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutask_serial.cpp
  *Description: 
    实现任务与串口交互的执行流程
  *History: 
    1, 创建, wangjs, 2021-8-3
    2, 加入信息查看功能, wangjs, 2021-8-13
    2, 加入定值切换功能, wangjs, 2021-8-16
**********************************************************************************/
#include "dtutask_serial.h"
#include "dtutask_dsp.h"
#include <dtuerror.h>
#include <dtucmdcode.h>
#include <vector>
#include <functional>
#include <dtulog.h>

#include <dtustorage.h>
#include <dtunetconfig.h>
#include <dtuparamconfig.h>
#include <dtudbmanager.h>

using namespace DTU;
using namespace std;


using TASKCALLBACK =  
    std::vector<std::tuple<std::vector<uint16_t>, std::function<int(dtuprotocol& proto, dtubasecomm* pIO)>>>;

// 这里只有读定值和查询事项的放回值
static std::unordered_map<uint16_t, uint16_t> gAck = {
    {PC_R_INT_FIX, TX_PC_INT_FIX},{PC_R_ADJ_LCD_FIX,TX_PC_ADJ_LCD_FIX},
    {PC_R_ADJ_FIX, TX_PC_ADJ_FIX},{PC_R_PUB_FIX, TX_PC_PUB_FIX},
    {PC_R_PRO_FIX,TX_PC_PRO_FIX},
    {PC_R_YB_STATE_INFO, TX_PC_SOFT_YB_FIX},{PC_R_LOG_DATA, TX_PC_LOG_DATA},
    {PC_R_FA_FIX,TX_PC_FA_FIX},
    {PC_R_DFA_FIX,TX_PC_DFA_FIX},
    {PC_R_TQHZ_FIX,TX_PC_TQHZ_FIX},{PC_R_AUTO_SPLIT_FIX,TX_PC_AUTOSPLIT_FIX},
    {PC_R_CLK, TX_PC_CLK},{PC_R_YC_DATA, TX_PC_YC_DATA},
    {PC_R_HYX_DATA, TX_PC_YX_DATA},{PC_R_CHECK, TX_PC_SELF_DATA},
    {PC_R_PROFUN_STATE, TX_PC_PROFUN_STATE},{PC_R_SOFT_PROG,TX_PC_PRV_DATA},
    {PC_R_MAIN_MENU_INFO, TX_PC_MAIN_MENU_INFO},{PC_R_ALARM_INFO,TX_PC_ALARM_REP_INFO},
    {PC_R_SYX_INFO, TX_PC_SYX_INFO},{PC_R_COS_DATA, TX_PC_COS_DATA},
    {PC_R_ZTLB_DATA, TX_PC_ZTLB_DATA},{PC_R_EXV_DATA, TX_PC_EXV_DATA},
    {PC_R_FIX_AREA_INFO,TX_PC_FIX_AREA_INFO},{PC_R_PRI_PRO_INFO,TX_PC_PRI_PRO_INFO},
    {PC_R_AUTORECLOSE_FIX,TX_PC_AUTORECLOSE_FIX},{PC_R_LINEBRKALARM_FIX,TX_PC_LINEBRKALARM_FIX},
    {PC_R_POWERDRIVER_FIX,TX_PC_POWERDRIVER_FIX},{PC_R_AUTOCFG_FIX,TX_PC_AUTOCFG_FIX},
    {PC_R_XDLGND_FIX,TX_PC_XDLGND_FIX},{PC_R_COMM,TX_PC_COMM_FIX}, {PC_R_XY, TX_PC_XY},
};

inline uint16_t get_ack(uint16_t cmd){
    auto ita = gAck.find(cmd);
    if (ita != gAck.end())
    {
        return ita->second;
    }
    return TX_PC_NACK;
}

inline void MakeRetAckMsg(dtuprotocol& proto, uint16_t ackCmd,
    uint32_t curSize = 0, uint32_t totleLen = 0){
    proto._header = 0xbb66;
    proto._cmd = ackCmd;
    proto._curLen = curSize;
    proto._totleLen = totleLen;
}

// 写参数
static int serialtask_execute_write(dtuprotocol& proto, dtubasecomm* pIO);
// 读参数
static int serialtask_execute_read(dtuprotocol& proto, dtubasecomm* pIO);
// 读取信息
static int serialtask_execute_query(dtuprotocol& proto, dtubasecomm* pIO);
// 控制
static int serialtask_execute_control(dtuprotocol& proto, dtubasecomm* pIO);
// 网络参数
static int serialtask_execute_IPquery(dtuprotocol& proto, dtubasecomm* pIO);
// 读取保护动作报告、SOE， 告警， 操作记录
static int serialtask_execute_ReadRSAO(dtuprotocol& proto, dtubasecomm* pIO);

// 执行任务
int execute_serial_task(DTU::dtuprotocol& proto, DTU::dtubasecomm* pIO)
{
    static TASKCALLBACK callbacks = {
            // 写参数，需要存定值文件
            {{PC_W_PUB_FIX, PC_W_YB_ON_OFF_INFO, PC_W_PRO_FIX, PC_W_AUTORECLOSE_FIX, PC_W_FA_FIX, 
              PC_W_DFA_FIX, PC_W_TQHZ_FIX, PC_W_AUTO_SPLIT_FIX, PC_W_XDLGND_FIX,
              PC_W_LINEBRKALARM_FIX, PC_W_POWERDRIVER_FIX, PC_W_AUTOCFG_FIX, PC_W_CLK, 
            },serialtask_execute_write},
            // 读参数，更新定值文件
            {{PC_R_ADJ_LCD_FIX, PC_R_PUB_FIX, PC_R_INT_FIX, PC_R_PRO_FIX, PC_R_AUTORECLOSE_FIX, 
              PC_R_FA_FIX, PC_R_DFA_FIX, PC_R_TQHZ_FIX, PC_R_AUTO_SPLIT_FIX, PC_R_XDLGND_FIX,
              PC_R_LINEBRKALARM_FIX, PC_R_POWERDRIVER_FIX, PC_R_AUTOCFG_FIX,
              PC_R_YB_STATE_INFO,PC_R_COMM},serialtask_execute_read},
            // 信息显示
            {{PC_R_CLK, PC_R_YC_DATA, PC_R_HYX_DATA, PC_R_CHECK, PC_R_PROFUN_STATE, PC_R_SOFT_PROG,
            PC_R_MAIN_MENU_INFO, PC_R_FIX_AREA_INFO,PC_R_PRI_PRO_INFO},serialtask_execute_query},
            // 控制                                              // 测试 LED 继电器 手动录波
            {{PC_W_FIX_AREA_NUM,PC_W_RST_SIG_JD,PC_W_TEST_FORMAT,PC_W_TEST_LED,PC_W_TEST_RELAY,PC_W_TEST_LB,
              PC_W_BLK_RST, PC_W_SETTING_COPY}, serialtask_execute_control},
            // 通讯定值
            //{{PC_R_COMM/*查询通讯定值*/,PC_W_COMM_FIX},serialtask_execute_IPquery},
            //读取保护动作报告、SOE， 告警， 操作记录
            {{PC_R_PRO_ACT_INFO,PC_R_SOE_INFO,PC_R_ALARM_INFO,PC_R_OPER_INFO},serialtask_execute_ReadRSAO}
    };
    for(auto& item1 : callbacks)
	{
		for(auto& cmditem : std::get<0>(item1))
		{
			if (cmditem == proto._cmd)
			{
				return std::get<1>(item1)(proto, pIO);
			}
		}
	}
    return DTU_UNKNOWN_ERROR;
}
//
int serialtask_execute_write(dtuprotocol& proto, dtubasecomm* pIO)
{
    int retCode = dsptask_execute_write(proto._cmd, proto._data);
    uint16_t ackCode = TX_PC_NACK;
    if (retCode == DTU_SUCCESS){
        ackCode = TX_PC_ACK;
    }
    // 回复串口
    if (pIO){
        //header cmd totleLen curLen blockno dataon=0xFFFF
        DTULOG(DTU_INFO, (char*)"发送命令 0x%04X 写响应 0x%04X", proto._cmd, ackCode);
        dtuprotocol ack(0xbb66, ackCode, 0, 0, 0xffff);
        if (pIO->SendData((ack.package()))<=0)
        {
            DTULOG(DTU_INFO, (char*)"serialtask_execute_write 发送命令[0x%04X]应答数据失败", proto._cmd);
            return DTU_SERIAL_ERROR;
        }
    }
    return DTU_SUCCESS;
}

int serialtask_execute_read(dtuprotocol& proto, dtubasecomm* pIO)
{
    auto ret = dsptask_execute_read(proto._cmd, proto._data);
    if (ret != DTU_SUCCESS){
        MakeRetAckMsg(proto, TX_PC_NACK, 0, 0);
    }
    else {
        MakeRetAckMsg(proto, get_ack(proto._cmd), proto._data.size(), proto._data.size());
    }

    if (pIO)
    {
        if (pIO->SendData(proto.package())<=0)
        {
            DTULOG(DTU_ERROR, (char*)"serialtask_execute_read 发送命令[0x%04X]应答数据失败", proto._cmd);
            return DTU_SERIAL_ERROR;
        }
    }
    return ret;
}

int serialtask_execute_query(dtuprotocol& proto, dtubasecomm* pIO)
{
    uint16_t cmd = proto._cmd;
    auto ret = dsptask_execute_query(proto._cmd, proto._data);
    if (ret != DTU_SUCCESS){
        MakeRetAckMsg(proto, TX_PC_NACK, 0, 0);
    }
    else{
        MakeRetAckMsg(proto, get_ack(proto._cmd), proto._data.size(), proto._data.size());
    }
    
    if (pIO)
    {
        if (pIO->SendData(proto.package())<=0)
        {
            DTULOG(DTU_ERROR, (char*)"serialtask_execute_query 发送命令[0x%04X]应答数据失败", cmd);
            return DTU_SERIAL_ERROR;
        }
    }
    return ret;
}

int serialtask_execute_control(dtuprotocol& proto, dtubasecomm* pIO)
{
    uint16_t cmd = proto._cmd;
    int ret = DTU_SUCCESS;
    // 切换定值区命令单独处理
    if(cmd == PC_W_FIX_AREA_NUM)
    {
        int group = proto._data.get(sizeof(int),sizeof(int)).value<int>();
        try
        {
            if (DTU::DSTORE::instance().change_setting_num(group,dsptask_execute_write))
                ret = DTU_SUCCESS;
            else
                ret = DTU_DSP_ERROR;
        }
        catch(std::exception& e)
        {
            DTULOG(DTU_ERROR,(char*)"rpc_change_group 执行失败:%s", e.what());
            ret = DTU_DSP_ERROR;
        }
    }
    // 定值区复制命令单独处理
    else if(cmd == PC_W_SETTING_COPY)
    {
        if(proto._data.size() == sizeof(uint32_t)*2) {
            uint32_t srcgroup = proto._data.get(0,sizeof(uint32_t)).value<uint32_t>();
            uint32_t destgroup = proto._data.get(4,sizeof(uint32_t)).value<uint32_t>();
            DTULOG(DTU_INFO,"正在将定值区[%u]复制到[%u]",srcgroup,destgroup);
            if(DTU::DSTORE::instance().setting_copy(srcgroup,destgroup))
                ret = DTU_SUCCESS;
            else
            {
                DTULOG(DTU_ERROR,"定值区拷贝失败");
                ret = DTU_DSP_ERROR;
            }
        }
        else {
            ret = DTU_SERIAL_ERROR;
        }
    }
    else
        auto ret = dsptask_execute_control(cmd);
    // 切换
    if (ret == DTU_SUCCESS)
        MakeRetAckMsg(proto, TX_PC_ACK, 0, 0);
    else
        MakeRetAckMsg(proto, TX_PC_NACK, 0, 0);

    // 要加延时，不然LCD收不到应答信号
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (pIO)
    {
        DTULOG(DTU_INFO, (char*)"发送命令 0x%04X 写响应 0x%04X", cmd, ret);
        if (pIO->SendData((proto.package()))<=0)
        {
            DTULOG(DTU_INFO, (char*)"serialtask_execute_control 发送命令[0x%04X]应答数据失败", cmd);
            return DTU_SERIAL_ERROR;
        }
    }
    return DTU_SUCCESS;
}

//网络参数配置 IP NETMASK NETGATE
static int serialtask_execute_IPquery(dtuprotocol& proto, dtubasecomm* pIO)
{
    DTU::dtuprotocol ret;
    ret._header = 0xbb66;
    if(proto._cmd == PC_R_COMM)
    {
        ret._data = DTUCFG::netconfigure::instance().get_net_param();
        if( ret._data.size() <= 0 )
        {
            DTULOG(DTU_ERROR, (char*)"网络参数读取失败！");
            return DTU_SERIAL_ERROR;
        }
        ret._curLen = ret._data.size();
        ret._totleLen = ret._data.size();
        ret._blockno = 0xFFFF;
    }
    else if(proto._cmd == PC_W_COMM_FIX)
    {        
        // 重新设置网络配置
        int nret = DTUCFG::netconfigure::instance().set_net_param(proto._data);
        if( nret != DTU_SUCCESS )
        {
            DTULOG(DTU_ERROR, (char*)"serialtask_execute_IPquery 设置网络参数错误");
            return nret;
        }
    }

    static std::map<uint16_t,uint16_t> map_ret = {
        {PC_R_COMM,TX_PC_COMM_FIX},
        {PC_W_COMM_FIX,TX_PC_ACK}
    };
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if(pIO)
    {
        auto Rfind = map_ret.find(proto._cmd);
        if(Rfind != map_ret.end())
        {
            DTULOG(DTU_INFO, (char*)"命令 0x%04X 写响应 0x%04X", proto._cmd, Rfind->second);
            ret._cmd = Rfind->second;
            if (pIO->SendData((ret.package()))<=0)
            {
                DTULOG(DTU_INFO, (char*)"serialtask_execute_IPquery 发送命令[0x%04X]应答数据失败", Rfind->second);
                return DTU_SERIAL_ERROR;
            }
        }
        else
        {
            return DTU_SERIAL_ERROR;
        }
    }
    return DTU_SUCCESS;
}//serialtask_execute_IPquery

// 命令  读取ID  返回值
static map<uint16_t,tuple<uint16_t, uint16_t>> RSAO_Report = {
    {PC_R_PRO_ACT_INFO,{ReportProSimple,TX_PC_PRO_ACT_INFO_DATA}},  // 保护动作报告
    {PC_R_ALARM_INFO,{ReportWAR,TX_PC_ALARM_REP_INFO}},             // 告警
    {PC_R_OPER_INFO,{ReportOPT,TX_PC_OPER_REP_DATA}},               // 操作记录
    {PC_R_SOE_INFO,{ReportSOE,TX_PC_SOE_INFO}}                      // 读取SOE
};

// 读取动作报告 、SOE、 告警、 操作记录
int serialtask_execute_ReadRSAO(dtuprotocol& proto, dtubasecomm* pIO)
{
    auto iter = RSAO_Report.find(proto._cmd);
    if(iter != RSAO_Report.end())
    {
        //所需要字符的序号 0最新一条 其他按序号获取
        uint64_t McuNeedNo = proto._data.get(0,sizeof(uint64_t)).value<uint64_t>();
        // 数据库中最新一条报告序号
        uint64_t DBCurNo = DSTORE::instance().get_cur_report_no(std::get<0>(iter->second));
        uint64_t OperateNo = 0;
        DTU::buffer retReport;
        if(DBCurNo == 0) {
            DTULOG(DTU_WARN, (char*)"当前没有记录");
            // 放入空 防止LCD不刷屏
            retReport.resize(DSTORE::instance().get_report_itemsize(std::get<0>(iter->second)));
        }
        else
        {
            if(McuNeedNo == 0) {
                // MCU索要第0条 返回最新的一条
                OperateNo = DBCurNo;
            }
            else if(McuNeedNo > DBCurNo) {
                // MCU索要大于最大 返回第一条
                OperateNo = 1;
            }
            else {
                // 其他情况直接按MCU索要的序号查询
                OperateNo = McuNeedNo;
            }
            retReport = DSTORE::instance().get_report_by_serial(std::get<0>(iter->second), OperateNo);
        }

        if(retReport.size() > 0)
        {
            if (pIO)
            {
                dtuprotocol ack(0xbb66, std::get<1>(iter->second), retReport.size(), retReport.size(), 0xFFFF);

                // 报告前需要发送64位当前序号
                ack._data.append((char*)&OperateNo, sizeof(OperateNo));
                ack._data.append(retReport);

                DTULOG(DTU_INFO, (char*)"命令 0x%04X 写响应 0x%04X", proto._cmd, std::get<1>(iter->second));
                if (pIO->SendData((ack.package()))<=0)
                {
                    DTULOG(DTU_INFO, (char*)"serialtask_execute_ReadReport 发送命令[0x%04X]应答数据失败", proto._cmd);
                    return DTU_SERIAL_ERROR;
                }
            }
        }
        else
        {
            DTULOG(DTU_WARN, (char*)"保护报告数据为空");
        }
        return DTU_SUCCESS;
    }
    else
    {
        DTULOG(DTU_ERROR, (char*)"命令错误");
        return DTU_SERIAL_ERROR;
    }
}
