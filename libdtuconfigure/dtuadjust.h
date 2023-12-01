/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuadjust.h
  *Description: 
    DTU的整定相关内容
  *History: 
    1, 创建, lhy, 2022-09-07
**********************************************************************************/
#ifndef _DTU_ADJUST_H_
#define _DTU_ADJUST_H_

#include <vector>

// 整定操作数
#define CALIBRATION_STEP_NUMBER 5

typedef struct analog_channel_param {
	analog_channel_param() {};
	analog_channel_param(std::uint32_t n, std::uint32_t type, std::string str = "") :
		_chNo(n), _type(type), _chName(str) {
	}
	std::uint32_t _chNo;		// 通道号
	std::string   _chName;		// 通道名称
	int _type;

	enum CHANNELTYPE {
		elVoltage = 0,
		elCurrent = 1,
		elUnknown = 2
	};
	int _curType = 0;

	enum PASSAGEWAY {
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
} CHN_ANALOG;

typedef std::vector<CHN_ANALOG> ANALOGLIST;

#endif