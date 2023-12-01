#include "dtuconnect.h"
#include "dtulog.h"
#include "dtusystemconfig.h"

#include <rest_rpc/rest_rpc.hpp>

#include "dspcomm.h"
#include "dtudatastruct.h"
#include "dtusoemap.h"
#include "dtunotifymanager.h"
#include "dtucommon.h"

using namespace DTU;
using namespace DTUCFG;

// 

void DTUConnectTest::init()
{
    isPublic = DSYSCFG::instance().isPublic();

    is_use_101 = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use;
    is_use_104 = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use;

    running = true;

    runThread = std::make_unique<std::thread>(&DTUConnectTest::run, this);
}

void DTUConnectTest::set_cs101_state(bool state)
{
    if (state) {
        DTULOG(DTU_INFO, "CS101规约已连接到主站");
    }
    else {
        DTULOG(DTU_INFO, "CS101规约已连从主站断开");
    }

    cs101_state = state;
}

void DTUConnectTest::set_cs104_state(bool state)
{
    if (state) {
        DTULOG(DTU_INFO, "CS104规约已连接到主站");
    }
    else {
        DTULOG(DTU_INFO, "CS104规约已连从主站断开");
    }

    cs104_state = state;
}

// 第一个为硬件设备地址 第二为HIOA
static std::map<CA, std::tuple<uint16_t, HIOA>> cimap = {
    {1, {0x0108, 0x006E}},
    {2, {0x0109, 0x006F}},
    {3, {0x010A, 0x0070}},
    {4, {0x010B, 0x0071}},
    {5, {0x010C, 0x0072}},
    {6, {0x010D, 0x0073}},
};

static std::map<CA, bool> curstatemap = {
    {1, false},
    {2, false},
    {3, false},
    {4, false},
    {5, false},
    {6, false},
};

void DTUConnectTest::run()
{
    DTULOG(DTU_INFO, "连接检测已开启");

    // 下发关灯命令
    DSPComm::GetInstance()->dsp_write_ctrl(PC_W_LED_TX_LINK_DOWN);

    for (;running;)
    {
        bool result = true;
        if (isPublic) {
            // 公共单元,检测对主站通信
            if (is_use_101) {
                result = (result && cs101_state);
            }

            if (is_use_104) {
                result = (result && cs104_state);
            }

            // 自检 与主站通信情况
            selfcheck.set(0, !result);

            // 与间隔单元通信情况
            for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
            {
                if (item.second.use) {
                    rest_rpc::rpc_client client(item.second.ProtoRPC.ip, item.second.ProtoRPC.port);
                    client.set_connect_timeout(100);
                    bool ret = client.connect();
                    selfcheck.set(4 + item.second.ca - 1, !ret);

                    // 查找CA是否存在
                    auto ita = cimap.find(item.second.ca);
                    if (ita != cimap.end()) {
                        // 更新软遥信
                        SOEMap::instance()->updateYXStateByHIOA(std::get<1>(ita->second), !ret);
                        // 上送软遥信(true为无错误)
                        this->updateSOE(std::get<0>(ita->second), item.second.ca, !ret);
                    }
                }
            }

        }
        else {
            // 间隔单元,检测对公共单元通信
            auto &publiccfg =  DSYSCFG::instance().GetPublicCFG().ProtoRPC;
            rest_rpc::rpc_client client(publiccfg.ip, publiccfg.port);
            client.set_connect_timeout(50);
            result = client.connect();

            // 与公共单元通信情况
            selfcheck.set(3, !result);
        }
        
        // 更新自检标志
        DSPComm::GetInstance()->update_connect(selfcheck.to_ulong());

        if (result != lastState) {
            // 状态发生了变化
            // 下发到DSP 注意这里需要等待DSP开启后再进行
            if (result) {
                for (int i=0; i<5; i++)
                {
                    if (DTU_SUCCESS != DSPComm::GetInstance()->dsp_write_ctrl(PC_W_LED_TX_LINK_UP))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else {
                for (int i=0; i<5; i++)
                {
                    if (DTU_SUCCESS != DSPComm::GetInstance()->dsp_write_ctrl(PC_W_LED_TX_LINK_DOWN))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                    else
                    {
                        break;
                    }
                }
            }
            lastState = result;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    DTULOG(DTU_INFO, "连接检测已退出");
}

void DTUConnectTest::stop()
{
    running = false;
    if (this->runThread)
        this->runThread->join();
}

void DTUConnectTest::updateSOE(uint16_t devaddr, int ca, bool state)
{
    if (curstatemap[ca] == state) {
        return;
    }
    else {
        curstatemap[ca] = state;
    }

    DTU::buffer SOEBuffer;
    // 20字节
    SOEBuffer.resize(20);
    int offset = 0;

    auto curtime = GetSystemTime();

    // 时间(s)
    uint32_t time_s = (uint32_t)(curtime / (uint32_t)1000000) - (uint32_t)946656000;// - (uint32_t)28800;
    SOEBuffer.set(offset, (char*)&time_s, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    // 时间(us)
    uint32_t time_us = (uint32_t)(curtime % 1000000);
    SOEBuffer.set(offset, (char*)&time_us, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    // 时间(闰秒标志)
    uint16_t time_r = 0;
    SOEBuffer.set(offset, (char*)&time_r, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    // 类别
    uint16_t type = 1;// 软遥信
    SOEBuffer.set(offset, (char*)&type, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    // 识别码
    uint16_t flagno = devaddr;
    SOEBuffer.set(offset, (char*)&flagno, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    uint8_t state_before = 0;
    uint8_t state_after = 1;

    // 状态变化
    if (state) {
        // 产生错误 0 -> 1
        state_before = 0;
        state_after = 1;
    }
    else {
        // 消除错误 1 -> 0
        state_before = 1;
        state_after = 0;
    }  

    SOEBuffer.set(offset, (char*)&state_before, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    SOEBuffer.set(offset, (char*)&state_after, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    DSPComm::GetInstance()->add_data_to_queue(PC_R_SOE_INFO, RAM_FLAG_SOE, SOEBuffer);
}