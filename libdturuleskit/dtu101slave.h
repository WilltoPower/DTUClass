/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtu101slave.h
  *Description: 
    用于实现dtu间隔单元的101服务
  *History: 
    1, 创建, wangjs, 2021-7-8
    2, 集成公共单元和间隔单元功能，添加对下101控制端管理, wangjs, 2021-8-9
**********************************************************************************/
#ifndef _DTU_101_SLAVE_H
#define _DTU_101_SLAVE_H
#include <lib60870/cs101_slave.h>
#include <dtusystemconfig.h>
#include <thread>
#include <atomic>
#include <map>
namespace DTU
{
    class D101Slave
    {
    private:
        D101Slave():_bStop(false) {
            is_101_init = false;
        }
    public:
        static D101Slave& instance() {
            static D101Slave slave;
            return slave;
        }
        ~D101Slave() { dtu101_stop_slave(); }

    public:
        void dtu101_init_slave();
        void dtu101_run_slave();
        void dtu101_stop_slave();
    public:
        // 时钟同步
        bool dtu_clockSyncHandler (IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);
        // 总召唤
        bool dtu_interrogationHandler(IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi);
        // 数据处理
        bool dtu_asduHandler(IMasterConnection connection, CS101_ASDU asdu);
        // 链路状态改变
        void dtu_linkLayerStateChanged(int address, LinkLayerState state);
        // 收发消息
        void dtu_rawMessageHandler(uint8_t* msg, int msgSize, bool sent);
        // 发送ASDU数据
        void send_asdu(CS101_ASDU asdu, IMasterConnection connection);
    public:
        // 获取AP参数
        CS101_AppLayerParameters get_applayer_param();
        // 读取地址
        int32_t get_address();
        // 主动上送
        void notify_master(CS101_ASDU asdu, bool autodestroy = true);
    private:
        std::unique_ptr<std::thread> _runthread;
        // 管理公共单元对间隔单元的101控制端
        std::map<uint32_t, std::tuple<std::string, uint16_t>> _m101master;
        // 本地101从站
        CS101_Slave _slave = nullptr;
        // 本地101从站的串口信息
        SerialPort _port = nullptr;

        std::atomic_bool _bStop;
    public:
        // 101规约是否初始化
        std::atomic_bool is_101_init;
    };
};
#endif