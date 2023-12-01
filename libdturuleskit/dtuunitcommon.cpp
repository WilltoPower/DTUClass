#include "dtuunitcommon.h"

#include "dtusystemconfig.h"

using namespace DTU;
using namespace DTUCFG;
using namespace BASE;

#define FIXBASE

transerInfomation dtuUnitCommonBase::ParseASDU(CS101_ASDU asdu)
{
    transerInfomation informationObj;
    informationObj.COT = CS101_ASDU_getCOT(asdu);
    informationObj.Ti = CS101_ASDU_getTypeID(asdu);
    informationObj.isSequence = CS101_ASDU_isSequence(asdu);
    informationObj.elementNumber = CS101_ASDU_getNumberOfElements(asdu);
    informationObj.payloadSize = CS101_ASDU_getPayloadSize(asdu);
    this->CA = CS101_ASDU_getCA(asdu);
    this->OA = CS101_ASDU_getOA(asdu);
    return informationObj;
}

void dtuUnitCommonBase::setCallback(std::function<void(CS101_ASDU, IMasterConnection)> function, IMasterConnection param)
{
    this->callback = function;
    this->connection = param;
}

void dtuUnitCommonBase::setAlParamer(CS101_AppLayerParameters alParams)
{
    this->alParams = alParams;
}

bool dtuUnitCommonBase::isSetCallback()
{
    return (this->callback != nullptr) && (this->connection != nullptr);
}

/*************************************************  RPC远程读写  **********************************************************/
void dtuUnitRPCTranser::sender(CS101_ASDU asdu)
{
    if(this->isSetCallback()) {
        this->callback(asdu, this->connection);
    }
    else {
        DTULOG(DTU_ERROR, "间隔单元调用未设置回调参数");
    }
}

int dtuUnitRPCTranser::reciver(CS101_ASDU asdu, int CA, BAY_UNIT_COMMON_CMD cmd, RemoteCtrlInfo rinfo)
{
    this->CA = CS101_ASDU_getOA(asdu);
    this->OA = CS101_ASDU_getCA(asdu);
    // 打包发送给间隔单元
    DTU::buffer transer = this->packer(asdu);

    std::vector<DTU::buffer> result;

    // 根据命令返回不同的ASDU
    switch(cmd)
    {
        case BAYUC_CMD_READ_PARAM: {
            result = dtuHALhandler::readParam(transer, rinfo);      // 读取定值
            break;
        }
        case BAYUC_CMD_WRITE_PARAM: {
            result = dtuHALhandler::writeParam(transer, rinfo);    // 写入定值
            auto ret = dtuHALhandler::createCRCOnly(this->OA, this->CA, IMasterConnection_getApplicationLayerParameters(connection));
            result.insert(result.end(), ret.begin(), ret.end());
            break;
        }
        case BAYUC_CMD_FIEL_REQ: {
            result = dtuHALhandler::fileRequest(transer);           // 文件请求
            break;
        }
        case BAYUC_CMD_CHANGE_GROUP: {
            result = dtuHALhandler::changeCurrentGroup(transer);    // 更改定值区
            auto ret = dtuHALhandler::createCRCOnly(this->OA, this->CA, IMasterConnection_getApplicationLayerParameters(connection));
            result.insert(result.end(), ret.begin(), ret.end());
            break;
        }
        case BAYUC_CMD_CURRENT_GROUP: {
            result = dtuHALhandler::readCurrentGroup(transer, rinfo);// 读取定值区
            break;
        }
        case BAYUC_CMD_ROMATE_CTRL: {
            result = dtuHALhandler::remoteCtrl(transer, rinfo);     // 遥控命令
            break;
        }
        case BAYUC_CMD_TIME_CAPTURE: {
            result = dtuHALhandler::timeCapture(transer);           // 时间获取命令
            break;
        }
        case BAYUC_CMD_UNKNOW_TYPEID: {
            DTULOG(DTU_INFO, "未知的TYPE ID %d", CS101_ASDU_getTypeID(asdu));
            result = dtuHALhandler::unknownTypeID(transer);         // 未知的TypeID
            break;
        }
        case BAYUC_CMD_READ_PARAM_B : {
            result = dtuHALhandler::NewreadParam_B(asdu, this->alParams, rinfo);    // 附录B 定值读取
            break;
        }
        case BAYUC_CMD_WRITE_PARAM_B: {
            result = dtuHALhandler::NewwriteParam_B(asdu, this->alParams);   // 附录B 定值写入
            break;
        }
    }

    // 解析回调结果 并发送
    this->unpacker(result);
}

DTU::buffer dtuUnitRPCTranser::packer(CS101_ASDU asdu)
{
    transerInfomation transObj = this->ParseASDU(asdu);
    DTU::buffer transer;
    transer.append((char*)&transObj, sizeof(transerInfomation));
    transer.append((char*)CS101_ASDU_getPayload(asdu), CS101_ASDU_getPayloadSize(asdu));
    return transer;
}

void dtuUnitRPCTranser::unpacker(std::vector<DTU::buffer> &data)
{

    bool isSeque = false;

    for(auto &item : data)
    {
        transerInfomation ret = item.get(0, sizeof(transerInfomation)).value<transerInfomation>();
        if(this->isSetCallback()) {

            if (ret.COT == CS101_COT_INTERROGATED_BY_STATION) {
                // 获取是否压缩

                if (DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use) {
                    isSeque = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence;
                }
                else {
                    isSeque = DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence;
                }
                DTULOG(DTU_INFO, "总召唤格式[%s]", isSeque ? "压缩" : "非压缩");
            }

            CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
            CS101_ASDU newASDU = CS101_ASDU_create(alParams, isSeque, static_cast<CS101_CauseOfTransmission>(ret.COT), 
                                                    this->OA, this->CA, false, false);

            // 内容追加 有内容才会追加
            if (ret.payloadSize > 0)
                CS101_ASDU_addPayload(newASDU, (uint8_t*)(item.get(sizeof(transerInfomation), ret.payloadSize).const_data()), ret.payloadSize);

            // 设置元素个数
            CS101_ASDU_setNumberOfElements(newASDU, ret.elementNumber);
            // 设置TYPEID
            CS101_ASDU_setTypeID(newASDU, static_cast<IEC60870_5_TypeID>(ret.Ti));
            // [回调]向主站发送结果
            this->sender(newASDU);
        }
        else {
            DTULOG(DTU_ERROR, "间隔单元调用未设置回调参数,回调返回解析失败");
        }
    }
}

void dtuUnitRPCTranser::IMasterConnect(CS101_ASDU asdu, CSTYPE type)
{
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
    std::vector<DTU::buffer> sender;

    this->CA = CS101_ASDU_getCA(asdu);
    this->OA = CS101_ASDU_getOA(asdu);

    auto ret = dtuHALhandler::NewIMasterConnect(static_cast<int>(type), alParams);
    sender.insert(sender.end(), ret.begin(), ret.end());

    // // 读取本机总召唤信息
    // auto tempbuffer = dtuHALhandler::IMasterConnect(static_cast<int>(type), true);
    // sender.insert(sender.end(), tempbuffer.begin(), tempbuffer.end());

    // if (DSYSCFG::instance().isPublic()) {
    //     for(const auto &item : DSYSCFG::instance().GetUnitCFG().info)
    //     {
    //         if (item.second.use) {
    //             auto temp = dtuHALhandler::caller(item.second, "rpc_proto_IMasterConnect", static_cast<int>(type), false);
    //             sender.insert(sender.end(), temp.begin(), temp.end());
    //         }
    //     }
    // }

    this->unpacker(sender);
}