#ifndef _DTU_DATA_STRUCT_H_
#define _DTU_DATA_STRUCT_H_

#include <stdint.h>

#include <msgpack.hpp>

namespace DTU
{
    using IOA=uint16_t;
    using HIOA=uint16_t;
    using CA=int;

    // 地址映射类型
    enum IOAMAPTYPE {
        IMAPT_YX,   /* 遥信映射 */
        IMAPT_YC,   /* 遥测映射 */
        IMAPT_YT,   /* 遥调映射 */
        IMAPT_YK,   /* 遥控映射 */
    };

    // IOA地址映射
    struct oneIOAMapItem {
        IOA ioa;                /* 信息体地址 */
        HIOA hioa;              /* 内部硬件地址 */
        IOAMAPTYPE type;        /* 映射类型 */
        bool spare = false;     /* 是否为备用 */
        bool active = false;    /* 是否需要主动上送 */
        CA ca = 0;              /* 来自哪一台设备 */
        MSGPACK_DEFINE(ioa, hioa, (int&)type, spare, active);
    };
    // 信息体地址映射
    using AllIOAMap = std::map<IOA, oneIOAMapItem>;
}

#endif  /* _DTU_DATA_STRUCT_H_ */