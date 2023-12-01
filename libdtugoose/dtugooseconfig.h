#ifndef _DTU_GOOSE_CONFIG_H_
#define _DTU_GOOSE_CONFIG_H_

#include <string>
#include <vector>
#include <map>
#include <atomic>

using APPID = int;
using VOFFSET = int;
using GooseData = std::vector<uint8_t>;
using GooseAllTable = std::vector<uint8_t>;

enum GooseValueType {
	GTYPE_FAULT,		/* 节点故障 */
	GTYPE_ISOLATE,		/* 故障隔离 */
	GTYPE_CHECK,		/* 开关拒跳 */
	GTYPE_CURRENT,		/* 过流闭锁 */
};

enum GooseSide {
	GOOSE_NLL_SIDE,
	GOOSE_N_SIDE,
	GOOSE_M_SIDE,
};

struct GCFGItem {
	bool use = false;
	APPID appID = 1000;
	GooseSide side = GOOSE_NLL_SIDE;
	VOFFSET faultIndex = 0;
	VOFFSET checkIndex = 0;
	VOFFSET isolateIndex = 0;
	VOFFSET currentIndex = 0;
};

struct GooseCFG {
	APPID appID = 1000;
	uint32_t TimeAllowedToLive = 5000;
	std::string gocbRef = "simpleIOGenericIO/LLN0$GO$gcbAnalogValues";
	std::string dataSet = "simpleIOGenericIO/LLN0$AnalogValues";
	uint32_t ConfRev = 1;
	std::string mac = "01:0C:CD:01:01:01";
	std::string netinterface = "eth0";
	std::string ineth = "eth0";
	std::string outeth = "eth0";
	std::map<APPID, GCFGItem> GItems;
};

namespace DTU
{
	//-> GOOSE配置
	class dtuGooseCfg
	{
		private:
			dtuGooseCfg(){}
		
		public:
			static dtuGooseCfg& instance() {
				static dtuGooseCfg cfg;
				return cfg;
			}

		public:
			//-> 从文件载入配置
			bool load(const std::string& file);
			//-> 读取GOOSE配置
			const GooseCFG& GetGooseCFG();
			//-> 修改GOOSE配置
			GooseCFG& ModifyGooseCFG();
			//-> 根据APPID查找偏移
			VOFFSET findOffset(APPID appid, GooseValueType type);

		private:
			//-> GOOSE配置
			GooseCFG Goosecfg;
	};

	//-> GOOSE表管理
	class dtuGooseTableMgr
	{
		private:
			dtuGooseTableMgr() {
				isTableReady = false;
			}
		
		public:
			static dtuGooseTableMgr& instance() {
				static dtuGooseTableMgr mgr;
				return mgr;
			}

		public:
			//-> 更新GOOSE表,如果表中值有改变则进行下发
			void updateGooseValues(APPID appID, const GooseData& newValue);
			//-> 从DSP中读取GOOSE表,刷新本地数据
			bool updateGooseTableFromDSP();
			
		
		private:
			//-> 从DSP中读取GOOSE表,刷新本地数据
			bool updateGooseTableFromDSP(const GooseData& newValue);
			//-> 将GOOSE值更新到DSP
			bool updateDSPGooseValues();

		private:
			//-> 查找偏移量
			VOFFSET findOffsetByAppID(APPID appid, GooseValueType type);

		private:
			std::atomic_bool isTableReady;
			GooseAllTable GooseTable = {
				0xAA,
				0,0,0,0,0,0,
				0,0,0,0,0,0,
				0,0,0,0,0,0,
				0,0,0,0,0,0,
				0,0,0,0,0,0,
				0xBB,
			};
	};
}

#endif /* _DTU_GOOSE_CONFIG_H_ */