#include "dtugoosesubscriber.h"
#include "dtugooseconfig.h"
#include <map>
#include <dtucmdcode.h>
#include "dtulog.h"
#include "dtutask_dsp.h"

using namespace DTU;

static void
gooseListener(GooseSubscriber subscriber, void* parameter)
{
	APPID appID = GooseSubscriber_getAppId(subscriber);

	MmsValue* values = GooseSubscriber_getDataSetValues(subscriber);
	char buffer[1024];
	MmsValue_printToBuffer(values, buffer, 1024);
	
	printf("GOOSE[%u]接收到消息:[%s] AllData: %s TimeAllowToLive %u \n",
			GooseSubscriber_getAppId(subscriber),GooseSubscriber_isValid(subscriber)?"有效":"无效", buffer,
			GooseSubscriber_getTimeAllowedToLive(subscriber));
	
	// 筛选
	// 判断配置版本号是否正确
	if(GooseSubscriber_getConfRev(subscriber) != ((GooseCFG*) parameter)->ConfRev) {
		return;
	}

	// 判断GOOSE控制块引用是否正确
	std::string gocbref = GooseSubscriber_getGoCbRef(subscriber);
	std::string compare = ((GooseCFG*) parameter)->gocbRef + std::to_string(appID);
	if(gocbref != compare) {
		return;
	}

	// 判断GOOSE ID是否正确
	std::string GoID = GooseSubscriber_getGoId(subscriber);
	if(GoID != compare) {
		return;
	}

	if (GooseSubscriber_isValid(subscriber)) {
		MmsValue* values = GooseSubscriber_getDataSetValues(subscriber);
		if (MmsValue_getType(values) == MMS_ARRAY)
		{
			GooseData inputValue;
			for (uint32_t i = 0; i < MmsValue_getArraySize(values); i++)
			{
				MmsType type = MmsValue_getType(MmsValue_getElement(values, i));
				switch (type)
				{
					case MMS_BOOLEAN: {
						bool v = MmsValue_getBoolean(MmsValue_getElement(values, i));
						inputValue.push_back(v);
					};break;
				}
			}
			dtuGooseSubscriber::instance().updateGooseTime(appID);
			dtuGooseTableMgr::instance().updateGooseValues(appID, inputValue);
		}
	}

	// printf("GOOSE event:\n");
	// printf("  stNum: %u sqNum: %u\n", GooseSubscriber_getStNum(subscriber),
	// 	GooseSubscriber_getSqNum(subscriber));
	// printf("  timeToLive: %u\n", GooseSubscriber_getTimeAllowedToLive(subscriber));
	// uint64_t timestamp = GooseSubscriber_getTimestamp(subscriber);
	// printf("  timestamp: %u.%u\n", (uint32_t)(timestamp / 1000), (uint32_t)(timestamp % 1000));
	// printf(" data size: %u\n", MmsValue_getArraySize(values));
}

dtuGooseSubscriber::~dtuGooseSubscriber()
{
	GooseReceiver_stop(_receiver);
	GooseReceiver_destroy(_receiver);
	for(auto &item : _subscribers) {
		GooseSubscriber_destroy(item.second);
	}
}

bool dtuGooseSubscriber::init(const GooseCFG &cfg)
{
	_receiver = GooseReceiver_create();

	DTULOG(DTU_INFO, "Goose订阅者已在网卡[%s]启动", cfg.ineth.c_str());
	GooseReceiver_setInterfaceId(_receiver, const_cast<char*>(cfg.ineth.c_str()));

	thisCFG = cfg;

	for(auto& item : cfg.GItems)
	{
		std::string goCbRefNew = cfg.gocbRef + std::to_string(item.first);
		
		GooseSubscriber sub = GooseSubscriber_create(const_cast<char*>(goCbRefNew.c_str()), NULL);
		
		uint8_t buff[6] = {};
		transMacStr(cfg.mac, ":", buff);
		// 设置目的Mac 如果不设置则可能导致接收不到数据
		GooseSubscriber_setDstMac(sub, buff);

		GooseSubscriber_setAppId(sub, item.first);
		// 设置回调函数
		GooseSubscriber_setListener(sub, gooseListener, &thisCFG);

		GooseReceiver_addSubscriber(_receiver, sub);

		_subscribers[item.first] = sub;
	}

	return true;
}

msSinceEpoch abstime(msSinceEpoch time1, msSinceEpoch time2)
{
	if (time1 > time2)
		return time1 - time2;
	else
		return time2 - time1;
}

void dtuGooseSubscriber::run()
{
	std::thread t([&]() 
	{
		GooseReceiver_start(_receiver);
		if (GooseReceiver_isRunning(_receiver)) {
			for (;;) {
				if (_stop) {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		else {
			DTULOG(DTU_ERROR,"无法启动GOOSE订阅服务.原因可能是以太网接口不存在或需要根权限.");
		}
	});
	t.detach();

	// GOOSE断链判断
	std::thread t1([&]() 
	{
		for (;;)
		{
			if (_stop) {
				break;
			}
			{
				auto currTime = Hal_getTimeInMs();
				std::lock_guard<std::mutex> lock(_GooseTimeLock);
				for (const auto& item : _GooseLinkMap) {
					if (abstime(currTime, item.second) > 20000) {
						updateGooseLinkState(item.first,false);
					}
					else {
						updateGooseLinkState(item.first,true);
					}
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
	t1.detach();
}

void dtuGooseSubscriber::stop()
{
	_stop = true;
}

void dtuGooseSubscriber::updateGooseTime(APPID appid)
{
	std::lock_guard<std::mutex> lock(_GooseTimeLock);
	_GooseLinkMap[appid] = Hal_getTimeInMs();
}

void dtuGooseSubscriber::updateGooseLinkState(APPID appid, bool state)
{
	// 查找appID
	auto ita = _GooseLinkStateMap.find(appid);
	if(ita != _GooseLinkStateMap.end())
	{
		if(ita->second != state)
		{
			// 能找到并且本次和上次不相等 下发GOOSE信号改变
			ita->second = state;
			bool curState = true;
			for(auto &item : _GooseLinkStateMap) {
				curState = (curState && item.second);
			}
			// 下发信号
			if(curState) {
				// 恢复
				dsptask_execute_control(PC_W_GOOSE_COMM_RST);
				DTULOG(DTU_INFO,"APPID:[%d]GOOSE连接恢复", appid);
			}
			else {
				// 断链
				dsptask_execute_control(PC_W_GOOSE_COMM_YC);
				DTULOG(DTU_ERROR,"APPID:[%d]GOOSE连接断开", appid);
			}
		}
		// 状态相等则不做出操作
	}
	else
	{
		// 找不到appID的状态 添加新的状态
		_GooseLinkStateMap[appid] = state;
		bool curState = true;
		for(auto &item : _GooseLinkStateMap) {
			curState = (curState && item.second);
		}
		// 下发信号
		if(curState) {
			// 恢复
			dsptask_execute_control(PC_W_GOOSE_COMM_RST);
			DTULOG(DTU_INFO,"APPID[%d]首次GOOSE恢复",appid);
		}
		else {
			// 断链
			dsptask_execute_control(PC_W_GOOSE_COMM_YC);
			DTULOG(DTU_ERROR,"APPID[%d]首次GOOSE断链",appid);
		}
	}
}