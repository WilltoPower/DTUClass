/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dspprotocol.h
  *Description:
    DSP通信协议
  *History:
    1, 创建, wangjs, 2021-7-30
        2, 对DSP主动上送任务加入优先级, wangjs, 2021-8-19
**********************************************************************************/
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#endif
#include <string.h>
#include <fcntl.h>
#include <list>
#include <algorithm>
#include <string>
#include "dspcomm.h"
/////////////////////////////////////////////
#include <unordered_map>
#include <tuple>
#include <functional>
#include <dtulog.h>
#include <dtuerror.h>
#include <dtucmdcode.h>
#include <dtustorage.h>
#include <dtunotifymanager.h>
#include <dtureporthelper.h>
#include <dturulesfile.h>
#include <dtucommon.h>
#include <dtuparamconfig.h>

#include <dtugoosepublisher.h>
#include <dtuconnect.h>

#define _FUXI_H2

using namespace std;
using namespace DTU;

#define USECRC 1
// 突发遥测数据长度
#define YC_COS_BURST_LENGTH 20

// 命令
#define FUNC_TYPE_CMD 0
// 数据
#define FUNC_TYPE_DATA 1

#define CMD_TYPE_READ 0  // 读命令
#define CMD_TYPE_WRITE 1 // 写命令
#define CMD_TYPE_REPLY 2 // 应答

// 定义对应答数据的分析
// 控制类应答
int getMCUSelfInfo(char *buff) { return 0; }
void makenotify(uint16_t cmd, const buffer &data, DTU::dtuprotocol &proto) {
    proto._header = 0xbb66;
    proto._cmd = cmd;
    proto._curLen = data.size();
    proto._totleLen = data.size();
    proto._data = data;
    proto._blockno = 0xFFFF;
}
DSPComm::DSPComm() {}

DSPComm::~DSPComm() {

}

// StartDSPDriver()启动DSP驱动程序
bool DSPComm::dsp_start() {
#ifdef _FUXI_H2
    DTULOG(DTU_INFO,(char*)"FUXI平台");
    _dev_co.name = (char*)"/dev/co8101";
    // 写数据区
    _dev_co.fd_shm_w = open("/dev/sc-shm2", O_RDWR);
    if (!_dev_co.fd_shm_w) {
        DTULOG(DTU_ERROR, (char *)"打开/dev/sc-shm2失败");
        return false;
    }
    // 读取数据区
    _dev_co.fd_shm_r = open("/dev/sc-shm3", O_RDWR);
    if (!_dev_co.fd_shm_r) {
        DTULOG(DTU_ERROR, (char *)"打开/dev/sc-shm3失败");
        return false;
    }

    _dev_co.shm_w = (uint8_t*)mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, _dev_co.fd_shm_w, 0);
    if (_dev_co.shm_w == MAP_FAILED) {
        DTULOG(DTU_ERROR, (char *)"打开写数据区映射失败");
        return false;
    }
    _dev_co.shm_r = (uint8_t*)mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, _dev_co.fd_shm_r, 0);
    if (_dev_co.shm_r == MAP_FAILED) {
        DTULOG(DTU_ERROR, (char *)"打开读数据区映射失败");
        return false;
    }

    _dev_co.fd_co = open(_dev_co.name, O_RDWR);
    if (!_dev_co.fd_co) {
        DTULOG(DTU_ERROR, (char *)"打开RAM失败");
        return false;
    }
   
    m_writecmd_buff = (char*)_dev_co.shm_w;                                             // arm写命令通信区起始地址
    m_writedata_buff = (char*)(_dev_co.shm_w + WRITE_CMD_LENGTH);                       // arm写数据区起始地址
    m_readflag_buff = (unsigned short *)(_dev_co.shm_r + READ_DATA_FLAG_MEM_OFFSET);    //可读数据标识区起始地址
    m_readcmd_buff = (char *)(_dev_co.shm_r + READ_DATA_FLAG_LENGTH);                   // arm读命令区起始地址
    m_readdata_buff = (char *)(_dev_co.shm_r + READ_DATA_FLAG_LENGTH+ READ_CMD_LENGTH); // arm读数据区起始地址
#else
    _fd = open("/dev/dpramdev", O_RDWR);
    if (_fd == -1) {
        DTULOG(DTU_ERROR, (char *)"打开RAM驱动失败,error=%d, %s", errno, strerror(errno));
        return false;
    }
    _mapaddr = (char *)mmap(NULL, 512 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if ((void *)_mapaddr == MAP_FAILED) {
        DTULOG(DTU_ERROR, (char *)"RAM映射失败,error=%d, %s", errno, strerror(errno));
        close(_fd);
        _fd = -1;
        return false;
    }

    m_readflag_buff = (unsigned short *)(_mapaddr + READ_DATA_FLAG_MEM_OFFSET); //可读数据标识区起始地址
    m_writecmd_buff = _mapaddr + WRITE_CMD_OFFSET;                              // arm写命令通信区起始地址
    m_writedata_buff = _mapaddr + WRITE_DATA_OFFSET;                            // arm写数据区起始地址
    m_readcmd_buff = _mapaddr + READ_CMD_OFFSET;                                // arm读命令区起始地址
    m_readdata_buff = _mapaddr + READ_DATA_OFFSET;                              // arm读数据区起始地址
#endif
    // 启动通信线程
    _bStopDsp = false;

    _dspthread = std::make_unique<std::thread>(&DSPComm::dsp_run, this);

    _dspHandleThread = std::make_unique<std::thread>(&DSPComm::dsp_handle, this);

    return true;
}

void DSPComm::SetCmdArea(const DSPProtocol &srcData) {
    uint32_t offset = 0;
    memcpy(m_writecmd_buff + offset, &srcData._crc32, sizeof(srcData._crc32));
    offset += sizeof(srcData._crc32);
    memcpy(m_writecmd_buff + offset, &srcData._cmd, sizeof(srcData._cmd));
    offset += sizeof(srcData._cmd);
    memcpy(m_writecmd_buff + offset, &srcData._totleLen, sizeof(srcData._totleLen));
    offset += sizeof(srcData._totleLen);
    memcpy(m_writecmd_buff + offset, &srcData._curLen, sizeof(srcData._curLen));
    offset += sizeof(srcData._curLen);
    memcpy(m_writecmd_buff + offset, &srcData._blockno, sizeof(srcData._blockno));
    offset += sizeof(srcData._blockno);
}

void DSPComm::SetDataArea(const DSPProtocol &srcData) {
    if (srcData.GetData().size() != 0) {
        memcpy(m_writedata_buff, srcData.GetData().const_data(), srcData.GetData().size());
    }
}

//
void DSPComm::GetCmdArea(DSPProtocol &dstData) {
    uint32_t offset = 0;
    memcpy(&dstData._crc32, m_readcmd_buff + offset, sizeof(dstData._crc32));
    offset += sizeof(dstData._crc32);
    memcpy(&dstData._cmd, m_readcmd_buff + offset, sizeof(dstData._cmd));
    offset += sizeof(dstData._cmd);
    memcpy(&dstData._totleLen, m_readcmd_buff + offset, sizeof(dstData._totleLen));
    offset += sizeof(dstData._totleLen);
    memcpy(&dstData._curLen, m_readcmd_buff + offset, sizeof(dstData._curLen));
    offset += sizeof(dstData._curLen);
    memcpy(&dstData._blockno, m_readcmd_buff + offset, sizeof(dstData._blockno));
    offset += sizeof(dstData._blockno);
    // for(int i=0;i<offset;i++){
    //     printf("0x%02x ", m_readcmd_buff[i]);
    // }
    // printf("\n");
}

void DSPComm::GetDataArea(DSPProtocol &dstData) {
    if (dstData._curLen != 0) {
        dstData.SetData(buffer(m_readdata_buff, dstData._curLen));
    }
}

// StopDSPDriver()	停止DSP驱动程序
void DSPComm::dsp_stop() {
    _bStopDsp = true;
    _dspthread->join();
    _dspHandleThread->join();
#ifdef _FUXI_H2
    if (_dev_co.fd_co)
        close(_dev_co.fd_co);
    if (_dev_co.fd_shm_w)
        close(_dev_co.fd_shm_w);
    if (_dev_co.fd_shm_r)
        close(_dev_co.fd_shm_r);
#else
    if (_mapaddr != nullptr) {
        munmap((void *)_mapaddr, 512 * 1024);
    }
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
    _mapaddr = nullptr;
#endif
    m_readflag_buff = nullptr;
    m_writecmd_buff = nullptr;
    m_writedata_buff = nullptr;
    m_readcmd_buff = nullptr;
    m_readdata_buff = nullptr;
}

int DSPComm::SendToDsp(DSPProtocol &srcData) {
    if (!m_writedata_buff || !m_writecmd_buff) {
        DTULOG(DTU_ERROR, (char *)"DSPComm RAM写缓存为空");
        return DTU_DSP_ERROR;
    }

    SetCmdArea(srcData);
    SetDataArea(srcData);

    char tmpbuff[2] = {};
    memset(tmpbuff, 0xFF, 2);
    for (int i = 0; i < 3; i++) {
#ifdef _FUXI_H2

        struct co_io send;
        memset(&send,0,sizeof(struct co_io));
        send.data[0] = 10 << 24; // offset in shm0
        send.data[1] = 16;       // length of data in shm0
        send.meta = 0;
        send.len = 16;
        send.data[2] = 0xBB66;  //数据头
        send.data[3] = 2;       //flag
        send.data[5] = 0x66BB;  //数据尾

        int retval = ioctl(_dev_co.fd_co, CO_IOCTL_SET_DATA, &send);
        if (retval){
            DTULOG(DTU_ERROR, (char *)"DSPComm 2 发送写中断失败,%d, 重试%d", retval, i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#else
        int retval = write(_fd, tmpbuff, 2); //再向DSP发送中断
        if (retval < 0)
        {
            DTULOG(DTU_ERROR, (char *)"DSPComm 发送写中断失败,%d, 重试%d", retval, i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#endif
        else {
            return DTU_SUCCESS;
        }
    }
    return DTU_DSP_ERROR;
}

int DSPComm::ReadFromDsp(DSPProtocol &recvData) {
    if (!m_readdata_buff || !m_readcmd_buff) {
        DTULOG(DTU_ERROR, (char *)"DSPComm 读取缓存为空");
        return DTU_DSP_ERROR;
    }
    char tmpbuff[2] = {};
    #ifdef _FUXI_H2
    memset(&_dev_co.coio, 0, sizeof(struct co_io ));
    #endif
    //for (int i = 0; i < 10; i++) {
    for (int i = 0; i < 3; i++) {
#ifdef _FUXI_H2

        auto retval = ioctl(_dev_co.fd_co, CO_IOCTL_GET_DATA, &_dev_co.coio);
        if (retval){
            DTULOG(DTU_WARN, (char *)"DSPComm 读取中断失败[%s]:%d,重试%d, ", strerror(errno), retval, i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
#else
        auto retval = read(_fd, tmpbuff, 2);
        if (retval < 0) {
            DTULOG(DTU_WARN, (char *)"DSPComm 读取中断失败%d,重试%d", retval, i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
#endif
        } else {
            // 从命令区读取数据
            GetCmdArea(recvData);
            GetDataArea(recvData);

#ifdef _FUXI_H2
            // // 循环读空ioctl队列
            int count = 0;
            while(!ioctl(_dev_co.fd_co, CO_IOCTL_GET_DATA, &_dev_co.coio)){
                printf("TEST +++++++++++++++++++++++++++++++++++++++++++++ [%d]\n",count++);
                if(count == 64)
                {
                    break;
                }
            }
            // static uint32_t count = 0;
            // printf("TEST >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            // printf("TEST [DSP count %u]  [ARM count %u] \n",_dev_co.coio.data[1],count++);
            // // for(int m = 0;m<=7;m++)
            // // {
            // //     printf("0x%04X ",_dev_co.coio.data[m]);
            // // }
            // // putchar(10);
            // printf("TEST <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
#endif

            return DTU_SUCCESS;
        }
    }
    return DTU_DSP_ERROR;
}

int DSPComm::try_execute_write(DSPProtocol &recvData, uint16_t type, uint32_t nRetry, uint32_t waitMillSec) {
    int nCount = 0;
    int nResend = 0;

    for (;;) {
        if (DTU_SUCCESS != SendToDsp(recvData)) {
            nResend++;
            if (nResend <= nRetry){
                DTULOG(DTU_WARN, (char *)"DSPComm 下发命令[0x%04X]失败 重试[%d]!!!", recvData._cmd, nResend);
                continue;
            }
            else{
                DTULOG(DTU_ERROR, (char *)"DSPComm 下发命令[0x%04X]失败 放弃[%d]!!!", recvData._cmd, nResend);
                return DTU_DSP_ERROR;
            }
        }
        nResend = 0;
        // 等待DSP处理
        if(recvData._cmd != PC_W_INT_FIX && recvData._cmd != PC_W_ADJ_FIX)
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMillSec));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(100));    // 整定定值和设备定值写时间较长,等待时间特殊处理
        // 接收回复
        DSPProtocol ack;
        if (DTU_SUCCESS != ReadFromDsp(ack)) {
            DTULOG(DTU_WARN, (char *)"DSPComm 接收命令[0x%04X]确认失败 放弃!!!", recvData._cmd);
            return DTU_DSP_ERROR;
        }

        if (ack.SelfCheck(type)) {
            break;
        }
        if (ack._cmd == TX_PC_NACK) {
            return DTU_DSP_ERROR;
        }
        ///////////////////////
        DTULOG(DTU_WARN, (char *)"DSPComm 命令0x%04X接收到错误应答信号0x%04X, 重新发送%d", recvData._cmd, ack._cmd,
               nResend);
        nResend++;
        if (nResend > nRetry) {
            DTULOG(DTU_WARN, (char *)"DSPComm 命令0x%04X接收到错误应答信号0x%04X错误超过3次,放弃!!!", recvData._cmd,
                   ack._cmd);
            return DTU_DSP_ERROR;
        }
    }
    return DTU_SUCCESS;
}

int DSPComm::try_execute_read(DSPProtocol &srcData, buffer &recvData) {
    try
    {
        recvData.remove();
        int nReSend = 1;
        for (;;) {
            // 发送数据
            if (SendToDsp(srcData) != DTU_SUCCESS) {
                DTULOG(DTU_ERROR, (char *)"DSPComm try_execute_read 发送命令0x%04X失败,放弃任务", srcData._cmd);
                return DTU_DSP_ERROR;
            }

            // 等待DSP装填好数据
            if (PC_R_GOOSE_INFO == srcData._cmd)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));  // GOOSE数据等待时间减少以怎加通信效率
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(20));

            nReSend = 0;
            DSPProtocol tempData;
            uint32_t nTotleLen = 0;
            uint32_t nCurlen = 0;
            uint32_t lastNo = 0xFFFF;

            // 开始接收

            for (;;) {
                // 读取数据
                if (ReadFromDsp(tempData) != DTU_SUCCESS) {
                    DTULOG(DTU_ERROR, (char *)"DSPComm try_execute_read 命令[0x%04X]接收数据失败,放弃!!!", srcData._cmd);
                    return DTU_DSP_ERROR;
                }
                // 成功,检查数据
                if (CheckRecvMsg(tempData) != DTU_SUCCESS) {
                    // 数据有误,需要重新读取
                    // 发送非应答信号
                    DSPProtocol nack(PC_W_NACK, 0, 0, 0xFFFE);
                    if (DTU_SUCCESS != (SendToDsp(nack))) {
                        DTULOG(DTU_ERROR, (char *)"DSPComm try_execute_read 命令[0x%04X]回复NACK失败,放弃读取!!!",
                            srcData._cmd);
                        // 如果发送非应答失败,认为通信断开,放弃本次读取
                        return DTU_DSP_ERROR;
                    }
                    //
                    if (nReSend == 3) {
                        DTULOG(DTU_ERROR, (char *)"DSPComm try_execute_read 命令[0x%04X]读取数据错误超过3次,放弃!!!",
                            srcData._cmd);
                        return DTU_DSP_ERROR;
                    }
                    DTULOG(DTU_WARN, (char *)"DSPComm try_execute_read 命令[0x%04X]回复NACK,等待重传%d!!!", srcData._cmd,
                        nReSend);
                    nReSend++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(133));
                    continue;
                } 
                else // 数据正确
                {
                    nTotleLen = tempData._totleLen;
                    ///////////
                    if (recvData.size() == 0){
                        recvData.resize(nTotleLen);
                    }

                    // 如果块号不相等则表示新数据,长度计数nCurlen累加
                    if (lastNo != tempData._blockno){
                        recvData.set(nCurlen, tempData.GetData());
                        nCurlen += tempData._curLen;
                        lastNo = tempData._blockno;
                    }

                    // 回复  ARM收到

                    DSPProtocol ack(PC_W_ACK, 0, 0, 0xFFFE);
                    if (DTU_SUCCESS != (SendToDsp(ack))) {
                        DTULOG(DTU_WARN, (char *)"DSPComm try_execute_read 命令[0x%04X]回复ACK失败!!!", srcData._cmd);
                    }

                    // blockno == 0xFFFC有如下几种情况
                    if(tempData._blockno == 0xFFFC)
                    {
                        // 情况一:长度计数(大于)等于nTotleLen,则表示已经接收完成(大于?)
                        if(nCurlen >= nTotleLen)
                        {
                            // if(nCurlen == 177280)
                            // {
                            //     bool ret = true;
                            //     printf("TEST >>>>>>>>>>>>>>>>>> \n");
                            //     std::string fullName = get_exec_dir() + "/protect/selfcheck/RecvBuffer.txt";
                                
                            //     FILE* fp = fopen(fullName.c_str(), "wb+");
                            //     if (!fp){
                            //         printf("TEST error");
                            //         ret = false;
                            //     }
                            //     if(ret) {
                            //         fwrite(recvData.const_data(),1,recvData.size(), fp);
                            //         fclose(fp);
                            //     }
                            //     printf("TEST <<<<<<<<<<<<<<<<<<< \n");
                            // }
                            return DTU_SUCCESS;
                        }
                        // 情况二:长度计数(大于)等于nTotleLen,则是重传的数据附加到了新的帧上,是错误的帧
                        else
                        {
                            return DTU_DSP_ERROR;
                        }
                    }


                }
            }
        }
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,"try_execute_read发生错误:%s",e.what());
        return DTU_UNKNOWN_ERROR;
    }
    return DTU_UNKNOWN_ERROR;
}

int DSPComm::CheckRecvMsg(const DSPProtocol &srcData) 
{
    if (srcData._totleLen == 0) {
        if (srcData._curLen != 0) {
            DTULOG(DTU_ERROR, (char *)"%s,%d,[%d]命令帧--总长度为0,数据帧长度不为0!", __FILE__, __LINE__,
                   srcData._cmd);
            return DTU_DSP_DATA_ERROR;
        }
        if (srcData._blockno != 0xFFFE) {
            DTULOG(DTU_ERROR, (char *)"%s,%d,[%d]命令帧--数据块号异常", __FILE__, __LINE__, srcData._cmd);
            return DTU_DSP_DATA_ERROR;
        }
    } else {
        if (srcData._curLen == 0) {
            DTULOG(DTU_ERROR, (char *)"%s,%d,[0x%04X]命令帧--数据帧长度为0!", __FILE__, __LINE__, srcData._cmd);
            return DTU_DSP_DATA_ERROR;
        }
        else if (srcData._blockno != 0xFFFC && !(srcData._blockno >= 0 && srcData._blockno <= 0xFFF0)) {
            DTULOG(DTU_ERROR, (char *)"%s,%d,[0x%04X]数据帧--数据块号异常!blocknum=[%d]", __FILE__, __LINE__,
                   srcData._cmd, srcData._cmd);
            return DTU_DSP_DATA_ERROR;
        }
    }
    return DTU_SUCCESS;
    //计算校验码
#if USECRC
#endif
}

#define DSP_SCAN_PERIOD 2

void DSPComm::dsp_run() {
    RAMLIST tasklist;
    if (m_readflag_buff == nullptr) {
        DTULOG(DTU_ERROR, (char *)"%s,%d 标志缓存readflagbuff为空", __FILE__, __LINE__);
        return;
    }
    while (!_bStopDsp) {
        tasklist.clear();
        //如果有数据任务需要获取,则从中选出优先级最高的一个进行处理,处理完后,需要重新获取任务信息,考虑有高优先级任务出现
        if (GetRAMTask(tasklist) > 0) {
            //进行任务处理,之后将该任务编号对应的位置0
            const RAMTask &tempinfo = tasklist.front();
            // DTULOG(DTU_DEBUG, (char *)"从DSP读取到任务0x%04X", tempinfo._taskid);
            int ret = ProcessRAMReadTask(tempinfo);
            if (ret != DTU_SUCCESS) {
                DTULOG(DTU_ERROR, (char *)"执行任务[%d]失败(-1)/暂停(1),%d]", tempinfo._taskid, ret);
            }
            ClearReadTask(tempinfo._taskid);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(DSP_SCAN_PERIOD));
    }
    DTULOG(DTU_INFO, (char *)"退出DSP接收任务");
}

// DSP失败重传等待时间
#define DSP_ERROR_RETRANS_TIME 50

// 写数据
int DSPComm::dsp_write_data(uint16_t cmd, const buffer& src, uint32_t waitSec, uint16_t reboot)
{
    static std::vector<uint16_t> write_data_list = {
        PC_W_CLK, PC_W_PUB_FIX, PC_W_YB_ON_OFF_INFO, PC_W_PRO_FIX, PC_W_AUTORECLOSE_FIX, PC_W_FA_FIX,
        PC_W_DFA_FIX, PC_W_TQHZ_FIX, PC_W_AUTO_SPLIT_FIX, PC_W_XDLGND_FIX, PC_W_LINEBRKALARM_FIX, PC_W_POWERDRIVER_FIX,
        PC_W_AUTOCFG_FIX, PC_W_INT_FIX, PC_W_ADJ_FIX, PC_W_GOOSE_TABLE_PKG,
    };
    auto ita = std::find(write_data_list.begin(), write_data_list.end(), cmd);
    if (ita == write_data_list.end()){
        // 
        DTULOG(DTU_ERROR,(char*)"dsp_write_data 不支持命令0x%04X", cmd);
        return DTU_INVALID_CMD;
    }
    int blockcnt = src.size() / MAXBLOCKLEN;
    if (src.size() % MAXBLOCKLEN)
        blockcnt++;
    
    uint32_t sendcurlen = 0;
    uint16_t sendblocknum = 0;
    int nRet = DTU_SUCCESS;
    ///////////////////////////////////////////////
    std::lock_guard<std::mutex> lock(_ram_lock);
    for (auto j = 0; j < blockcnt; j++) 
    {
        if (j != blockcnt - 1) {
            sendcurlen = MAXBLOCKLEN; // short 最大值65535
            sendblocknum = j;
        } else {
            sendcurlen = src.size() % (MAXBLOCKLEN);
            if (sendcurlen == 0)
                sendcurlen = MAXBLOCKLEN;
            sendblocknum = 0xFFFC;
        }
        DSPProtocol writedata(cmd, src.size(), sendcurlen, sendblocknum, reboot);
        writedata.SetData(src.get(j * MAXBLOCKLEN, sendcurlen));
        // 如果失败进行三次重新发送
        for(int m=0; m<3; m++)
        {
            if (DTU_SUCCESS != (try_execute_write(writedata, DSPProtocol::DSP_WRITE, 9, waitSec))) {
                // 下发失败
                DTULOG(DTU_ERROR, (char *)"dsp_write_data 命令[0x%04X]第[%d]块下发失败,重试%d/3", cmd, (j + 1), m+1);
                nRet = DTU_DSP_ERROR;
                std::this_thread::sleep_for(std::chrono::milliseconds(DSP_ERROR_RETRANS_TIME));
            }
            else {
                // 下发成功
                nRet = DTU_SUCCESS;
                break;
            }
        }

        if(nRet != DTU_SUCCESS) {
            DTULOG(DTU_ERROR, (char *)"dsp_write_data 命令[0x%04X]下发失败", cmd);
            return nRet;
        }
            
    }
    return nRet;
}

// 写控制
int DSPComm::dsp_write_ctrl(uint16_t cmd)
{
    static std::vector<uint16_t> write_cmd_list = {
        PC_W_SELF_CHECK, PC_W_RST_SIG_YF, PC_W_DSP_REBOOT, PC_W_YK_SET,
        PC_W_YK_CANCLE, PC_W_YK_FZ_YF, PC_W_YK_HZ_YF, PC_W_POWER_DRIVER,
        PC_W_POWER_DRIVER_SYB_ON, PC_W_GOOSE_COMM_YC, PC_W_GOOSE_COMM_RST,

        PC_W_ACK, PC_W_NACK, PC_W_CRC_BAD, PC_W_POWER_DRIVER_PRE, 

        PC_W_YK_FZ_JD, PC_W_YK_HZ_JD, PC_W_RST_SIG_JD, PC_W_YK_HH_QD, PC_W_YK_HH_TC,
        PC_W_LED_TX_LINK_UP, PC_W_LED_TX_LINK_DOWN, PC_W_BLK_RST,

        PC_W_TEST_RELAY, PC_W_TEST_LED, PC_W_TEST_LB, PC_W_TEST_FORMAT,
    };
    auto ita = std::find(write_cmd_list.begin(), write_cmd_list.end(), cmd);
    if (ita == write_cmd_list.end()){
        // 
        DTULOG(DTU_ERROR,(char*)"dsp_write_ctrl 不支持命令0x%04X", cmd);
        return DTU_INVALID_CMD;
    }
    int nRet = DTU_SUCCESS;
    ///////////////////////////////////////////////
    std::lock_guard<std::mutex> lock(_ram_lock);
    DSPProtocol writecmd(cmd, 0, 0, 0xFFFE);

    for(int m=0;m<3;m++)
    {
        if (DTU_SUCCESS != (try_execute_write(writecmd, DSPProtocol::DSP_CONTROL))) {
            DTULOG(DTU_ERROR, (char *)"dsp_write_ctrl 命令[0x%04X]执行失败,重试%d/3", cmd, m+1);
            nRet = DTU_DSP_ERROR;
            std::this_thread::sleep_for(std::chrono::milliseconds(DSP_ERROR_RETRANS_TIME));
        }
        else {
            nRet = DTU_SUCCESS;
            break;
        }
    }

    if(nRet != DTU_SUCCESS) {
        DTULOG(DTU_ERROR, (char *)"dsp_write_ctrl 命令[0x%04X]执行失败", cmd);
    }

    return nRet;
}

// 读数据
int DSPComm::dsp_read_data(uint16_t cmd, buffer& result, int32_t waitSec)
{
    static std::vector<uint16_t> read_cmd_list = {
            // 读定值
            PC_R_PUB_FIX,           PC_R_YB_STATE_INFO,     PC_R_PRO_FIX,           PC_R_AUTORECLOSE_FIX,   PC_R_FA_FIX,
            PC_R_DFA_FIX,           PC_R_TQHZ_FIX,          PC_R_AUTO_SPLIT_FIX,    PC_R_XDLGND_FIX,        PC_R_LINEBRKALARM_FIX,
            PC_R_POWERDRIVER_FIX,   PC_R_AUTOCFG_FIX,      PC_R_INT_FIX,           PC_R_ADJ_LCD_FIX,       PC_R_ADJ_FIX,           
            // 读数据
            PC_R_CLK, PC_R_YC_DATA, PC_R_HYX_DATA, PC_R_RST, PC_R_CHECK, PC_R_OPER_INFO,
            PC_R_PRO_ACT_INFO, PC_R_SOE_INFO, PC_R_SOFT_PROG, PC_R_LO_SAP_DATA, PC_R_LO_ADJ_DATA,
            PC_R_LO_FAUL, PC_R_PROFUN_STATE, PC_R_MAIN_MENU_INFO, PC_R_ALARM_INFO, PC_R_SYX_INFO,
            PC_R_COS_DATA, PC_R_ZTLB_DATA, PC_R_EXV_DATA, PC_R_LOG_DATA, PC_R_POP_INFO,
            PC_R_PRI_PRO_INFO, PC_R_FIX_AREA_INFO, PC_R_GOOSE_INFO, PC_R_GOOSE_TABLE_INFO, PC_R_XY,
    };
    auto ita = std::find(read_cmd_list.begin(), read_cmd_list.end(), cmd);
    if (ita == read_cmd_list.end()){
        // 
        DTULOG(DTU_ERROR,(char*)"dsp_read_data 不支持命令0x%04X", cmd);
        return DTU_INVALID_CMD;
    }
    int nRet = DTU_SUCCESS;
    ///////////////////////////////////////////////
    std::lock_guard<std::mutex> lock(_ram_lock);
    //读操作,发送一次,收一次/回复一次应答
    DSPProtocol readCmd(cmd, 0, 0, 0xFFFE);
    // 如果失败最多读取三次
    for(int m=0; m<3; m++)
    {
        if (DTU_SUCCESS != (try_execute_read(readCmd, result))) {
            DTULOG(DTU_ERROR, (char *)"dsp_read_data 命令[0x%04X]读取数据失败,重试%d/3", cmd, m+1);
            nRet = DTU_DSP_ERROR;
            std::this_thread::sleep_for(std::chrono::milliseconds(DSP_ERROR_RETRANS_TIME));
        }
        else {
            nRet = DTU_SUCCESS;
            break;
        }
    }

    if(nRet != DTU_SUCCESS) {
        DTULOG(DTU_ERROR, (char *)"DSPComm 命令[0x%04X]读取数据失败,", cmd);
    }

    if (cmd == PC_R_CHECK && nRet == DTU_SUCCESS) {
        uint16_t check = result.get(22, sizeof(uint16_t)).value<uint16_t>();
        uint16_t checkresult = (check & 0x400) | ConnectState;
        result.set(22, (char*)&checkresult, sizeof(uint16_t));
    }

    return nRet;
}

// needsort 是否需要按任务优先级排序
size_t DSPComm::GetRAMTask(RAMLIST &tasklist, bool needsort) {

    static std::unordered_map<int, uint32_t> s_cmdmap = {
        {RAM_FLAG_GOOSE, PC_R_GOOSE_INFO},       /* 读取GOOSE包内容 */
        {RAM_FLAG_SOE, PC_R_SOE_INFO},           /* 读取SOE记录信息 */
        {RAM_FLAG_COS, PC_R_COS_DATA},           /* 读COS数据 */
        {RAM_FLAG_PROTRECORDER, PC_R_ZTLB_DATA}, /* 读取暂态录波数据101相关 */
        {RAM_FLAG_PROTACT, PC_R_PRO_ACT_INFO},   /* 读保护动作报告 */
        {RAM_FLAG_WARN, PC_R_ALARM_INFO},        /* 读告警信息 */
        {RAM_FLAG_OPTRCD, PC_R_OPER_INFO},       /* 读取操作报告 */
        {RAM_FLAG_SELFCHK, PC_R_CHECK},          /* 自检数据 */
        {RAM_FLAG_EXTREME, PC_R_EXV_DATA},       /* 极值 */
        {RAM_FLAG_LOG, PC_R_LOG_DATA},           /* 读取日志数据 */
        {RAM_FLAG_TRANSRECORDER, PC_R_LO_FAUL},  /* 读低速故障录波数据 */
    };

#ifdef _FUXI_H2

        struct co_io send;
        
        memset(&send,0,sizeof(struct co_io));

        send.data[0] = 10 << 24; // offset in shm0
        send.data[1] = 16;       // length of data in shm0
        send.meta = 0;
        send.len = 16;
        send.data[2] = 0xBB66;  //数据头
        send.data[3] = 1;       //flag
        send.data[5] = 0x66BB;  //数据尾

        int retval = ioctl(_dev_co.fd_co, CO_IOCTL_SET_DATA, &send);
        if (retval)
        {
            DTULOG(DTU_ERROR, (char *)"ioctl 1 发送失败");
            return tasklist.size();
        }

#endif
    // 设置读任务
    for (auto &item : s_cmdmap) {
        if (m_readflag_buff[item.first] == 1) {
            tasklist.push_back(RAMTask(item.first, item.second));
            DTULOG(DTU_INFO, (char *)"读取标志:0x%02X, 读取命令:0x%02X", item.first, item.second);
        }
    }
#ifdef _FUXI_H2

        memset(&send,0,sizeof(struct co_io));

        send.data[0] = 10 << 24; // offset in shm0
        send.data[1] = 16;       // length of data in shm0
        send.meta = 0;
        send.len = 16;
        send.data[2] = 0xBB66;  //数据头
        send.data[3] = 0;       //flag
        send.data[5] = 0x66BB;  //数据尾

        retval = ioctl(_dev_co.fd_co, CO_IOCTL_SET_DATA, &send);
        if (retval)
        {
            DTULOG(DTU_ERROR, (char *)"ioctl 2 发送失败");
            return tasklist.size();
        }
        else
        {
            return tasklist.size();
        }
#endif
    return tasklist.size();
}

//返回值=0 正常完成 -1 出现错误  1 有优先级高任务,自行暂停
int DSPComm::ProcessRAMReadTask(const RAMTask &taskinfo) {
    buffer result;
    auto ret = dsp_read_data(taskinfo._cmdcode, result, 500);
    if (DTU_SUCCESS != ret) {
        DTULOG(DTU_ERROR, (char *)"ProcessRAMReadTask任务[%d]执行失败 %d", taskinfo._taskid, ret);
        return ret;
    }
    if (result.size() != 0) {
        DTULOG(DTU_INFO, (char *)"ProcessRAMReadTask 0x%04X 读取数据长度:%u", taskinfo._cmdcode, result.size());
        add_data_to_queue(taskinfo._cmdcode, taskinfo._taskid, result);
    }
    return ret;
}

int DSPComm::dsp_handle() {
    //
    static std::map<uint16_t, uint16_t> ack = {
        {PC_R_SOE_INFO, TX_PC_SOE_INFO},       {PC_R_ALARM_INFO, TX_PC_ALARM_REP_INFO},
        {PC_R_OPER_INFO, TX_PC_OPER_REP_DATA}, {PC_R_LO_FAUL, TX_PC_LS_LB_DATA},
        {PC_R_COS_DATA, TX_PC_COS_DATA},       {PC_R_ZTLB_DATA, TX_PC_ZTLB_DATA},
    };
    buffer recvData;
    while (!_bStopDsp) {
        uint16_t flag = 0;
        auto cmdcode = get_data_from_queue(recvData, flag);
        if (recvData.size() == 0) {
            // 扫描标志位时间
            std::this_thread::sleep_for(std::chrono::milliseconds(DSP_SCAN_PERIOD));
            continue;
        }
        switch (cmdcode) {
        // GOOSE消息
        case PC_R_GOOSE_INFO: {
            sendtoGoose(recvData);
            break;
        }
        //突变遥测COS事件
        case PC_R_COS_DATA: {
            uint32_t length = recvData.size();
            if ((length % YC_COS_BURST_LENGTH) == 0) {
                DTU::dtuprotocol proto;
                proto._cmd = TX_PC_COS_DATA;
                proto._curLen = recvData.size();
                proto._totleLen = recvData.size();
                proto._data = recvData;
                //通知规约
                DTU::DTUNotifyMgr::instance().notify_101(proto);
                DTU::DTUNotifyMgr::instance().notify_104(proto);
                // 通知公共单元
                DTU::DTUNotifyMgr::instance().notify_public(proto);
                //通知MCU
                //DTU::DTUNotifyMgr::instance().notify_mcu(proto);
                //通知工具
                //DTU::DTUNotifyMgr::instance().notify_dtutools(proto);
            } else {
                DTULOG(DTU_ERROR, (char *)"突变遥测COS事件上送长度错误,当前数据长度%u", length);
            }
            break;
        }
        // 读取保护动作信息 暂态录波数据 业务录波数据
        case PC_R_PRO_ACT_INFO:
        case PC_R_LO_FAUL:
        case PC_R_ZTLB_DATA: {
            // 存储动作报告
            DTU::DSTORE::instance().add_achives_data(flag, recvData);
            break;
        }
        case PC_R_SOE_INFO:
        case PC_R_ALARM_INFO:
        case PC_R_OPER_INFO: {
            // SOE, 告警, 操作记录
            uint16_t reportid = DTU::DSTORE::instance().get_reportid_by_cmd(cmdcode);
            auto itemsize = DTU::DSTORE::instance().get_report_itemsize(reportid);
            uint32_t offset = 0;
            while ((offset + itemsize) <= recvData.size()) {
                DTU::dtuprotocol proto;
                DTU::DSTORE::instance().add_report_data(reportid,0,0,recvData.get(offset, itemsize));
                makenotify(ack[cmdcode], recvData.get(offset, itemsize), proto);
                if(cmdcode == PC_R_SOE_INFO)
                {
                    // 如果是SOE
                    // 通知规约
                    DTU::DTUNotifyMgr::instance().notify_101(proto);
                    DTU::DTUNotifyMgr::instance().notify_104(proto);
                    // 通知公共单元
                    DTU::DTUNotifyMgr::instance().notify_public(proto);
                }
                // 通知工具
                DTU::DTUNotifyMgr::instance().notify_dtutools(proto);
                offset += itemsize;
            }
            if (cmdcode == PC_R_SOE_INFO)
            {
                // 如果是SOE则生成SOE文件
                DTU::DRULESFILE::instance().form_soe_file();
            }
            break;
        }
        // 添加极值数据
        case PC_R_EXV_DATA: {
            DTU::DRULESFILE::instance().add_exv_data(recvData);
            break;
        }
        // 日志数据
        case PC_R_LOG_DATA: {
            DTU::DRULESFILE::instance().add_log_data(recvData);
            DTU::DRULESFILE::instance().form_log_file();
            break;
        }
        #if !_WIN32
        // 自检数据
        case PC_R_CHECK: {
            DTU::DSTORE::instance().save_selfcheck_data(recvData);
            break;
        }
        #endif
        default :
            break;
        }
    }

    return 0;
}

int DSPComm::ClearReadTask(int taskid) {
    // cout<<"清理数据任务标识"<<taskid<<endl;
    m_readflag_buff[taskid] = 0;
    return 0;
}

void DSPComm::add_data_to_queue(uint16_t cmd, uint16_t flag, const buffer &result) {
    std::lock_guard<std::mutex> lock(_queue_lock);
    _handleQueue.push(std::make_tuple(cmd, flag, result));
}

uint16_t DSPComm::get_data_from_queue(buffer &result, uint16_t &flag) {
    result.remove();
    std::lock_guard<std::mutex> lock(_queue_lock);
    if (_handleQueue.empty()) {
        return 0;
    }
    auto res = _handleQueue.front();
    _handleQueue.pop();
    result = std::get<2>(res);
    flag = std::get<1>(res);
    return std::get<0>(res);
}

DTU::buffer DSPComm::init_adjust_param()
{
    DTU::buffer result;
    for(int m=0;m<3;m++) 
    {
        if (DTU_SUCCESS != dsp_read_data(PC_R_ADJ_FIX, result, 50))
        {
            DTULOG(DTU_WARN,  "初始化读取整定参数失败! 重试:%d/3",m+1);
            if(m != 2)
                continue;// 未达到次数
            // 达到次数直接返回
            DTULOG(DTU_ERROR,  "初始化读取整定参数失败! 加载配置文件中数据");
            return result;
        }
        else
            break;
    }
    return result;
}

void DSPComm::update_connect(uint16_t state)
{
    this->ConnectState = state;
}

void DSPComm::sendtoGoose(DTU::buffer &recv,int retrytime,int retrycount)
{
    struct GOOSE_RECV_INFO {
        uint32_t sec;
        uint32_t ms;
        uint8_t tx_data[8];
    };
    
    int count = recv.size() / sizeof(GOOSE_RECV_INFO);
    DTULOG(DTU_INFO,"GOOSE包长度%u,共[%d]帧",recv.size(),count);
    std::vector<GooseData> sendAttr;
    for(int i = 0;i < count;i++)
    {
        GOOSE_RECV_INFO goosevector;
        memcpy(&goosevector,recv.data()+(i*sizeof(GOOSE_RECV_INFO)),sizeof(GOOSE_RECV_INFO));
        GooseData send;
        send.emplace_back(goosevector.tx_data[1]);
        send.emplace_back(goosevector.tx_data[3]);
        send.emplace_back(goosevector.tx_data[5]);
        send.emplace_back(goosevector.tx_data[7]);
        DTULOG(DTU_DEBUG, "DSP 节点故障[%d]故障隔离成功[%d]开关拒跳[%d]过流闭锁[%d]", goosevector.tx_data[1], goosevector.tx_data[3], goosevector.tx_data[5], goosevector.tx_data[7]);
        sendAttr.emplace_back(send);
    }

    uint8_t zcount = 1;

    try
    {
        for(auto &item : sendAttr) 
        {
            dtuGoosePublisherMgr::instance().publish(item);
            DTULOG(DTU_INFO,"发送第%u帧GOOSE信息",zcount);
            zcount++;
        }
    }
    catch(std::exception &e)
    {
        DTULOG(DTU_ERROR,"DSPComm::sendtoGoose()发生错误:%s",e.what());
    }
}