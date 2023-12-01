/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtusystemconfig.h
  *Description:
    101/104相关的配置
  *History:
    1, 创建, wangjs, 2021-7-8
    2, 加入101/104的是否使用开关, wangjs, 2021-8-3
    3, 加入配置工具RPC通信的端口, wangjs, 2021-8-4
**********************************************************************************/
#ifndef _COMM_CFG_H
#define _COMM_CFG_H
#include <string>
#include <vector>
#include <atomic>
#include <tuple>
#include <string>
#include <map>
#include <atomic>
#include "dtucommon.h"
#include <dtubuffer.h>

#include "json/json.h"

namespace DTUCFG {

    // 测量值类型
    enum MEASURED_TYPE {
        MEAS_MEASURED_NORMALIZED = 0x00,    /* 测量值 归一化值 */
        MEAS_MEASURED_NORMALIZED_CP56,      /* 测量值 带CP56时标的归一化值 */
        MEAS_MEASURED_NORMALIZED_CP24,      /* 测量值 带CP24时标的归一化值 */
        MEAS_MEASURED_SCALED,               /* 测量值 标度化值 */
        MEAS_MEASURED_SCALED_CP56,          /* 测量值 带CP56时标的标度化值 */
        MEAS_MEASURED_SCALED_CP24,          /* 测量值 带CP24时标的标度化值 */
        MEAS_MEASURED_SHORT,                /* 测量值 短浮点数 */
        MEAS_MEASURED_SHORT_CP56,           /* 测量值 带CP56时标的短浮点数 */
        MEAS_MEASURED_SHORT_CP24,           /* 测量值 带CP24时标的短浮点数 */
    };

    // 遥信值类型
    enum TELEGRAM_TYPE {
        TELE_SINGLE_POINT = 0x00,           /* 遥信值 单点遥信 */
        TELE_SINGLE_POINT_CP56,             /* 遥信值 带CP56时标的单点遥信 */
        TELE_SINGLE_POINT_CP24,             /* 遥信值 带CP24时标的单点遥信 */
        TELE_DOUBLE_POINT,                  /* 遥信值 双点遥信 */
        TELE_DOUBLE_POINT_CP56,             /* 遥信值 带CP56时标的双点遥信 */
        TELE_DOUBLE_POINT_CP24,             /* 遥信值 带CP24时标的双点遥信 */
    };

    //-> 101链路模式
    enum LinkLayerMode {
        LINK_LAYER_BALANCED = 0,            /* 平衡模式传输 */
        LINK_LAYER_UNBALANCED = 1,          /* 非平衡模式传输 */
    };

    //-> 串口配置
    struct SerialCFG {
        std::string name = "ttyS1";   // 串口
        int baudrate = 9600;            // 波特率
        int databits = 8;               // 数据位
        char pairty = 'N';              // 奇偶校验
        int stopbits = 1;               // 停止位
    };

    //-> 网口配置
    struct EthernetCFG {
        int port = 2404;
        std::string ip = "0.0.0.0";
    };

    //-----------------> class DSYSCFG
    class DSYSCFG
    {
        public:
            static DSYSCFG &instance() {
                static DSYSCFG cfg;
                return cfg;
            }

        private:
            DSYSCFG() : _load(false) {}

        public:
            //-> 规约应用参数
            struct CSAppLayerParameters {
                int sizeofCOT = 2;
                int sizeofCA = 2;
                int sizeofIOA = 3;
            };
            //-> 规约链路参数
            struct CSLinkLayerParameters {
                int LinkAddrLength = 1;
                bool SingalCharACK = true;
                int TimeoutForACK = 500;
                int TimeoutForRepeat = 1000;
            };
            //-> 规约值类型
            struct CSValueType {
                MEASURED_TYPE MeasuredValueType = MEAS_MEASURED_NORMALIZED;
                TELEGRAM_TYPE TelegramValueType = TELE_SINGLE_POINT;
            };
            //-> 通用其他配置
            struct ExtraParameters {
                bool isSequence = true;     /* 是否使用压缩格式 */
            };
            //-> 101规约配置
            struct CS101Cfg {
                bool use = false;
                LinkLayerMode mode = LINK_LAYER_UNBALANCED;
                int otheraddr = 2;                          /* 主站链路地址 */
                SerialCFG serial;
                CSValueType VType;
                CSAppLayerParameters ALParam;
                CSLinkLayerParameters LLParam;
                ExtraParameters EXParam;
            };
            //-> 104规约配置
            struct CS104Cfg {
                bool use = false;
                EthernetCFG eth;
                CSValueType VType;
                CSAppLayerParameters ALParam;
                ExtraParameters EXParam;
            };
            //-> 规约单端配置
            struct StationCFG {
                CS101Cfg CS101;
                CS104Cfg CS104;
            };
            //-> 60870 规约总配置
            struct IEC60870CFG {
                StationCFG Master;
                StationCFG Slave;
            };

        public:
            //-> 线损模块配置
            struct LineLossCfg {
                bool use = false;
                CS101Cfg proto;
            };
            //-> LCD屏幕配置
            using LCDCfg = SerialCFG;
            //-> RPC配置
            using RPCCfg = EthernetCFG;
            // 单元间通信协议
            enum UNITPROTOCOL {
                PROTO_CS101 = 0x00, // 101规约
                PROTO_CS104,        // 104规约
                PROTO_CSRPC,        // RPC规约
            };

            using CA = int;
            using OA = int;
            using ASDUAddr = int;   // 在101中为链路层地址

            struct OneBAYCFG {
                bool use = false;                       // 是否使用该间隔单元
                CA ca = 1;                              // 公共单元地址(104中为CA 101中为单元的链路地址)
                UNITPROTOCOL proto = PROTO_CSRPC;       // 公共单元与间隔单元之间使用的通信协议
                StationCFG ProroCS;                     // 间隔单元规约配置
                EthernetCFG ProtoRPC;                   // 间隔单元RPC规约配置
            };
            using PUBLICCFG = OneBAYCFG;
            using BAY_ATTR = std::map<CA, OneBAYCFG>;
            //-> 单元配置
            enum UNITTYPE {
                MODE_ERR = 0x00,  /* 错误单元模式 */
                MODE_PUB,         /* 公共单元模式 */
                MODE_BAY,         /* 间隔单元模式 */
            };

            struct UnitCFG {
                UNITTYPE type = MODE_ERR;
                BAY_ATTR info;
            };
            //-> 自动校时配置
            struct SyncTimeCFG {
                bool use = true;
                int timeInSec = 15;
            };
            //-> GOOSE配置
            using APPID=int;
            struct oneGooseItem {
                bool use = false;
                APPID appid = 1000;
                MSGPACK_DEFINE(use, appid);
            };
            struct GooseCFG {
                APPID appid;                        // 本机APPID号
                std::string eth;                    // 网卡
                std::string ineth;                  // 输入网卡
                std::string outeth;                 // 输出网卡
                std::string mac;                    // 组播地址
                std::vector<oneGooseItem> mside;    // M侧连接设备
                std::vector<oneGooseItem> nside;    // N连接设备
                MSGPACK_DEFINE(appid, eth, ineth, outeth, mac, mside, nside);
            };

            struct MACCFG {
                bool use = false;
                std::string eth;
                std::string mac;
            };

        public:
            /**
             * @brief 载入配置
             * 
             * @param file 文件完整路径
             */
            void load();
            /**
             * @brief 判断是否为公共单元
             * 
             * @return 是否为公共单元
             */
            bool isPublic();
            /**
             * @brief 判断是否启用101规约主站端(线损模块开启或者主站端开启)
             * 
             * @return 是否启用101规约主站端
             */
            bool useCS101Slave();
            // 系统配置打包
            DTU::buffer syspack();
            // 系统配置保存
            bool syssave(std::string &file);
            // 系统配置解包
            bool sysunpack(DTU::buffer &data);
            // 获取单元模式
            const UNITTYPE &GetUnitType();
            // 获取对端单元配置(当单元为公共单元时,该函数获取的是间隔单元的配置;当为间隔单元时,该函数获取的是公共单元的配置)
            const PUBLICCFG &GetPublicCFG();
            // 获取间隔单元通信参数
            OneBAYCFG &GetUnitCFG(CA ca, bool &result);
            const UnitCFG &GetUnitCFG();
            // 获取RPC服务器配置
            const RPCCfg &GetRPCCFG();
            // 修改RPC服务器配置
            RPCCfg &ModifyRPCCFG();
            // 获取LCD配置
            const LCDCfg &GetLCDCFG();
            // 获取LCD配置
            LCDCfg &ModifyLCDCFG();
            // 获取校时配置
            const SyncTimeCFG &GetSyncCFG();
            // 设置校时
            SyncTimeCFG &ModifySyncCFG();
            // 获取线损模块配置
            const LineLossCfg &GetLineCFG();
            // 设置线损模块配置
            LineLossCfg &ModifyLineCFG();
            // 获取GOOSE配置
            const GooseCFG &GetGooseCFG();
            // 获取GOOSE配置
            GooseCFG &ModifyGooseCFG();
            // 设置间隔单元配置
            UnitCFG &ModifyUnitCFG();
            // 设置公共单元配置
            PUBLICCFG &ModifyPublicCFG();
        
        public:
            // 0为公共单元设备,这是默认的情况
            int devno = 0;
            std::string iedName;
            int linkAddr = 0;   // 链路层地址
            int piclinkaddr = 0;
            
        public:
            // GOOSE配置(仅在ARM中使用)
            const GooseCFG &read_from_file();
            bool save_to_file(const GooseCFG &data, bool iskill = true);

        private:
            // 载入系统配置
            bool load_sys_cfg(const std::string &fullPath);
            // 载入Goose配置
            bool load_goose_cfg(const std::string &fullPath);
            // 载入101/104规约配置
            bool load_cs_cfg(const std::string &fullPath);

        private:
            enum JsonParseFlag {
                JSON_FLAG_DEFALUT = 0x00,    /* 按默认从文件加载 */
                JSON_FLAG_NO_USE,            /* 不加载used标志 */
                JSON_FLAG_MANUAL,            /* 手动设置used标志 */
            };

            bool parse_cs101_from_Json(CS101Cfg &cfg, Json::Value &jitem, JsonParseFlag flag = JSON_FLAG_DEFALUT, bool use = false);
            bool parse_cs104_from_Json(CS104Cfg &cfg, Json::Value &jitem, JsonParseFlag flag = JSON_FLAG_DEFALUT, bool use = false);

            bool parse_cs101_to_Json(CS101Cfg &cfg, Json::Value &jitem, JsonParseFlag flag = JSON_FLAG_DEFALUT, bool use = false);
            bool parse_cs104_to_Json(CS104Cfg &cfg, Json::Value &jitem, JsonParseFlag flag = JSON_FLAG_DEFALUT, bool use = false);

        private:
            // 文件路径
            std::string filePath;
            // 是否完成加载
            std::atomic_bool _load;
            // 公共单元向间隔单元通信 配置
            UnitCFG UCFG;
            // RPC配置
            RPCCfg RPCCFG;
            // LCD屏幕配置
            LCDCfg LCDCFG;
            // 校时配置
            SyncTimeCFG STCFG;
            // 线损模块配置
            LineLossCfg LLCFG;
            // Goose配置
            GooseCFG GOOSECFG;
            // 间隔单元向公共单元通信 配置
            PUBLICCFG PUBCFG;
        
        public:
            // 网卡MAC
            std::vector<MACCFG> MacCFG;
            void fulshMAC();

        /**************************************  101/104规约配置  **************************************/
        public:
            // 保存文件
            bool cssave(std::string &file);
            // 获取ASDU单元地址
            int ASDU();
            // 设置ASDU单元地址
            void ASDU(int ASDUno);
            // 获取IEC60870配置(只读)
            const IEC60870CFG &Get_IEC60870_CFG();
            // 修改IEC60870配置
            IEC60870CFG &Modify_IEC60870_CFG();
            // 获取IEC60870主站端口配置(只读)
            const StationCFG &Get_IEC60870_Master_CFG();
            // 修改IEC60870主站端口配置(只读)
            StationCFG &Modify_IEC60870_Master_CFG();
            // 获取IEC60870从站端口配置(只读)
            const StationCFG &Get_IEC60870_Slave_CFG();
            // 修改IEC60870从站端口配置
            StationCFG &Modify_IEC60870_Slave_CFG();
            // 当前数据打包
            DTU::buffer cspack();
            // 数据解包
            bool csunpack(DTU::buffer &data);
            // 将配置恢复到默认值
            bool csrecover(std::string file = "", bool issave = false);

        private:
            int asduaddr = 1;
            IEC60870CFG IECCFG;

    };

}; // namespace DTUCFG
#endif
