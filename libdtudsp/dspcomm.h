/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dspprotocol.h
  *Description:
    DSP通信协议
  *History:
    1, 创建, wangjs, 2021-7-30
    2, 修改DSP主动上送的任务格式，加入的读取的FLAG， wangjs, 2021-8-18
    3, 加入对伏羲平台的兼容, wangjs, 2021-11-17
**********************************************************************************/
#ifndef _DSP_COMM_H
#define _DSP_COMM_H

#define CMDLEN 32         //命令区字节数
#define MAXDATATASKCNT 32 //数据标识区字数
#define MAXBLOCKLEN 131072

#define READ_DATA_FLAG_MEM_OFFSET 0  //数据标识区偏移量
#define READ_DATA_FLAG_LENGTH 256    // 标志区长度

#define WRITE_CMD_OFFSET 0x00040000  // arm通信写命令区域偏移量(字节)
#define WRITE_CMD_LENGTH 256          // 32字节 arm通信写命令长度
#define WRITE_DATA_OFFSET 0x00040100 // arm通信写数据区域偏移量(字节)
#define WRITE_DATA_LENGTH 0x00020000 // 128k字节 arm通信写数据区长度

#define READ_CMD_OFFSET 0x00000100  // arm通信读命令区域偏移量(字节)
#define READ_CMD_LENGTH 256          // 32字节 arm通信读命令区域长度
#define READ_DATA_OFFSET 0x00000200 // arm通信读数据区域偏移量(字节)
#define READ_DATA_LENGTH 0x00020000 // 128k字节 arm通信读数据区长度

#define DSP_RCMD_BUFFLEN 32
#define DSP_WCMD_BUFFLEN 32
#define DSP_RDATA_BUFFLEN (128 * 1024)
#define DSP_WDATA_BUFFLEN (128 * 1024)

#include <list>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include "dspprotocol.h"

#ifdef _FUXI_H2
#define CO_IOCTL_GET_REG 1
#define CO_IOCTL_SET_REG 2
#define CO_IOCTL_EXECUTE 3
#define CO_IOCTL_GET_DATA 4
#define CO_IOCTL_SET_DATA 5
#define CO_IOCTL_GET_CONFIGS 6
#define CO_IOCTL_SET_PC 7

#ifndef CO_MB_FIFO_SIZE
#define CO_MB_FIFO_SIZE 32
#endif

struct co_regio {
    uint32_t reg;
    uint32_t val;
};

struct co_io {
    uint8_t meta;
    uint16_t metadata;
    uint32_t data[CO_MB_FIFO_SIZE - 1];
    uint8_t len;
};

struct co_config {
    uint32_t text_start;
    uint32_t text_size;
    uint32_t data_start;
    uint32_t data_size;
};
struct dev_co {
    char* name;
    int fd_co = -1;
    int fd_shm_w = -1;
    int fd_shm_r = -1;

    uint8_t *shm_w = nullptr;
    uint8_t *shm_r = nullptr;

    struct co_io coio;
};
#endif
namespace DTU {
struct RAMTask {
    RAMTask() {}
    RAMTask(int32_t id, uint16_t code) : _taskid(id), _cmdcode(code) {}

    bool operator()(const RAMTask &lhs, const RAMTask &rhs) const { return (lhs._taskid > rhs._taskid); }
    int32_t _taskid = 0;   //任务编号
    uint16_t _cmdcode = 0; //读数据命令
};

#define TYPE_READ 0
#define TYPE_WRITE 1
// 构建优先级队列
// using RAMLIST = std::priority_queue<RAMTask, std::vector<RAMTask>, RAMTask>;
using RAMLIST = std::list<RAMTask>;
class DSPComm {
public:
    DSPComm(DSPComm &) = delete;
    DSPComm &operator=(const DSPComm &) = delete;
    static DSPComm *GetInstance() {
        static DSPComm dsp;
        return &dsp;
    }
    ~DSPComm();
    ///////////////////////
    //打开驱动设备
    bool dsp_start();
    //关闭与DSP的通讯
    void dsp_stop();
    //
    //int DspHandle(uint16_t cmd, const buffer &src, buffer &reslut, uint32_t waitSec = 0, uint16_t reboot = 1);
    // 写数据
    int dsp_write_data(uint16_t cmd, const buffer& src, uint32_t waitSec = 0, uint16_t reboot=1);
    // 写控制
    int dsp_write_ctrl(uint16_t cmd);
    // 读数据
    int dsp_read_data(uint16_t cmd, buffer& result, int32_t waitSec = 0);
    ////////////////////////////////////
    void dsp_run();
    // 初始化整定定值
    DTU::buffer init_adjust_param();

    void update_connect(uint16_t state);

protected:
    // 写入/控制
    int try_execute_write(DSPProtocol &recvData, uint16_t type, uint32_t nRetry = 3, uint32_t waitMillSec = 3);
    // 读取
    int try_execute_read(DSPProtocol &srcData, buffer &recvData);

public:
    // 加入数据处理队列
    void add_data_to_queue(uint16_t cmd, uint16_t flag, const buffer &result);

protected:
    // 获取数据队列
    uint16_t get_data_from_queue(buffer &result, uint16_t &flag);

private:
    DSPComm();
    // 发送数据到RAM
    int SendToDsp(DSPProtocol &srcData);
    // 从RAM读取数据
    int ReadFromDsp(DSPProtocol &recvData);
// #ifdef _FUXI_H2
//     int CheckRecvMsg(uint64_t id, const DSPProtocol &srcData);
// #else
//     // 信息校验
//     int CheckRecvMsg(const DSPProtocol &srcData);
// #endif
    int CheckRecvMsg(const DSPProtocol &srcData);
    // 读取RAM的标志位
    size_t GetRAMTask(RAMLIST &tasklist, bool needsort = true);
    //
    void SetCmdArea(const DSPProtocol &srcData);
    void SetDataArea(const DSPProtocol &srcData);
    //
    void GetCmdArea(DSPProtocol &dstData);
    void GetDataArea(DSPProtocol &dstData);
    // 处理
    int ProcessRAMReadTask(const RAMTask &taskinfo);
    int ClearReadTask(int taskid);
    // static void CheckToGetDSPData();
    int dsp_handle();
private:
#ifdef _FUXI_H2
    dev_co _dev_co;
#else
    /////////////////////////////////////////////////
    // pthread_mutex_t m_mutex;			                 //互斥量 for 操作双口RAM
    int32_t _fd = -1;          //字符驱动设备文件描述符
    char *_mapaddr = nullptr;  // RAM映射后的内存地址
    int32_t _controlstat = -1; //控制状态,无控制执行时为-1,有控制时为执行的控制命令字

    // char _readcmdbuff[DSP_RCMD_BUFFLEN] = {};
    // char _readdatabuff[DSP_RDATA_BUFFLEN] = {}; // 128k
    // char _sendcmdbuff[DSP_WCMD_BUFFLEN] = {};
    // char _senddatabuff[DSP_WDATA_BUFFLEN] = {}; // 128k
    ////////////////////////////////////////////////
#endif
    unsigned short *m_readflag_buff = nullptr;
    char *m_writecmd_buff = nullptr;
    char *m_writedata_buff = nullptr;
    char *m_readcmd_buff = nullptr;
    char *m_readdata_buff = nullptr;
    //
    std::mutex _ram_lock;
    // 负责实时接收DSP数据
    std::unique_ptr<std::thread> _dspthread;
    // 实时处理DSP数据
    std::unique_ptr<std::thread> _dspHandleThread;

    std::atomic<bool> _bStopDsp;

    std::mutex _queue_lock;
    std::queue<std::tuple<uint16_t, uint16_t, buffer>> _handleQueue;
private:
    void sendtoGoose(DTU::buffer &recv,int retrytime = 50,int retrycount = 3);

    uint16_t ConnectState = 0x00;
};
} // namespace DTU

#endif
