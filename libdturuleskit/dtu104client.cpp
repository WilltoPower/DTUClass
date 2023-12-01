/*********************************************************************************
	*Copyright(C),2021-2025,sddl
	*FileName:  dtu104client.cpp
	*Description: 
		用于实现dtu间隔单元的104服务
	*History: 
		1, 创建, lhy, 2022-08-08
**********************************************************************************/
#include "dtu104client.h"
#include <stdio.h>

#include <dtusystemconfig.h>

using namespace DTU;
using namespace DTUCFG;

D104Client::~D104Client() 
{
    dtu104_stop_client();
}

// 接收回调 在这里进行主要处理
bool D104Client::dtu_asduRecvivedHandler(CS101_ASDU asdu)
{
    printf("ASDU type: %s(%i) elements: %i\n", TypeID_toString(CS101_ASDU_getTypeID(asdu)),
                                    CS101_ASDU_getTypeID(asdu), CS101_ASDU_getNumberOfElements(asdu));

    switch (CS101_ASDU_getTypeID(asdu))
    {
        case M_ME_TE_1: {

            printf("  measured scaled values with CP56Time2a timestamp:\n");

            int i;

            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {

                MeasuredValueScaledWithCP56Time2a io =
                        (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);

                printf("    IOA: %i value: %i\n",
                        InformationObject_getObjectAddress((InformationObject) io),
                        MeasuredValueScaled_getValue((MeasuredValueScaled) io)
                );

                MeasuredValueScaledWithCP56Time2a_destroy(io);
            }
        };break;
        case M_SP_NA_1: {

            printf("  single point information:\n");

            int i;

            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {

                SinglePointInformation io =
                        (SinglePointInformation) CS101_ASDU_getElement(asdu, i);

                printf("    IOA: %i value: %i\n",
                        InformationObject_getObjectAddress((InformationObject) io),
                        SinglePointInformation_getValue((SinglePointInformation) io)
                );

                SinglePointInformation_destroy(io);
            }
        };break;
        case C_TS_TA_1: {

            printf("  test command with timestamp\n");

        };break;
    }

    return true;
}

void D104Client::dtu_connectHandler(CS104_Connection conn, CS104_ConnectionEvent event)
{
    switch (event) {
    case CS104_CONNECTION_OPENED:
        printf("Connection established\n");
        break;
    case CS104_CONNECTION_CLOSED:
        printf("Connection closed\n");
        break;
    case CS104_CONNECTION_STARTDT_CON_RECEIVED:
        printf("Received STARTDT_CON\n");
        break;
    case CS104_CONNECTION_STOPDT_CON_RECEIVED:
        printf("Received STOPDT_CON\n");
        break;
    }
}

void D104Client::dtu_rawMessageHandler(uint8_t* msg, int size, bool sent)
{
    // if (sent)
    //     printf("SEND: ");
    // else
    //     printf("RCVD: ");

    // int i;
    // for (i = 0; i < size; i++) {
    //     printf("%02x ", msg[i]);
    // }

    // printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool asduReceivedHandler(void* parameter, int address, CS101_ASDU asdu)
{
    if (!parameter) 
        return false;

    ((D104Client*) parameter)->dtu_asduRecvivedHandler(asdu);
}

static void connectionHandler(void* parameter, CS104_Connection connection, CS104_ConnectionEvent event)
{
    if (!parameter) 
        return;
    
    ((D104Client*) parameter)->dtu_connectHandler(connection, event);
}

static void rawMessageHandler(void* parameter, uint8_t* msg, int msgSize, bool sent)
{
    if(!parameter)
        return;
    
    ((D104Client*) parameter)->dtu_rawMessageHandler(msg, msgSize, sent);
}

static bool install_hander(CS104_Connection conn, void *param)
{
    CS104_Connection_setConnectionHandler(conn, connectionHandler, param);
    CS104_Connection_setASDUReceivedHandler(conn, asduReceivedHandler, param);
    /* uncomment to log messages */
    CS104_Connection_setRawMessageHandler(conn, rawMessageHandler, param);

    return true;
}

bool D104Client::dtu104_init_client()
{
    // 这里要去连接管理里面找间隔单元的ip地址和端口号

    std::string ip = "localhost";
    uint16_t port = IEC_60870_5_104_DEFAULT_PORT;
    if(ip.empty()) {
        ip = "192.168.168.100";
    }

    con = CS104_Connection_create(ip.c_str(), port);

    CS101_AppLayerParameters alParams = CS104_Connection_getAppLayerParameters(con);
    
    alParams->sizeOfIOA = DSYSCFG::instance().Get_IEC60870_Master_CFG().CS104.ALParam.sizeofIOA;
    alParams->sizeOfCOT = DSYSCFG::instance().Get_IEC60870_Master_CFG().CS104.ALParam.sizeofCOT;
    alParams->sizeOfCA = DSYSCFG::instance().Get_IEC60870_Master_CFG().CS104.ALParam.sizeofCA;

    return install_hander(con, this);
}

void D104Client::dtu104_run_client()
{
    if(con != nullptr) {
        if (CS104_Connection_connect(con)) {
            CS104_Connection_sendStartDT(con);
        }
    }
}

void D104Client::dtu104_stop_client()
{
    if(con) {
        CS104_Connection_destroy(con);
        con = nullptr;
    }
}

CS101_ASDU 
D104Client::dtu_create_asdu(bool isSequence, CS101_CauseOfTransmission cot, int oa, int ca, bool isTest, bool isNegative)
{
    CS101_ASDU result = nullptr;
    if(con != nullptr) {
        CS101_AppLayerParameters alParams = CS104_Connection_getAppLayerParameters(con);
        result = CS101_ASDU_create(alParams, isSequence, cot, oa, ca, isTest, isNegative);
    }
    return result;
}

bool D104Client::send_asdu(CS101_ASDU asdu)
{
    bool result = false;
    if(asdu != nullptr) {
        result = CS104_Connection_sendASDU(con, asdu);
        CS101_ASDU_destroy(asdu);
        asdu = nullptr;
    }
    return result;
}
