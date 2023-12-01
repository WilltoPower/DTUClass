#ifndef _UNITY_COMMON_H_
#define _UNITY_COMMON_H_

#include <stdint.h>
#include <functional>

#include <lib60870/cs104_connection.h>
#include <lib60870/cs101_slave.h>

#include "dtusystemconfig.h"
#include "dtustructs.h"
#include "dtuhalasdu.h"
#include "dtucommon.h"
#include "dtubuffer.h"
#include "dtulog.h"

//规约通信需要有自己的私有消息队列

namespace DTU
{
    enum BAY_UNIT_COMMON_CMD {
        BAYUC_CMD_READ_PARAM = 0x00,    /* 读取定值 */
        BAYUC_CMD_WRITE_PARAM,          /* 写入定值 */
        BAYUC_CMD_FIEL_REQ,             /* 文件请求 */
        BAYUC_CMD_CHANGE_GROUP,         /* 更改定值区 */
        BAYUC_CMD_CURRENT_GROUP,        /* 读取定值区 */
        BAYUC_CMD_ROMATE_CTRL,          /* 遥控命令 */
        BAYUC_CMD_TIME_CAPTURE,         /* 时间获取命令 */
        BAYUC_CMD_UNKNOW_TYPEID,        /* 未知的TypeID */
        BAYUC_CMD_READ_PARAM_B,         /* 附录B 读取定值 */
        BAYUC_CMD_WRITE_PARAM_B,        /* 附录B 写入定值 */
    };

    enum BAY_FIXNO_TRANS {
        BAY_FIXNO_DZ_R,           /* 定值信息读 */
        BAY_FIXNO_DZ_W,           /* 定值信息写 */
        BAY_FIXNO_YK_C,           /* 遥控信息 */
    };

namespace BASE
{
    class dtuUnitCommonBase
    {
        public:
            /**
             * @brief 用于向主站发送最终数据
             * 
             * @param asdu 
             * @return int 
             */
            virtual void sender(CS101_ASDU asdu) = 0;
            /**
             * @brief 用于从主站接收最终数据
             * 
             * @param asdu 
             * @return int 
             */
            virtual int reciver(CS101_ASDU asdu, int CA, BAY_UNIT_COMMON_CMD cmd, RemoteCtrlInfo rinfo = RemoteCtrlInfo()) = 0;
            /**
             * @brief 用于装置间传输数据解包
             * 
             * @return DTU::buffer 
             */
            virtual DTU::buffer packer(CS101_ASDU asdu) = 0;
            /**
             * @brief 用于装置间传输数据打包
             * 
             * @return true 
             * @return false 
             */
            virtual void unpacker(std::vector<DTU::buffer> &data) = 0;

        public:
            using sendCallback = void(*)(CS101_ASDU asdu, IMasterConnection con);
            void setCallback(std::function<void(CS101_ASDU, IMasterConnection)> function, IMasterConnection param);
            bool isSetCallback();
            void setAlParamer(CS101_AppLayerParameters alParams);

        protected:
            transerInfomation ParseASDU(CS101_ASDU asdu);
        
        protected:
            IMasterConnection connection = nullptr; // 对外通信所需要的软件
            std::function<void(CS101_ASDU, IMasterConnection)> callback;
            int CA = 0; // 目标单元地址
            int OA = 0; // 源单元地址
            CS101_AppLayerParameters alParams;// 应用层参数
    };
}
    /***********************************************************************************************************/
    /**
     * @brief 间隔单元通信RPC
     * 
     */
    class dtuUnitRPCTranser : public BASE::dtuUnitCommonBase
    {
        public:
            dtuUnitRPCTranser() {}

        public:
            void sender(CS101_ASDU asdu) override;
            int reciver(CS101_ASDU asdu, int CA, BAY_UNIT_COMMON_CMD cmd, RemoteCtrlInfo rinfo = RemoteCtrlInfo()) override;
            enum CSTYPE {
                CS101 = 101,
                CS104 = 104,
            };
            // 总召唤回复
            void IMasterConnect(CS101_ASDU asdu, CSTYPE type);

        private:
            DTU::buffer packer(CS101_ASDU asdu) override;
            void unpacker(std::vector<DTU::buffer> &data) override;
    };
}

#endif /* _UNITY_COMMON_H_ */