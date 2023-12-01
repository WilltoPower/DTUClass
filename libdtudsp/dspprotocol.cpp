/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dspprotocol.h
  *Description: 
    DSP通信协议
  *History: 
    1, 创建, wangjs, 2021-7-30
    2, 加入重启标志, wangjs, 2021-8-16 
**********************************************************************************/
#include <dtucmdcode.h>
#include <dtuerror.h>
#include "dspprotocol.h"
using namespace DTU;
void DSPProtocol::MakeBuffer(buffer& data)
{
    data.remove();
    data.resize(_headLen);
    uint32_t offset = 0;
    data.set(offset, (char*)&_crc32, sizeof(_crc32));
    offset += sizeof(_crc32);
    data.set(offset, (char*)&_cmd, sizeof(_cmd));
    offset += sizeof(_cmd);
    data.set(offset, (char*)&_totleLen, sizeof(_totleLen));
    offset += sizeof(_totleLen);
    data.set(offset, (char*)&_curLen, sizeof(_curLen));
    offset += sizeof(_curLen);
    data.set(offset, (char*)&_blockno, sizeof(_blockno));
    offset += sizeof(_blockno);
    data.set(offset,(char*)&_isReboot, sizeof(_isReboot));
    offset += sizeof(_isReboot);
    // 添加数据
    data.append(_data);
}
// 从缓存解析数据
void DSPProtocol::ParseBuffer(const buffer &data)
{
    DTU_USER()
    if (data.size() < _headLen){
        DTU_THROW((char*)"DSPProtocal parse invaild data length %u", data.size());
    }
    uint32_t offset = 0;
    _crc32 = data.get(offset, sizeof(_crc32)).value<uint32_t>();
    offset += sizeof(_crc32);
    _cmd = data.get(offset, sizeof(_cmd)).value<uint16_t>();
    offset += sizeof(_cmd);
    _totleLen = data.get(offset, sizeof(_totleLen)).value<uint32_t>();
    offset += sizeof(_totleLen);
    _curLen = data.get(offset, sizeof(_curLen)).value<uint32_t>();
    offset += sizeof(_curLen);
    _blockno = data.get(offset, sizeof(_blockno)).value<uint16_t>();
    offset += sizeof(_blockno);
    //////////////////////////
    if (_curLen != 0)
    {
        _data.remove();
        _data = data.get(_headLen, _curLen);
    }
}
// 设置数据
void DSPProtocol::SetData(const buffer &data)
{
    _data = data;
}
// 获取数据部分
const DTU::buffer &DSPProtocol::GetData() const
{
    return _data;
}
bool DSPProtocol::SelfCheck(uint16_t type)
{
    if (type == DSP_CONTROL ||
        type == DSP_WRITE)
    {
        if (_totleLen == 0 &&
            _curLen == 0 &&
            _blockno == 0xFFFE &&
            _cmd == TX_PC_ACK)
        {
            return true;
        }
        // 暂时不做CRC处理
        if (_cmd == TX_PC_CRC_BAD)
        {
            return true;
        }
    }
    else if (type == DSP_READ)
    {
        return true;
    }
    return false;
}