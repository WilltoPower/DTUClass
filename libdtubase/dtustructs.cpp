#include "dtustructs.h"
#ifdef _WIN32
#pragma execution_character_set("utf-8")
#include "dtucommon.h"
#else
#include <dtucommon.h>
#endif
#include <dtuerror.h>
#include <pugixml/pugixml.hpp>
#include "dtulog.h"
#include "dtusystemconfig.h"

using namespace DTU;

#define IOALENGTH 2

settingItem::settingItem(const uint16_t addr, char tag, const DTU::buffer& value)
:_addr(addr),_tag(tag),_value(value){
    _length = value.size();
}

settingItem::settingItem(const settingItem& src)
:_addr(src._addr),_tag(src._tag), _length(src._length), _value(src._value)
{
}
settingItem::settingItem(settingItem&& src)
:_addr(src._addr),_tag(src._tag),_length(src._length),_value(src._value)
{
}
settingItem& settingItem::operator=(const settingItem& src)
{
    if (&src == this){
        return *this;
    }
    _addr = src._addr;
    _tag = src._tag;
    _length = src._length;
    _value = src._value;

    return *this;
}
void settingItem::parse_buff(const DTU::buffer& buff)
{
    if (buff.size() < settingItemHeadLen){
        return;
    }
    uint32_t offset = 0;
    _addr = buff.get(offset, sizeof(_addr)).value<uint16_t>();
    offset+=3;
    _tag = buff.get(offset, sizeof(_tag)).value<char>();
    offset+=1;
    _length = buff.get(offset, sizeof(_length)).value<char>();
    offset+=1;
    if (_length != 0 && (_length+settingItemHeadLen)<buff.size())
    {
        _value = buff.get(offset, _length);
    }
}

uint32_t settingItem::parse_buff(char* buff, uint32_t length)
{
    if (length < settingItemHeadLen){
        return 0;
    }
    uint32_t offset = 0;
    memcpy(&_addr, buff+offset, sizeof(uint16_t));
    // offset+=IOALENGTH;
    offset+= DTUCFG::DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA;
    memcpy(&_tag, buff+offset, sizeof(_tag));
    offset+=1;
    memcpy(&_length, buff+offset, sizeof(_length));
    offset+=1;
    if (_length != 0 && (_length + settingItemHeadLen) <= length)
    {
       _value = DTU::buffer(buff + settingItemHeadLen, _length);
       //_value.dump(0,_length);
    }
    offset += _length;
    
    return offset;
}

DTU::buffer settingItem::make_buff()
{
    DTU::buffer result;
    result.resize(settingItemHeadLen+_length);
    uint32_t offset = 0;
    result.set(offset, (char*)&_addr, sizeof(_addr));
    offset+=3;
    result.set(offset, (char*)&_tag, sizeof(_tag));
    offset+=1;
    result.set(offset, (char*)&_length, sizeof(_length));
    offset+=1;
    if (_length != 0)
    {
        result.set(offset, _value);
    }
	return result;
}
int settingInfo::get_cont(){
    return _tag[0];
}
void settingInfo::set_cont(bool stat)
{
    _tag.set(0, stat);
}
// 取消预置
int settingInfo::get_cr()
{
    return _tag[6];    
}
void settingInfo::set_cr(bool stat)
{
    _tag.set(6, stat);
}
// 固化/预置
int settingInfo::get_se()
{
    return _tag[7];
}
void settingInfo::set_se(bool stat)
{
    _tag.set(7, stat);
}
unsigned char settingInfo::get_tag()
{
    unsigned char tag = 0;
    uint32_t nbyte = 0;

    bitset_2_array<8>(_tag, 8, (char*)&tag, nbyte);

    return tag;
}

void settingInfo::parse(const DTU::buffer& src)
{
    if (src.size() < 3){
        return;
    }
    uint32_t offset = 0;
    _group = src.get(offset, sizeof(uint16_t)).value<uint16_t>();
    offset += sizeof(uint16_t);
    char tag = src.get(offset, sizeof(char)).value<char>();
    uint32_t nbyte = 0;
    array_2_bitset<8>(&tag, 1, _tag, nbyte);
    offset += sizeof(uint8_t);
    //
    uint32_t length = src.size() - offset;
    char* begin = const_cast<char*>(src.const_data()) + offset;

    offset = 0;
    while(offset < src.size() - 3)
    {
        settingItem item;
        uint32_t size = item.parse_buff(begin + offset, length);
        offset += size;
        length -= size;
        _settings.emplace_back(std::move(item));
    }
}

DTUParamAdjust::DTUParamAdjust()
{
	_adjustData.resize(CALIBRATION_LENGTH);
}

DTUParamAdjust::DTUParamAdjust(const DTU::buffer &src)
{
    _adjustData.resize(CALIBRATION_LENGTH);
    _adjustData = src;
}

void DTUParamAdjust::parse(const char* data, uint32_t size)
{
    THROW_RUNTIME_ERROR_IF(size != _paramLen, "DTUParamAdjust parse 错误的参数长度");
	_adjustData.remove();
	_adjustData.append(data, size);
}

uint32_t DTUParamAdjust::valueoffset(uint32_t type)
{
    static std::unordered_map<uint32_t, uint32_t> s_offset = {
        {ADJUST_RATIO    , 0*_unitLen},
        {ADJUST_INTERCEPT, 1*_unitLen},//_unitLen = (20*sizeof(float))
        {ADJUST_ZERO     , 2*_unitLen},
        {ADJUST_ANGLE_COS, 3*_unitLen},
        {ADJUST_ANGLE_SIN, 4*_unitLen},
    };
    auto ita = s_offset.find(type);
    THROW_RUNTIME_ERROR_IF(ita == s_offset.end(), "DTUParamAdjust GetValue 错误的值类型");
    return ita->second;
}

void DTUParamAdjust::load()
{
	DTU_USER()
#ifdef _WIN32
	std::string fileName = get_exec_dir() + "\\config\\adj.xml";
#else
    std::string fileName = get_exec_dir() + "/config/adj.xml";
#endif
    DTULOG(DTU_INFO,"初始化整定定值:%s", fileName.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(fileName.c_str());
    if (res.status != pugi::xml_parse_status::status_ok){
        DTU_THROW((char*)"加载整定参数文件%s错误 %s", fileName.c_str(), res.description());
    }

    pugi::xml_node adj = doc.child("adjust");
    auto ita = adj.begin();
    while(ita != adj.end())
    {
        uint32_t no = ita->attribute("no").as_uint();
        float ratio = ita->attribute("ratio").as_float();
        float intercept = ita->attribute("intercept").as_float();
        float zero = ita->attribute("zero").as_float();
        float angle = ita->attribute("angle").as_float();
        SetValue(no, ratio, ADJUST_RATIO);
		SetValue(no, intercept, ADJUST_INTERCEPT);
		SetValue(no, zero, ADJUST_ZERO);
		SetValue(no, cos(angle), ADJUST_ANGLE_COS);
		SetValue(no, sin(angle), ADJUST_ANGLE_SIN);

        ita++;
    }
}

void DTUParamAdjust::save()
{
	DTULOG(DTU_INFO,(char*)"保存整定参数");
    pugi::xml_document root;
    pugi::xml_node adj = root.append_child("adjust");
    for(int i=0;i<CHANNEL_NO_MAX;i++){
        pugi::xml_node channel = adj.append_child("channel");
        channel.append_attribute("no") = i+1;
        channel.append_attribute("ratio") = GetValue(i+1, ADJUST_RATIO);
        channel.append_attribute("intercept") = GetValue(i+1, ADJUST_INTERCEPT);
        channel.append_attribute("zero") = GetValue(i+1, ADJUST_ZERO);
        channel.append_attribute("angle") = acos(GetValue(i+1, ADJUST_ANGLE_COS));
    }
    #ifdef _WIN32
    std::string fileName = get_exec_dir() + "\\config\\adj.xml";
    #else
    std::string fileName = get_exec_dir() + "/config/adj.xml";
    #endif
    if (!root.save_file(fileName.c_str())){
        DTULOG(DTU_ERROR, (char*)"保存整定参数失败");
    }
}

float DTUParamAdjust::GetValue(uint32_t channel, uint32_t type)
{
    float value = 0.0f;
    THROW_RUNTIME_ERROR_IF(channel>CHANNEL_NO_MAX, "DTUParamAdjust GetValue 错误的通道号");
    THROW_RUNTIME_ERROR_IF(channel<CHANNEL_NO_MIN, "DTUParamAdjust GetValue 错误的通道号");
	//直流通道
	if (channel > 17)
	{/* 直流通道数据放在11 12里面 */
		channel = channel - 7;
	}
	else
	{
		//第二路交流通道
		if (channel > 9)
		{/*通道10 11 12为保留通道*/
			channel += 3;
		}
	}
    value = _adjustData.get((channel-1)*sizeof(float)+valueoffset(type), sizeof(float)).value<float>();
    return value;
}
void DTUParamAdjust::SetValue(uint32_t channel, const float& v, uint32_t type)
{
    THROW_RUNTIME_ERROR_IF(channel>CHANNEL_NO_MAX, "DTUParamAdjust SetValue 错误的通道号");
    THROW_RUNTIME_ERROR_IF(channel<CHANNEL_NO_MIN, "DTUParamAdjust GetValue 错误的通道号");
	//直流通道 
	if (channel > 17)
	{/* 直流通道数据放在11 12里面 */
		channel = channel - 7;
	}
	else
	{
		if (channel > 9)
		{/*通道10 11 12为保留通道*/
			channel += 3;
		}
	}
    _adjustData.set((channel - 1)*sizeof(float)+valueoffset(type),(char*)&v,sizeof(float));
}

void DTUParamAdjust::insert(uint32_t channel, const float& v, uint32_t type)
{
    THROW_RUNTIME_ERROR_IF(channel>CHANNEL_NO_MAX, "DTUParamAdjust SetValue 错误的通道号");
    THROW_RUNTIME_ERROR_IF(channel<CHANNEL_NO_MIN, "DTUParamAdjust GetValue 错误的通道号");
}

DTU::buffer &DTUParamAdjust::Adjdata()
{
    return _adjustData;
}

DTUParamAdjust &DTUParamAdjust::operator=(DTUParamAdjust &src)
{
    _adjustData = src.Adjdata();
    return *this;
}