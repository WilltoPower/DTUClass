#ifndef _DTU_GOOSE_PUBLISHER_H
#define _DTU_GOOSE_PUBLISHER_H
#include <vector>
#include <libiec61850/mms_value.h>
#include <libiec61850/goose_publisher.h>
#include <atomic>
#include <concurrentqueue.h>
#include <condition_variable>

#include "dtugooseconfig.h"

namespace DTU 
{
	class dtuGooseOutputer {
		public:
			dtuGooseOutputer() {}
			dtuGooseOutputer(APPID appid, const std::vector<uint8_t>& value);
		
		public:
			//-> 数据打包
			LinkedList makePackage();
			//-> 清空所有数据
			void clear();
	
		public:
			bool fault = false;		/* 节点故障 */
			bool check = false;		/* 开关拒跳 */
			bool isolate = false;	/* 故障隔离 */
			bool current = false;	/* 过流闭锁 */
	};

	class dtuGoosePublisher {
		public:
			dtuGoosePublisher():_stop(false) {
				isPing = true;
			}

		public:
			bool init(const GooseCFG &cfg);
			void publish(const dtuGooseOutputer& data);
			void run();
			void stop();

		private:
			GoosePublisher _publisher;
			//-> 是否为本帧是否为Ping
			std::atomic_bool isPing;
			std::atomic_bool _stop;

			APPID _appid = -1;
			//-> 条件变量
			std::condition_variable cv;
			std::mutex mmutex;
			//-> 发布者任务队列
			moodycamel::ConcurrentQueue<dtuGooseOutputer> _pulish_queue;
			//-> 上次发送帧保存
			dtuGooseOutputer lastFrame;

	};

	class dtuGoosePublisherMgr {
		public:
			static dtuGoosePublisherMgr& instance() {
				static dtuGoosePublisherMgr mgr;
				return mgr;
			}

		private:
			dtuGoosePublisherMgr() {}

		public:
			bool load(const GooseCFG &cfg);
			void run();
			bool publish(const std::vector<uint8_t>& gooseData);
			void stop();
		private:
			dtuGoosePublisher* _publisher = nullptr;
			APPID appid;
		
	};
}

#endif