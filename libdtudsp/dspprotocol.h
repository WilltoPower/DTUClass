/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dspprotocol.h
  *Description: 
    DSP通信协议
  *History: 
    1, 创建, wangjs, 2021-7-30
    2, 加入重启标志, wangjs, 2021-8-16 
**********************************************************************************/
#ifndef _DSP_PROTOCAL_H
#define _DSP_PROTOCAL_H
#include <dtubuffer.h>
namespace DTU
{
    class DSPProtocol
    {
    public:
        enum{DSP_CONTROL=0, DSP_WRITE, DSP_READ};
        DSPProtocol()
        {
        }
        DSPProtocol(uint16_t cmd, uint32_t tLen, uint32_t cLen, uint32_t blockno, uint16_t reboot = 1)
            : _cmd(cmd), _totleLen(tLen), _curLen(cLen), _blockno(blockno), _isReboot(reboot)
        {
        }
        // 构建缓存
        void MakeBuffer(DTU::buffer& data);
        // 从缓存解析数据
        void ParseBuffer(const DTU::buffer &data);
        // 设置数据
        void SetData(const DTU::buffer &data);
        // 获取数据部分
        const DTU::buffer &GetData() const;
        // 自检
        bool SelfCheck(uint16_t type);
    public:
        uint64_t _id = 0;
        // 校验码
        uint32_t _crc32 = 0;
        // 命令字
        uint16_t _cmd = 0;
        // 总长度
        uint32_t _totleLen = 0;
        // 当前长度
        uint32_t _curLen = 0;
        // 块号
        uint16_t _blockno = 0;
        //
        uint16_t _type = 0;
        // 
        uint16_t _isReboot = 1;
    private:
        // 保留区字节数
        const uint16_t _saveLen = 16;
        // 当前数据头固定34字节
        const uint32_t _headLen = 34;
        // 数据区
        DTU::buffer _data;
        // 用于接收数据
        DTU::buffer _totleBuff;
    };
}; // namespace PROTOCAL
#endif