/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtu104server.h
  *Description:
    用于实现dtu间隔单元的104服务
  *History:
    1, 创建, wangjs, 2021-7-1
    2, 加入功能测试, wangjs, 2021-7-2
    3, 加入了对间隔单元的104连接管理功能, wangjs, 2021-8-6
**********************************************************************************/
#ifndef _DTU_104_SERVER_H
#define _DTU_104_SERVER_H
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <tuple>
#include <dtusystemconfig.h>
#include <lib60870/cs104_slave.h>
namespace DTU {
class D104Server {
private:
    D104Server() : _run(true) {
        is_104_init = false;
    }
public:
    static D104Server& instance(){
        static D104Server svr;
        return svr;
    } 
    ~D104Server() { dtu104_stop_server(); }

public:
    bool dtu104_init_server();
    void dtu104_run_server();
    void dtu104_stop_server();

public:
    // asdu接收
    virtual bool dtu_asduHandler(IMasterConnection connection, CS101_ASDU asdu);
    // 时钟同步
    virtual bool dtu_clockSyncHandler(IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);
    // 总召唤
    virtual bool dtu_interrogationHandler(IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi);
    // 104连接消息
    virtual bool dtu_connectionRequestHandler(const char *ipAddress);
    // 连接状态
    virtual void dtu_connectionEventHandler(IMasterConnection con, CS104_PeerConnectionEvent event);
    // 收发消息
    virtual void dtu_rawMessageHandler(IMasterConnection con, uint8_t* msg, int msgSize, bool sent);
    // 发送ASDU
    void send_asdu(CS101_ASDU asdu, IMasterConnection con);
    
public:
    // 获取AP参数
    CS101_AppLayerParameters get_applayer_param();
    // 读取地址
    int32_t get_address();
    // 主动上送
    void notify_master(CS101_ASDU asdu);

private:
    // 从ASDU中获取目录要求
    void get_directory_request(CS101_ASDU asdu, bool &bUseTime, uint64_t &beginTime, uint64_t &endTime);

private:
    // Slave control
    CS104_Slave _pSlave = nullptr;
    //
    std::atomic_bool _run;
    //
    std::unique_ptr<std::thread> _threadptr;

    std::mutex _write_file_lock;
    std::map<uint32_t, std::tuple<std::string, std::string, FILE *>> _write_file_map;
public:
    // 104规约是否初始化
    std::atomic_bool is_104_init;
    std::atomic_bool _is104_connect = { false };
    std::atomic_int _104_client_count;
};
}; // namespace DTU
#endif
