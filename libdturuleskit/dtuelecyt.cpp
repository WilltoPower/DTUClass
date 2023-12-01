#include "dtuelecyt.h"
#include "dtulog.h"

using namespace DTU;

uint8_t TYParse::createParamerIdentification(bool isPreset, bool CR, bool isFollow)
{
    uint8_t result = 0x00;

    if (isPreset)
        result = result | 0x80;

    if (CR)
        result = result | 0x40;

    if (isFollow)
        result = result | 0x01;

    return result;
}

void TYParse::parseParamerIdentification(const uint8_t& flag, bool& isPreset, bool& CR, bool& isFollow)
{
    isPreset = false;
    CR = false;
    isFollow = false;

    if (flag & 0x80)
        isPreset = true;

    if (flag & 0x40)
        CR = true;

    if (flag & 0x01)
        isFollow = true;
}

bool TYParse::parseReadParam(CS101_ASDU asdu, std::vector<IOA>& result, bool& readAll, int& groupno, int sizeofIOA)
{
    int count = CS101_ASDU_getNumberOfElements(asdu);
    result.clear();
    readAll = false;

    // 获取点表列表
    DTU::buffer payload((char*)(CS101_ASDU_getPayload(asdu)), CS101_ASDU_getPayloadSize(asdu));

    try
    {
        // 获取定值区号
        groupno = payload.get(0, sizeof(uint16_t)).value<uint16_t>();

        for (int offset = 2; offset < payload.size();)
        {
            if (sizeofIOA > 3)
                sizeofIOA = 2;

            IOA ioa = payload.get(offset, sizeofIOA).value<IOA>();

            result.emplace_back(ioa);

            offset += sizeofIOA;
        }
    }
    catch(const std::exception& e)
    {
        DTULOG(DTU_INFO, "读定值解析发生错误:%s", e.what());
        return false;
    }

    // 判断是否需要读取全部定值
    if (count == 0 && (payload.size() <=2)) {
        readAll = true;
        return true;
    }

    return true;
}

bool TYParse::addOneReadParam(CS101_ASDU asdu, IOA ioa, uint8_t tag, uint8_t datasize, const DTU::buffer& value, int sizeofIOA)
{
    if (CS101_ASDU_getNumberOfElements(asdu) >= 127 || CS101_ASDU_getNumberOfElements(asdu) < 0)
        return false;

    // TODO: 确认这里最大长度可以为多少
    if (CS101_ASDU_getPayloadSize(asdu) + sizeofIOA + sizeof(uint8_t) + sizeof(uint8_t) + value.size() > 240)
        return false;

    DTU::buffer payload = create_informationObjectAddress(ioa, tag, datasize, value, sizeofIOA);

    // 添加数据
    CS101_ASDU_addPayload(asdu, (uint8_t*)(payload.const_data()), payload.size());

    // 更改VSQ计数
    int count = CS101_ASDU_getNumberOfElements(asdu);
    count++;
    CS101_ASDU_setNumberOfElements(asdu, count);

    return true;
}

std::vector<CS101_ASDU> TYParse::readParamReturn(const TYPacker& pack, CS101_AppLayerParameters alParams, int ca, int oa, int groupno)
{
    std::vector<CS101_ASDU> ret;

    CS101_ASDU newAsdu = nullptr;

    // 创建ASDU
    newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, oa, ca, false, false);
    // 设置Ti
    CS101_ASDU_setTypeID(newAsdu, C_RS_NA_1);

    // 添加组号
    CS101_ASDU_addPayload(newAsdu, (uint8_t*)&groupno, sizeof(uint16_t));

    for (const auto& item : pack)
    {
        bool retn = addOneReadParam(newAsdu, item.first, std::get<0>(item.second), std::get<1>(item.second), std::get<2>(item.second), alParams->sizeOfIOA);

        if (!retn) {
            // 已满添加到返回结果 添加新的结果集合
            ret.emplace_back(newAsdu);
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, oa, ca, false, false);
            CS101_ASDU_setTypeID(newAsdu, C_RS_NA_1);
            // 添加组号
            CS101_ASDU_addPayload(newAsdu, (uint8_t*)&groupno, sizeof(uint16_t));
            addOneReadParam(newAsdu, item.first, std::get<0>(item.second), std::get<1>(item.second), std::get<2>(item.second), alParams->sizeOfIOA);
        }
    }

    return ret;
}

bool TYParse::parseWriteParam(CS101_ASDU asdu, TYPacker& result, bool& isPreset, bool& isConfirm, int& groupno, int sizeofIOA)
{
    bool ret = false;
    bool isFollow = false;  // 是否存在后续报文
    result.clear();

    int count = CS101_ASDU_getNumberOfElements(asdu);

    DTU::buffer payload((char*)(CS101_ASDU_getPayload(asdu)), CS101_ASDU_getPayloadSize(asdu));

    // 获取定值区号
    groupno = payload.get(0, sizeof(uint16_t)).value<uint16_t>();
    // 获取 参数特征标识Pi
    uint8_t flag = payload.get(2, sizeof(uint8_t)).value<uint8_t>();
    // 解析 参数特征标识Pi
    parseParamerIdentification(flag, isPreset, isConfirm, isFollow);

    int offset = 0;
    if (count > 0) {
        // 解析IOA
        for (;payload.size() > offset;)
            result.insert(parse_informationObjectAddress(payload, offset, sizeofIOA));
    }

    return ret;
}

DTU::buffer TYParse::create_informationObjectAddress(IOA ioa, uint8_t tag, uint8_t length, const DTU::buffer& value, int sizeofIOA)
{
    DTU::buffer result;
    // 追加信息体地址
    if (sizeofIOA > 2) {
        result.append((char*)&ioa, sizeof(uint16_t));
        result.append(DTU::buffer(1));
    }
    else
        result.append((char*)&ioa, sizeofIOA);

    // TAG类型
    result.append((char*)&tag, sizeof(uint8_t));

    // 数据类型
    result.append((char*)&length, sizeof(uint8_t));

    // 追加数据值
    result.append(value);
}

std::pair<IOA, TYParse::oneYTIO> TYParse::parse_informationObjectAddress(const DTU::buffer& payload, int& offset, int sizeofIOA)
{
    IOA ioa = 0x0000;

    if (sizeofIOA < 2) 
        ioa = payload.get(offset, 1).value<uint8_t>();
    else
        ioa = payload.get(offset, sizeofIOA).value<uint16_t>();

    offset += sizeofIOA;

    // TAG类型
    uint8_t tag = payload.get(offset, sizeof(uint8_t)).value<uint8_t>();
    offset += sizeof(uint8_t);
    // 数据长度
    uint8_t length = payload.get(offset, sizeof(uint8_t)).value<uint8_t>();
    offset += sizeof(uint8_t);
    // 值
    DTU::buffer value = payload.get(offset, length);
    offset += length;

    return {ioa, {tag, length, value}};
}
