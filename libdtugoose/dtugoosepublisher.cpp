#include "dtugoosepublisher.h"
#include <regex>

#include "dtucommon.h"
#include "dtulog.h"

using namespace DTU;

//-> dtuGooseOutputer

dtuGooseOutputer::dtuGooseOutputer(APPID appid, const std::vector<uint8_t>& value)
{
	if (value.size() < 4) {
		return;
	}
	fault = value[0];
	check = value[1];
	isolate = value[2];
	current = value[3];
}

LinkedList dtuGooseOutputer::makePackage()
{
	LinkedList dataSet = LinkedList_create();
	LinkedList_add(dataSet, MmsValue_newBoolean(fault));
	LinkedList_add(dataSet, MmsValue_newBoolean(check));
	LinkedList_add(dataSet, MmsValue_newBoolean(isolate));
	LinkedList_add(dataSet, MmsValue_newBoolean(current));
	DTULOG(DTU_DEBUG, "节点故障[%d]故障隔离成功[%d]开关拒跳[%d]过流闭锁[%d]", fault, check, isolate, current);
	return dataSet;
}

void dtuGooseOutputer::clear()
{
	fault = false;
	check = false;
	isolate = false;
	current = false;
}

///////////////////////////////////////////////

bool dtuGoosePublisher::init(const GooseCFG &cfg)
{
	CommParameters gooseCommParameters;

	auto result = split_str(cfg.mac, ":");

	if (result.size() < 6) {
		return false;
	}

	_appid = cfg.appID;

	gooseCommParameters.dstAddress[0] = (char)strtol(result[0].c_str(), NULL, 16);
	gooseCommParameters.dstAddress[1] = (char)strtol(result[1].c_str(), NULL, 16);
	gooseCommParameters.dstAddress[2] = (char)strtol(result[2].c_str(), NULL, 16);
	gooseCommParameters.dstAddress[3] = (char)strtol(result[3].c_str(), NULL, 16);
	gooseCommParameters.dstAddress[4] = (char)strtol(result[4].c_str(), NULL, 16);
	gooseCommParameters.dstAddress[5] = (char)strtol(result[5].c_str(), NULL, 16);

	gooseCommParameters.appId = cfg.appID;
	gooseCommParameters.vlanId = 0;
	gooseCommParameters.vlanPriority = 4;
	
	DTULOG(DTU_INFO, "Goose发布者已在网卡[%s]启动", cfg.outeth.c_str());
	_publisher = GoosePublisher_create(&gooseCommParameters, cfg.outeth.c_str());

	if (!_publisher) {
		DTULOG(DTU_ERROR, "Goose发布者启动失败");
		return false;
	}
	
	std::string goCbRefNew = cfg.gocbRef + std::to_string(cfg.appID);

	GoosePublisher_setGoCbRef(_publisher, const_cast<char*>(goCbRefNew.c_str()));
	GoosePublisher_setConfRev(_publisher, cfg.ConfRev);
	GoosePublisher_setDataSetRef(_publisher, const_cast<char*>(cfg.dataSet.c_str()));
	GoosePublisher_setTimeAllowedToLive(_publisher, cfg.TimeAllowedToLive);

	return true;
}

void dtuGoosePublisher::publish(const dtuGooseOutputer& data)
{
	_pulish_queue.enqueue(data);
	isPing = false;
	cv.notify_one();
}

void dtuGoosePublisher::run()
{
	std::thread t([&]() {
		for (;;) {
			if (_stop) {
				break;
			}

			dtuGooseOutputer output;
			// 条件变量加锁
			std::unique_lock<std::mutex> lock(mmutex);
            cv.wait_for(lock,std::chrono::seconds(5));

			if(!isPing)
			{

				for (;_pulish_queue.try_dequeue(output);)
				{
					if (!_publisher) {
						// DTULOG(DTU_ERROR, "Goose发布者未启动");
						continue;
					}


					// 变化Stnum
					GoosePublisher_increaseStNum(_publisher);
					// 数据
					auto mmsValue = output.makePackage();
					for (auto i = 0; i < 5; i++)
					{
						GoosePublisher_publish(_publisher, mmsValue);
						if (i < 2) {
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
						}
						else if (i < 3) {
							std::this_thread::sleep_for(std::chrono::milliseconds(2));
						}
						else if (i < 4) {
							std::this_thread::sleep_for(std::chrono::milliseconds(4));
						}
					}
					isPing = true;
					// 将本帧数据赋值给心跳帧数据
					lastFrame = output;
					// printf("TEST 发送数据\n");
				}

				// if (_pulish_queue.try_dequeue(output))
				// {
				// 	if (!_publisher) {
				// 		// DTULOG(DTU_ERROR, "Goose发布者未启动");
				// 		continue;
				// 	}


				// 	// 变化Stnum
				// 	GoosePublisher_increaseStNum(_publisher);
				// 	// 数据
				// 	auto mmsValue = output.makePackage();
				// 	for (auto i = 0; i < 5; i++)
				// 	{
				// 		GoosePublisher_publish(_publisher, mmsValue);
				// 		if (i < 2) {
				// 			std::this_thread::sleep_for(std::chrono::milliseconds(1));
				// 		}
				// 		else if (i < 3) {
				// 			std::this_thread::sleep_for(std::chrono::milliseconds(2));
				// 		}
				// 		else if (i < 4) {
				// 			std::this_thread::sleep_for(std::chrono::milliseconds(4));
				// 		}
				// 	}
				// 	isPing = true;
				// 	// 将本帧数据赋值给心跳帧数据
				// 	lastFrame = output;
				// 	// printf("TEST 发送数据\n");
				// }
				// else {
				// 	DTULOG(DTU_ERROR,"发送数据失败 数据可能已经损坏");
				// }
			}
			else 
			{
				if (!_publisher) {
					// DTULOG(DTU_ERROR, "Goose发布者未启动");
					continue;
				}
				// 心跳 从上次数据中取出
				auto mmsValue = lastFrame.makePackage();
				GoosePublisher_publish(_publisher, mmsValue);
				LinkedList_destroyDeep(mmsValue, (LinkedListValueDeleteFunction)MmsValue_delete);
				// printf("TEST 发送心跳\n");
				// DTULOG(DTU_ERROR,"发送GOOSE心跳");
			}
		}
	});
	t.detach();
}

void dtuGoosePublisher::stop()
{
	_stop = true;
}

///////////////////////////////////////////////

bool dtuGoosePublisherMgr::load(const GooseCFG &cfg)
{
	if(_publisher) {
		delete _publisher;
	}
	_publisher = new dtuGoosePublisher;
	_publisher->init(cfg);
	appid = cfg.appID;
	return true;
}

bool dtuGoosePublisherMgr::publish(const GooseData& gooseData)
{
	if(gooseData.size() != 4) {
		DTULOG(DTU_ERROR,"GOOSE发布长度错误目标为4,当前为%u",gooseData.size());
		return false;
	}
	_publisher->publish(dtuGooseOutputer(this->appid, gooseData));
}

void dtuGoosePublisherMgr::run()
{
	if(_publisher) {
		_publisher->run();
	}
	else {
		DTULOG(DTU_ERROR,"GOOSE服务启动失败");
	}
}

void dtuGoosePublisherMgr::stop()
{
	if(_publisher) {
		_publisher->stop();
		delete _publisher;
	}
}