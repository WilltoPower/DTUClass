/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtutransrecorder.cpp
  *Description: 
    暂态录波文件
  *History: 
    1, 创建, wangjs, 2021-8-25
**********************************************************************************/
#include "dturecorder.h"
#include <dtuerror.h>
#include <dtucmdcode.h>
#include <fstream>
#include <dtucommon.h>
#include <dtulog.h>

using namespace DTU;
#define SETVALUE(v, offset, src) \
    memcpy(&v, src+offset, sizeof(v)); \
    offset+=sizeof(v); 

static std::map<uint16_t, std::tuple<std::string, uint32_t, std::string>> _adjust_mnl_attr =
{
    {0, {"通道1 A电压(板1)", 0, "V"}},{1, {"通道1 B电压(板1)", 1, "V"}},{2, {"通道1 C电压(板1)", 2, "V"}},{3, {"通道1 0序电压(板1)", 3, "V"}},
    {4, {"通道5 A电流(板1)", 4, "A"}},{5, {"通道5 B电流(板1)", 5, "A"}},{6, {"通道5 B电流(板1)", 6, "A"}},{7, {"通道5 0序电流(板1)", 7, "A"}},
    {8, {"I0x", 8, "A"}},
    {9, {"通道1 A电压(板1)", 9, "V"}},{10, {"通道1 B电压(板1)", 10, "V"}},{11, {"通道1 C电压(板1)", 11, "V"}},{12, {"通道1 0序电压(板1)", 12, "V"}},
    {13, {"通道5 A电流(板1)", 13, "A"}},{14, {"通道5 B电流(板1)", 14, "A"}},{15, {"通道5 B电流(板1)", 15, "A"}},{16, {"通道5 0序电流(板1)", 16, "A"}},
};

static std::map<uint16_t, std::tuple<std::string, uint32_t, std::string>> _mnl_attr = 
{
    {0, {"通道1电压(板1)", 0, "V"}},{1, {"通道2电压(板1)", 1,"V"}},{2, {"通道3电压(板1)", 2,"V"}},{3, {"通道4电压(板1)", 3, "V"}},
    {4, {"通道5电流(板1)", 4, "A"}},{5, {"通道6电流(板1)", 5, "A"}},{6, {"通道7电流(板1)", 6, "A"}},{7, {"通道8电流(板1)", 7, "A"}},
    {8, {"I0x", 8, "A"}},{9, {"GU0", 9,"V"}},{10, {"GI0", 10,"A"}},
    {11, {"通道1电压(板2)", 11, "V"}},{12, {"通道2电压(板2)", 12, "V"}},{13, {"通道3电压(板2)", 13, "V"}},{14, {"通道4电压(板2)", 14,"V"}},
    {15, {"通道5电流(板2)", 15,"A"}},{16, {"通道6电流(板2)", 16,"A"}},{17, {"通道7电流(板2)", 17,"A"}},{18, {"通道8电流(板2)", 18,"A"}},
    {19, {"Fm", 19, "Hz"}},{20, {"Fx", 20,"Hz"}}
};

static std::map<uint16_t, std::string> _digital_attr = 
{
    {1, "断路器分位"},{2, "断路器合位"},{3, "刀闸分位"},{4, "刀闸合位"},
    {5, "重合闸投入"},{6, "检修状态投入"},{7, "隔离开关位置"},{8, "低气压告警信号"},
    {9, "低气压闭锁信号"},{10, "远方/就地"},{11, "弹簧未储能"},{12, "温湿度越限信号"},
    {13, "停用同期合闸功能"},{14, "停用自动解列功能"},{15, "常规保护投入"},{16, "就地化FA投入"},
    {17, "分布式FA投入"},{18, "禁止所有保护功能"},{19, "复归"},{20, "电池活化"},
    {21, "输入来电指示"},{22, "手动跳闸"},{23, "手动合闸"},
    
    {28, "电源故障"},{29,"电池欠压"},
    
    {33, "电源预控返校"},{34, "遥控分闸返校"},{35, "遥控合闸返校"},{36, "保护分闸返校"},
    {37, "保护合闸返校"},

    {49,"故障总"},{50,"A相故障"},{51,"B相故障"},{52,"C相故障"},{53,"整组启动"},
    {54,"整组复归"},{55,"跳闸失败"},{56,"Ua超限"},{57,"Ub超限"},{58,"Uc超限"},{59,"Ia超限"},{60,"Ib超限"},{61,"Ic超限"},

    {65,"过流I段动作"},{66,"过流I段告警"},{67,"过流I段后加速动作"},
    {81,"过流II段动作"},{82,"过流II段告警"},{83,"过流II段后加速动作"},
    {97,"过流III段动作"},{98,"过流III段告警"},{99,"过流III段后加速动作"},
    {113,"零序过流I段动作"},{114,"零序过流I段告警"},{115,"零序过流I段后加速动作"},
    {129,"零序过流II段动作"},{130,"零序过流II段告警"},{131,"零序过流II段后加速动作"},
    {145,"零序过压保护动作"},{146,"零序过压保护告警"},{147,"零序过压保护后加速动作"},

    {161,"过负荷动作"},{169,"重载保护动作"},
    {177,"低电压解列动作"},{178,"电压过低解列动作"},{185,"高压电解列动作"},{186,"电压过高解列动作"},

    {193,"高频解列动作"},{201,"低频解列动作"},{202,"频率过低解列动作"},

    {209,"保护启动重合闸"},{210,"不对应启动重合闸"},{211,"一次重合闸动作"},{212,"二次重合闸动作"},
    {213,"三次重合闸动作"},{214,"重合闸闭锁"},{215,"重合闸充电完成"},{216,"重合闸放电"},
    {217,"重合闸复归"},{218,"重合闸超时"},

    {225,"涌流闭锁"},{226,"大电流闭锁重合闸"},{233,"暂态接地动作"},{234,"暂态接地告警"},
    {235,"暂态接地后加速"}, {236, "暂态接地启动"},

    {241,"PT断线"},{242,"控制回路断线"},{249,"遥控分闸"},{250,"遥控合闸"},
    {251,"合后位信号"},{251,"遥控预设"},{252,"遥控取消"},

    {257,"与主站通信异常"},{265,"与间隔单元1通信异常"},{266,"与间隔单元2通信异常"},
    {267,"与间隔单元3通信异常"},{268,"与间隔单元4通信异常"},{269,"与间隔单元5通信异常"},
    {270,"与间隔单元6通信异常"},

    {274,"分段模式"},{275,"联络模式"},{276,"电源侧X闭锁"},{277,"负荷侧X闭锁"},
    {278,"电源侧Y闭锁"},{279,"负荷侧Y闭锁"},{280,"电源侧残压闭锁"},{281,"负荷侧残压闭锁"},

    {282,"电源侧得电延时合闸"},{283,"负荷侧得电延时合闸"},
    {284,"双侧失电延时分闸"},{285,"合到故障快速分闸"},
    {286,"合到零压闭锁合闸"},{287,"单侧失压延时合闸"},
    {288,"联络开关双侧失电分闸"},{289,"联络开关电源残压闭锁"},
    {290,"双侧有压闭锁合闸"},{291,"PT断线闭锁合闸"},
    {292,"多次失电分闸闭锁合闸"},{293,"遥分闭锁得电合闸"},
    {294,"非遮断闭锁分闸"},{295,"联络开关负荷残压闭锁"},
    {296,"FA相间过流告警"},{297,"FA接地告警"},
    {298,"合到零压闭锁合闸"},{299,"合到故障闭锁合闸"},
    {300,"合闸成功闭锁分闸"},
};

void DTransHeader::parse(const char* data, uint32_t size)
{
    DTU_USER()
    if (size < _header_length){
        DTU_THROW((char*)"DTransHeader::parse 数据长度:%u小于头部长度:%u",
            size, _header_length);
    }
    uint32_t offset = 0;
    SETVALUE(_fault_mircosec, offset, data);
    SETVALUE(_fault_seconds, offset, data);
    SETVALUE(_leap_info, offset, data);
    SETVALUE(_digital_num, offset, data);
    SETVALUE(_start_flag, offset, data);
    SETVALUE(_frame_length, offset, data);
    SETVALUE(_frame_no, offset, data);
    SETVALUE(_samples, offset, data);
    SETVALUE(_fault_type, offset, data);
}

void DTransSamples::parse(const char* data, uint32_t size)
{
    DTU_USER()
    if (size < _samples_length){
        DTU_THROW((char*)"DTransSamples::parse 数据长度:%u小于采样点长度:%u",
            size, _samples_length);
    }
    _samples_data.remove();
    _samples_data.append(data, size);
}
void DWorkSamples::parse(const char* data, uint32_t size)
{
    DTU_USER()
    if (size < _samples_length){
        DTU_THROW((char*)"DWorkSamples::parse 数据长度:%u小于采样点长度:%u",
            size, _samples_length);
    }
    _samples_data.remove();
    _samples_data.append(data, size);
}
void DAdjustSamples::parse(const char* data, uint32_t size)
{
    DTU_USER()
    if (size < _samples_length){
        DTU_THROW((char*)"DAdjustSamples::parse 数据长度:%u小于采样点长度:%u",
            size, _samples_length);
    }
    _samples_data.remove();
    _samples_data.append(data, size);
}
int16_t DAdjustSamples::get_raw_value(uint32_t chNo)
{
    THROW_RUNTIME_ERROR_IF(chNo>CHANNEL_NO_MAX, "DAdjustSamples:通道号大于最大模拟量通道号");
    THROW_RUNTIME_ERROR_IF(chNo<CHANNEL_NO_MIN, "DAdjustSamples:通道号小于最小模拟量通道号");
    return _samples_data.get((chNo-1)*sizeof(int16_t),sizeof(int16_t)).value<int16_t>();
}
std::vector<int16_t> DAdjustCycle::get_raw_value(uint32_t chNo)
{
    THROW_RUNTIME_ERROR_IF(chNo>CHANNEL_NO_MAX, "DAdjustCycle:通道号大于最大模拟量通道号");
    THROW_RUNTIME_ERROR_IF(chNo<CHANNEL_NO_MIN, "DAdjustCycle:通道号小于最小模拟量通道号");
    std::vector<int16_t> tempData;
    uint16_t data;
    for(auto& point:_samples)
    {
        data = point.get_raw_value(chNo);
        tempData.push_back(data);
    }
    return tempData;
}
std::vector<int16_t> DAdjustRcd::get_raw_value(uint32_t chNo)
{
    // 加入判断
    THROW_RUNTIME_ERROR_IF(chNo>CHANNEL_NO_MAX, "DAdjustRcd:通道号大于最大模拟量通道号");
    THROW_RUNTIME_ERROR_IF(chNo<CHANNEL_NO_MIN, "DAdjustRcd:通道号小于最小模拟量通道号");
    std::vector<int16_t> tempData;
    for(auto& cyc:_cycles)
    {
        std::vector<int16_t> data = cyc.get_raw_value(chNo);
        tempData.reserve(tempData.size() + data.size());
        tempData.insert(tempData.end(),data.begin(),data.end());
    }
    return tempData;
}

const SAMPLE_MNL_ATTR& DTransSamples::get_mnl_attr()
{
    return _mnl_attr;
}
const SAMPLE_DIG_ATTR& DTransSamples::get_dig_attr()
{
    return _digital_attr;
}
const SAMPLE_MNL_ATTR& DWorkSamples::get_mnl_attr()
{
    return _mnl_attr;
}
const SAMPLE_DIG_ATTR& DWorkSamples::get_dig_attr()
{
    return _digital_attr;
}
/*
const SAMPLE_MNL_ATTR&  DAdjustSamples::get_mnl_attr()
{
	return _adjust_mnl_attr;
}
const SAMPLE_DIG_ATTR&  DAdjustSamples::get_dig_attr()
{
	return _digital_attr;
}
*/


bool DTransCycleHeader::parse(const char* data, uint32_t size)
{
    DTU_USER()
    if (size < _header_length){
        DTU_THROW((char*)"DTransCycleHeader::parse 数据长度:%u小于周波头长度:%u, 第%u个周波",
            size, _header_length, nCycNum);
    }
    // 长度判断
    uint16_t head = 0;
    memcpy(&head, data, sizeof(head));
    uint16_t end = 0;
    memcpy(&end, data+62, sizeof(end));
    if (head != 0x55AA || end != 0xAA55){
		// for (int i = 0; i < size; i++)
		// {
        //     if((i%16)==0)putchar(10);
		// 	printf("%02X ", data[i]);
		// }
		// putchar(10);
        DTULOG(DTU_ERROR,"DTransCycleHeader::parse 头部存在错误, 第%u个周波 HEAD = [0x%04X] END = [0x%04X] size %u", nCycNum, head, end, size);
        return false;
    }

    uint32_t offset = 2;
    SETVALUE(_freq, offset, data);
    SETVALUE(_mircosec, offset, data);
    SETVALUE(_seconds, offset, data);
    SETVALUE(_leap_info, offset, data);
    SETVALUE(_time_status, offset, data);
    SETVALUE(_vol_num, offset, data);
    SETVALUE(_cur_num, offset, data);
    SETVALUE(_ki_num, offset, data);
    SETVALUE(_freq_num, offset, data);
    SETVALUE(_cur_1, offset, data);
    SETVALUE(_cur_2, offset, data);
    SETVALUE(_zt_ground, offset, data);
    offset+=sizeof(_save);
    SETVALUE(_board_temp, offset, data);
    SETVALUE(_board_vol_1, offset, data);
    SETVALUE(_board_vol_2, offset, data);
    offset+=22;
    SETVALUE(_test_counter, offset, data);

    return true;
}

void DTransCycle::parse(const char* data, uint32_t size)
{
    //
    _header.nCycNum = nCycNum;
    _header.parse(data, _header._header_length);
    uint32_t offset = DTransCycleHeader::_header_length;
    while(offset < size)
    {
        DTransSamples samples;
        samples.parse(data+offset, samples._samples_length);
        _samples.emplace_back(std::move(samples));
        offset+=samples._samples_length;
    }
}
bool DWorkCycle::parse(const char* data, uint32_t size)
{
    //
    if(!_header.parse(data, _header._header_length))
    {
        return false;
    }
    uint32_t offset = DWorkCycleHeader::_header_length;
    while(offset < size)
    {
        DWorkSamples samples;
        samples.parse(data+offset, samples._samples_length);
        _samples.emplace_back(std::move(samples));
        offset+=samples._samples_length;
    }
    return true;
}
void DAdjustCycle::parse(const char* data, uint32_t size)
{
    //
    _header.parse(data, DAdjustCycleHeader::_header_length);
    uint32_t offset = DAdjustCycleHeader::_header_length;
    while(offset < size)
    {
        DAdjustSamples samples;
        samples.parse(data+offset, samples._samples_length);
        _samples.emplace_back(std::move(samples));
        offset+=samples._samples_length;
    }
}

void DTransRcd::parse(const char* data, uint32_t size)
{
    _header.parse(data, DTransHeader::_header_length);
    uint32_t offset = DTransHeader::_header_length;
    // 单个周波长度
    uint32_t cycleLength = _header._samples * DTransSamples::_samples_length
         + DTransCycleHeader::_header_length;
    int nCount = 0;
    while(offset < size){
        DTransCycle cycle;
        cycle.nCycNum = nCount;
        cycle.parse(data+offset, cycleLength);
        _cycles.emplace_back(std::move(cycle));
        offset += cycleLength;
        nCount++;
    }
}

void DWorkRcd::parse(const char* data, uint32_t size)
{
    _header.parse(data, DTransHeader::_header_length);
    uint32_t offset = DTransHeader::_header_length;
    // 单个周波长度
    uint32_t cycleLength = _header._samples * DWorkSamples::_samples_length
         + DTransCycleHeader::_header_length;
    while(offset < size){
        DWorkCycle cycle;
        cycle.parse(data+offset, cycleLength);
        _cycles.emplace_back(std::move(cycle));
        offset += cycleLength;
    }
}
void DAdjustRcd::parse(const char* data, uint32_t size)
{
    uint32_t offset =0;
    // 单个周波长度
    uint32_t cycleLength = ((DTU::DAdjustSamples::_samples_length * 50 / 2) * 4 + 32) * 2;//字节
    _cycles.clear();
    while(offset < size){
        DAdjustCycle cycle;
        cycle.parse(data+offset, cycleLength);
        _cycles.emplace_back(std::move(cycle));
        offset += cycleLength;
    }
}