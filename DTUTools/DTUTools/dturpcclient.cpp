#include "dturpcclient.h"
#include "dtusysconfig.h"
#include <dtucmdcode.h>
#include <dtuprotocol.h>
#include <dtutask.h>
#include <dtulog.h>

void dturpcclient::init(const std::string& ip, uint16_t port,bool reinit)
{
	if (reinit)
	{
		_client.update_addr(ip,port);
		return;
	}
	_ip = ip;
	_port = port;
	thisInfoID = 0;
	runInfomation = false;
	// _client.enable_auto_heartbeat();
	_client.enable_auto_reconnect();
	_client.connect(_ip, _port);
	_client.set_connect_timeout(100);

	// 订阅频道 report
	_client.subscribe("report", [&](string_view data) {
		msgpack_codec codec;
		DTU::dtuprotocol p = codec.unpack<DTU::dtuprotocol>(data.data(), data.size());
		emit update_report(p._cmd, QByteArray(p._data.data(), p._data.size()));
		});

	// 订阅频道 CSLOG
	_client.subscribe("CSLOG", [&](string_view data) {
		msgpack_codec codec;
		DTU::dtuprotocol p = codec.unpack<DTU::dtuprotocol>(data.data(), data.size());
		emit update_CSLOG(QByteArray(p._data.data(), p._data.size()));
	});

	// 订阅频道 CSMSG
	_client.subscribe("CSMSG", [&](string_view data) {
		msgpack_codec codec;
		DTU::dtuprotocol p = codec.unpack<DTU::dtuprotocol>(data.data(), data.size());
		emit update_CSMSG(QByteArray(p._data.data(), p._data.size()));
		});

	// 订阅GOOSE状态
	_client.subscribe("GOOSE", [&](string_view data) {
		msgpack_codec codec;
		DTU::dtuprotocol p = codec.unpack<DTU::dtuprotocol>(data.data(), data.size());
		emit update_GOOSE(QByteArray(p._data.data(), p._data.size()));
		});

}

void dturpcclient::setInformationID(uint16_t infoID,bool isrun)
{
	thisInfoID = infoID;
	runInfomation = isrun;
}

void dturpcclient::run()
{
	std::thread t([&]() {
		while (1)
		{
			try
			{
				int cmd = 0;
				bool execrun = false;
				uint16_t InfoID = 0;
				for (;;)
				{
					execrun = runInfomation;
					if (_client.has_connected() && execrun)
					{
						InfoID = thisInfoID;
						switch (thisInfoID)
						{
						case 0:cmd = PC_R_HYX_DATA; break;
						case 1:cmd = PC_R_YC_DATA; break;
						case 2:cmd = PC_R_CHECK; break;
						case 3:cmd = PC_R_PROFUN_STATE; break;
						}
						DTU::buffer result = _client.call<DTU::buffer>("rpc_read_parameter", cmd);
						emit update_information(InfoID, QByteArray(result.const_data(), result.size()));
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
			}
			catch (std::exception& e)
			{
				QtDTULOG(DTU_ERROR,"查询信息发生错误:%s",e.what());
			}
		}
	});
	t.detach();

	std::thread tstatus([&]() {
		bool LastState = false;
		bool Curstate = false;
		bool firstTime = true; // 第一次发送
		int changeCount = 0;
		for (;;)
		{
			Curstate = _client.has_connected();
			if (Curstate != LastState)
			{
				if (changeCount > 15)
				{
					emit update_status(Curstate);
					LastState = Curstate;
					changeCount = 0;
				}
				else
				{
					changeCount++;
				}
			}
			else
			{
				if (firstTime)
				{
					emit update_status(Curstate);
					firstTime = false;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});
	tstatus.detach();
}

void dturpcclient::change_net_param()
{
	init(DESTIP(), DESTPORT(),true);
}
