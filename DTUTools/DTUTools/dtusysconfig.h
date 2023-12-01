/*
工具相关的配置
*/
#pragma once
#include <string>
#include <vector>

#define LOCALIP() \
	dtusysconfig::instance().get_local_ip()

#define DESTIP() \
	dtusysconfig::instance().get_dest_ip()

#define LOCALPORT() \
	dtusysconfig::instance().get_local_port()

#define DESTPORT() \
	dtusysconfig::instance().get_dest_port()

/************************************* 定值整定 *******************************************/
//操作数
#define CALIBRATION_STEP_NUMBER 5

typedef struct analog_channel_param {
	analog_channel_param() {};
	analog_channel_param(std::uint32_t n, std::uint32_t type, std::string str = "") :
		_chNo(n), _type(type), _chName(str) {
	}
	std::uint32_t _chNo;		// 通道号
	std::string   _chName;		// 通道名称
	int _type;
	enum {
		elVoltage = 0,
		elCurrent = 1,
		elUnknown = 2
	};
	int _curType = 0;
	enum {
		AC=0,
		DC=1,
	};
	double rawValues[CALIBRATION_STEP_NUMBER] = {};
	double Values[CALIBRATION_STEP_NUMBER] = {};
	double Phases[CALIBRATION_STEP_NUMBER] = {};
	int isModify = 3;   //是否进行了修改
	int wModify = 0;	//手动修改了哪一个值 isModify = 3时此值无效
	float Kc = 0.0;		// 变比
	float Ka = 0.0;		// 角度偏差
	float X0 = 0.0;		// 截距
	float ZeroExcursion = 0.0;	// 零漂
}CHN_ANALOG;

typedef std::vector<CHN_ANALOG> ANALOGLIST;
/**************************************************************************************/
class dtusysconfig
{
private:
	dtusysconfig(){}
public:
	static dtusysconfig& instance() {
		static dtusysconfig sys;
		return sys;
	}
	void load();
	void save();
	///
	const std::string& get_local_ip() const;
	void set_local_ip(const std::string& ip);
	uint16_t get_local_port() const;
	void set_local_port(uint16_t port);
	const std::string& get_dest_ip() const;
	void set_dest_ip(const std::string& ip);
	uint16_t get_dest_port() const;
	void set_dest_port(uint16_t port);

	uint64_t get_tool_id();

	std::string get_analyze_tools();
	//
private:
	uint64_t _tool_id;

	std::string _rpc_local_ip;
	uint16_t _rpc_local_port;

	std::string _rpc_dest_ip;
	uint16_t _rpc_dest_port;

	std::string _analyze_tools;
};

