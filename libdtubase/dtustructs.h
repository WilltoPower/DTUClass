/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtustructs.h
  *Description: 
    通用功能定义
  *History: 
    1, 创建, wangjs, 2022-1-7
**********************************************************************************/
#ifndef _DTU_STRUCTS_H
#define _DTU_STRUCTS_H
#include "dtubuffer.h"
#include <bitset>

// 日志内容
#define CSLOG 0x01
// 报文内容
#define CSMSG 0x02

//
using FILEINFO = std::tuple<std::string, uint32_t, uint64_t, uint16_t>;
using FILELIST = std::vector<FILEINFO>;
//
class settingItem{
public:
    settingItem(){}
    settingItem(const uint16_t addr, char tag, const DTU::buffer& value);
    settingItem(const settingItem& src);
    settingItem(settingItem&& src);

    settingItem& operator=(const settingItem& src);
public:
    void parse_buff(const DTU::buffer& buff);

    uint32_t parse_buff(char* buff, uint32_t length);

    DTU::buffer make_buff();
public:
    // 信息体地址
    uint16_t _addr;
    // 数据类型 
    char _tag;
    //
    unsigned char _length;
    // 值
    DTU::buffer _value;

    const uint32_t settingItemHeadLen =  5;

};
class settingInfo{
public:
    // 后续状态
    int get_cont();
    void set_cont(bool stat);
    // 取消预置
    int get_cr();
    void set_cr(bool stat);
    // 固化/预置
    int get_se();
    void set_se(bool stat);
    /////////////////////////
    unsigned char get_tag();

    void parse(const DTU::buffer& src);
public:
    // 定值区号
    uint16_t _group;
    // 特征表示
    std::bitset<8> _tag;
    // 定值信息
    std::vector<settingItem> _settings;
};
#define ADJUST_RATIO     0 // 变比
#define ADJUST_INTERCEPT 1 // 截距
#define ADJUST_ZERO      2 // 零漂
#define ADJUST_ANGLE_COS 3 // 角度校正系数cos
#define ADJUST_ANGLE_SIN 4 // 角度校正系数sin

#define CALIBRATION_LENGTH 512 //整定结果长度

#define CHANNEL_NO_MAX 19 //通道号最大值
#define CHANNEL_NO_MIN 1  //通道号最小值

class DTUParamAdjust
{
public:
    // 初始化构造函数
    DTUParamAdjust();
    DTUParamAdjust(const DTU::buffer &src);

    // 从文件中加载
    void load();
    
    // 保存到文件
    void save();

    // 从buffer中解析
    void parse(const char* data, uint32_t size);

    // 按通道获取值 type:变比零漂等
    float GetValue(uint32_t channel, uint32_t type);
    
    // 按通道设置值
    void SetValue(uint32_t channel, const float& v, uint32_t type);

    // 按通道号插入值
    void insert(uint32_t channel, const float& v, uint32_t type);

    DTU::buffer &Adjdata();

    // =运算符重载
    DTUParamAdjust &operator=(DTUParamAdjust &src);
private:
    DTU::buffer _adjustData;
    const uint32_t _paramLen = CALIBRATION_LENGTH;
    uint32_t valueoffset(uint32_t type);
    const uint32_t _unitLen = (20 * sizeof(float));
};

// 规约传输结构体
    struct transerInfomation {
        bool isSequence = false;
        int COT = 0;
        int Ti  = 0;
        int elementNumber = 0;
        int payloadSize = 0;
        MSGPACK_DEFINE(isSequence, COT, Ti, elementNumber, payloadSize);
    };

    // struct ProtoTransObj {
    //     transerInfomation info;
    //     DTU::buffer data;
    //     MSGPACK_DEFINE(info, data);
    // };

    // struct ProtoTransObjAttr {
    //     std::vector<ProtoTransObj> attr;
    //     MSGPACK_DEFINE(attr);
    // };

#endif