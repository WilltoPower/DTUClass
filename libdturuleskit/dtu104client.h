/*********************************************************************************
	*Copyright(C),2021-2025,sddl
	*FileName:  dtu104client.h
	*Description: 
		用于实现dtu间隔单元的104服务
	*History: 
		1, 创建, lhy, 2022-08-08
**********************************************************************************/
#include <string>
#include <map>

#include <lib60870/cs104_connection.h>
#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>

namespace DTU {

/** 发起者地址 */
using OA = int;
/** 公共地址 */
using CA = int;

class D104Client {
    public:
        static D104Client& instance() {
            static D104Client dclient;
            return dclient;
        }

    private:
        D104Client() {}
        ~D104Client();
    
    public:
        bool dtu104_init_client();
        void dtu104_run_client();
        void dtu104_stop_client();
    
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
         * @brief 发送ASDU消息
         * 
         * @param asdu 要发送的消息
         * @return true成功 false其他情况
         */
        bool send_asdu(CS101_ASDU asdu);

    public:
        bool dtu_asduRecvivedHandler(CS101_ASDU asdu);
        void dtu_connectHandler(CS104_Connection conn, CS104_ConnectionEvent event);
        void dtu_rawMessageHandler(uint8_t* msg, int size, bool sent);

    private:
        CS104_Connection con = nullptr;
        std::map<OA,CS104_Connection> connmap;

};

}