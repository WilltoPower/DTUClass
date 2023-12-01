#ifndef _DTU_ELEC_YT_H_
#define _DTU_ELEC_YT_H_

#include <lib60870/cs104_connection.h>
#include <lib60870/iec60870_common.h>
#include <lib60870/cs101_information_objects.h>


#include "dtudatastruct.h"
#include "dtubuffer.h"

namespace DTU
{
    /**
     * @brief 遥调解析/打包
     * 
     */
    class TYParse
    {
    public:
        using oneYTIO = std::tuple<uint8_t, uint8_t, DTU::buffer>;
        using TYPacker = std::map<IOA, oneYTIO>;

    private:
        /**
         * @brief 创建参数特征标识
         * 
         * @param isPreset 是否进行预置 true:预置 false:执行
         * @param CR 取消预置标志位 true:取消预置
         * @param isFollow 是否存在后续标志 true:存在后续 false 不存在后续
         * @return uint8_t flag结果
         */
        static uint8_t createParamerIdentification(bool isPreset, bool CR, bool isFollow);
        /**
         * @brief 解析参数特征标识
         * 
         * @param flag 参数特征标识
         * @param isPreset 是否进行预置 true:预置 false:执行
         * @param CR 取消预置标志位 true:取消预置
         * @param isFollow 是否存在后续标志 true:存在后续 false 不存在后续
         */
        static void parseParamerIdentification(const uint8_t& flag, bool& isPreset, bool& CR, bool& isFollow);
    
    public:
        static bool parseReadParam(CS101_ASDU asdu, std::vector<IOA>& result, bool& readAll, int& groupno, int sizeofIOA);

        // TAG size value
        // 注意这里需要更改alParams里的IOAsize
        static std::vector<CS101_ASDU> readParamReturn(const TYPacker& pack, CS101_AppLayerParameters alParams, int ca, int oa, int groupno);

        static bool parseWriteParam(CS101_ASDU asdu, TYPacker& result, bool& isPreset, bool& isConfirm, int& groupno, int sizeofIOA);

    private:
        static bool addOneReadParam(CS101_ASDU asdu, IOA ioa, uint8_t tag, uint8_t datasize, const DTU::buffer& value, int sizeofIOA);

        static DTU::buffer create_informationObjectAddress(IOA ioa, uint8_t tag, uint8_t length, const DTU::buffer& value, int sizeofIOA);
        static std::pair<IOA, oneYTIO> parse_informationObjectAddress(const DTU::buffer& payload, int& offset, int sizeofIOA);
    };
}

#endif