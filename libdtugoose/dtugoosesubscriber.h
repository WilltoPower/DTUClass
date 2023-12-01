#pragma once
#include <libiec61850/goose_receiver.h>
#include <libiec61850/goose_subscriber.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

#include "dtugooseconfig.h"

namespace DTU
{
	class dtuGooseSubscriber
	{
		private:
			dtuGooseSubscriber():_stop(false) {}

		public:
			~dtuGooseSubscriber();
			static dtuGooseSubscriber& instance() {
				static dtuGooseSubscriber sub;
				return sub;
			}
		
		public:
			bool init(const GooseCFG &cfg);
			void run();
			void stop();
			void updateGooseTime(APPID appid);
			void updateGooseLinkState(APPID appid, bool state);
		
		private:
			GooseReceiver _receiver;
			std::atomic_bool _stop;

			std::mutex _GooseTimeLock;
			std::map<APPID, uint64_t> _GooseLinkMap;
			//-> 连接状态
			std::map<APPID, bool> _GooseLinkStateMap;
			std::map<APPID, GooseSubscriber> _subscribers;
			//-> Goose配置
			GooseCFG thisCFG;
	};
}
