/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtu101master.h
  *Description:
    用于实现dtu间隔单元的101服务
  *History:
    1, 创建, lhy, 2022-01-06
    2, 集成公共单元和间隔单元功能，添加对下101控制端管理, lhy, 2022-01-06
**********************************************************************************/
#ifndef _DTU_101_MASTER_H
#define _DTU_101_MASTER_H
#include <lib60870/hal_serial.h>
#include <lib60870/cs101_master.h>


#include <thread>
#include <atomic>
#include <map>
#include <string>
#include <vector>

#include <pugixml/pugixml.hpp>


#define IOT_START 0x6401



namespace DTU
{
    using CA = int;
    using LinkAddr = int;

    class D101Master
    {
    private:
        D101Master() : _bStop(false) {}

    public:
        static D101Master& instance() {
            static D101Master master;
            return master;
        }
        ~D101Master();
    
    public:
        void dtu101_init_master(std::string serialPort, uint32_t baudrate, int address = 1);
        void dtu101_run_master();
        void dtu101_stop_master();

    public:
        // 时钟同步
        bool dtu_clockSyncHandler(int ca = 1);
        // 数据处理
        bool dtu_asduHandler(int address, CS101_ASDU asdu);
        // 链路状态改变
        void dtu_linkLayerStateChanged(int address, LinkLayerState state);
        // 消息回调
        void dtu_RawMessageHandler(uint8_t* msg, int msgSize, bool sent);

    public:
        /**
         * @brief 创建ASDU
         * 
         * @param isSequence 是否将信息对象编码为具有后续IOA值的信息对象的紧凑序列
         * @param cot 传送原因
         * @param oa 发起者地址
         * @param ca 公共地址(接收方地址)
         * @param isTest 测试标志一般不设置
         * @param isNegative 
         * @return CS101_ASDU ASDU对象
         */
        CS101_ASDU dtu_create_asdu(bool isSequence, CS101_CauseOfTransmission cot, int oa, int ca, bool isTest = false, bool isNegative = false);
        /**
         * @brief 发送ASDU
         * 
         * @param address 目标链路地址
         * @param asdu 要发送的ASDU
         * @return 是否正确发送
         */
        bool send_asdu(int address, CS101_ASDU asdu);
        /**
         * @brief 发送电能量总召唤命令
         * 
         * @param addr 公共单元地址
         */
        void send_elec_interrogation_cmd(CA addr);

        const CS101_AppLayerParameters GetAlParamter();

    private:
        std::unique_ptr<std::thread> _runthread;
        // 本地101从站
        CS101_Master _master = nullptr;
        // 本地101从站的串口信息
        SerialPort _port = nullptr;
        // 是否停止
        std::atomic_bool _bStop;

        std::vector<LinkAddr> slaveAddress;
        int slave_address = 0;
    };
};
#endif