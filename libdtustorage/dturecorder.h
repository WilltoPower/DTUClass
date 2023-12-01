/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturecorder.h
  *Description: 
    定义录波相关结构体和功能
  *History: 
    1, 创建, wangjs, 2021-8-25
    2, 修改文件名, wangjs, 2021-9-1
**********************************************************************************/
#ifndef _DTU_TRANS_RECORDER_H
#define _DTU_TRANS_RECORDER_H
#include <dtubuffer.h>
#include <vector>
#include <map>
#include <tuple>
#include <dtuerror.h>
#include "json/json.h"

namespace DTU
{
    using SAMPLE_MNL_ATTR = std::map<uint16_t,std::tuple<std::string, uint32_t, std::string>>;
    using SAMPLE_DIG_ATTR = std::map<uint16_t,std::string>;
    // 暂态录波文件头
    class DTransHeader
    {
    public:
        // 解析
        void parse(const char* data, uint32_t size);
    public:
        // 故障时间微妙
        uint32_t _fault_mircosec = 0;
        // 故障时间秒
        uint32_t _fault_seconds = 0;
        // 闰秒信息
        uint16_t _leap_info = 0;
        // 开入量通道数
        uint16_t _digital_num = 0;
        // 启动标志
        uint32_t _start_flag = 0;
        // 文件帧长度
        uint32_t _frame_length = 0;
        // 文件帧号
        uint16_t _frame_no = 0;
        // 采样点/周波
        uint16_t _samples = 0;
        // 故障文件类型
        uint16_t _fault_type = 0;
        // 保留
        const uint32_t _save_length = 102;
        // 头长度128字节
        static const uint32_t _header_length = 128;
    };

    using DWorkHeader = DTransHeader;
    using DAdjustHeader = DTransHeader;

    // 暂态采样点
    class DTransSamples
    {
    public:
        void parse(const char* data, uint32_t size);
        const SAMPLE_MNL_ATTR& get_mnl_attr();
        const SAMPLE_DIG_ATTR& get_dig_attr();
    public:
        // 单点长度
        static const uint32_t _samples_length = 80;
        buffer _samples_data;
    };

    // 业务采样点
    class DWorkSamples
    {
    public:
        void parse(const char* data, uint32_t size);
        const SAMPLE_MNL_ATTR& get_mnl_attr();
        const SAMPLE_DIG_ATTR& get_dig_attr();
    public:
        // 单点长度
        static const uint32_t _samples_length = 44;
        buffer _samples_data;
    };

    // 整定采样点
    class DAdjustSamples
    {
    public:
        void parse(const char* data, uint32_t size);
        int16_t get_raw_value(uint32_t chNo);
        //
    public:
        // 单点长度
        static const uint32_t _samples_length = 38;
        buffer _samples_data;
    };

    // 暂态周波头
    class DTransCycleHeader
    {
    public:
        bool parse(const char* data, uint32_t size);
    public:
        // 采样频率
        uint16_t _freq = 0;
        // 采样点0时刻起的微妙数
        uint32_t _mircosec = 0;
        // 采样点0时刻起的秒数
        uint32_t _seconds = 0;
        // 闰秒信息
        uint16_t _leap_info = 0;
        // 对时状态
        uint16_t _time_status = 0;
        // 电压通道数
        uint16_t _vol_num = 0;
        // 电流通道数
        uint16_t _cur_num = 0;
        // 开入量通道数
        uint16_t _ki_num = 0;
        // 频率字数
        uint16_t _freq_num = 0;
        // 直流1
        uint16_t _cur_1 = 0;
        // 直流2
        uint16_t _cur_2 = 0;
        // 暂态接地通道数
        uint16_t _zt_ground = 0;
        //保留
        uint16_t _save = 0;
        // 板卡温度
        uint16_t _board_temp = 0;
        // 采集板1电源电压
        uint16_t _board_vol_1 = 0;
        // 采集板2电源电压
        uint16_t _board_vol_2 = 0;
        // 测试计数器
        uint16_t _test_counter = 0;
        //
        static const uint32_t _header_length = 64;

        uint32_t nCycNum = 0;
    };

    using DWorkCycleHeader   = DTransCycleHeader;
    using DAdjustCycleHeader = DTransCycleHeader;

    // 暂态周波数据
    class DTransCycle
    {
    public:
        void parse(const char* data, uint32_t size);
        // 周波头
        DTransCycleHeader _header;
        // 单个周波采样点
        std::vector<DTransSamples> _samples;

        int nCycNum = 0;
    };
    ///////////////////////////////////////
    // 业务周波
    class DWorkCycle
    {
    public:
        bool parse(const char* data, uint32_t size);
        // 周波头
        DWorkCycleHeader _header;
        // 单个周波采样点
        std::vector<DWorkSamples> _samples;
    };
    ///////////////////////////////////////
    // 整定周波
    class DAdjustCycle
    {
    public:
        void parse(const char* data, uint32_t size);
        std::vector<int16_t> get_raw_value(uint32_t chNo);
        // 周波头
        DAdjustCycleHeader _header;
        // 单个周波采样点
        std::vector<DAdjustSamples> _samples;
    };

    ///////////////////////////////////////
    // 暂态录波
    class DTransRcd
    {
    public: 
        void parse(const char* data, uint32_t size);
        // 头部
        DTransHeader _header;
        // 周波数据
        std::vector<DTransCycle> _cycles;
    };
    ///////////////////////////////////////
    // 业务录波
    class DWorkRcd
    {
    public: 
        void parse(const char* data, uint32_t size);
        // 头部
        DTransHeader _header;
        // 周波数据
        std::vector<DWorkCycle> _cycles;
    };
    ///////////////////////////////////////
    // 整定录波
    class DAdjustRcd
    {
    public:
        void parse(const char* data, uint32_t size);
        // 周波数据
        std::vector<DAdjustCycle> _cycles;
        std::vector<int16_t> get_raw_value(uint32_t chNo);
        // 起始时间
        uint64_t _beginTime = 0;
        // 故障时间
        uint64_t _faultTime = 0;
    };
};
#endif