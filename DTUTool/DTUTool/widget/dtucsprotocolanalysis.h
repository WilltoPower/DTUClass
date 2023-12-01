﻿#pragma once
//#pragma execution_character_set("UTF-8")

#include <map>
#include <tuple>
#include <QString>

#define index_Ti_101 7
#define index_COT_101 9
#define index_Ti_104 6
#define index_COT_104 8

using Ti_t = std::map<uint8_t, std::tuple<QString, QString>>;
using COT_t = std::map<uint8_t, std::tuple<QString, QString>>;
class Analysis{
public:
	QString get_desc_by_Ti(uint8_t Tino,uint16_t no = 0)
	{
		no = 0;
		QString qstr;
		auto ret = Ti.find(Tino);
		if (ret != Ti.end())
		{
			auto info = ret->second;
			if(no == 0)
				qstr = std::get<0>(info);
			else if(no == 1)
				qstr = std::get<1>(info);
			return qstr;
		}
		qstr = "————————";
		return qstr;
	}
	QString get_desc_by_COT(uint8_t COTno, uint16_t no = 0)
	{
		if (COTno != 3 && COTno != 5)
			COTno = 0;

		QString qstr;
		auto ret = COT.find(COTno);
		if (ret != COT.end())
		{
			auto info = ret->second;
			if (no == 0)
				qstr = std::get<0>(info);
			else if (no == 1)
				qstr = std::get<1>(info);
			return qstr;
		}
		qstr = "————";
		return qstr;
	}
private:
	Ti_t Ti = {
	{1,{"单点信息","————"}},
	{3,{"双点信息","————"}},
	{9,{"测量值,归一化值","————"}},
	{11,{"测量值,标度化值","————"}},
	{13,{"测量值,段浮点数","————"}},
	{30,{"带CP56Time2a的单点信息","————"}},
	{31,{"带CP56Time2a的双点信息","————"}},
	{42,{"故障值信息","————"}},
	{45,{"单命令","————"}},
	{46,{"双命令","————"}},
	{70,{"初始化结束","————"}},
	{100,{"总召唤","————"}},
	{101,{"电能量召唤命令","————"}},
	{103,{"时钟同步","————"}},
	{105,{"复位进程","————"}},
	{200,{"切换定值区","————"}},
	{201,{"读定值区","————"}},
	{202,{"读参数和定值","————"}},
	{203,{"写参数和定值","————"}},
	{206,{"累积量,断浮点数","————"}},
	{207,{"带CP56Time2a时标的累积量,段浮点数","————"}},
	{210,{"文件传输","————"}},
	{211,{"软件升级启动/结束","————"}},
	};
	COT_t COT = {
	{1,{"周期/循环","————"}},
	{2,{"背景扫描","————"}},
	{3,{"突发","自发"}},
	{4,{"初始化完成","————"}},
	{5,{"被请求","请求"}},
	{6,{"激活","————"}},
	{7,{"激活确认","————"}},
	{8,{"停止激活","————"}},
	{9,{"停止激活确认","————"}},
	{10,{"激活终止","————"}},
	{13,{"文件传输","————"}},
	{20,{"响应主站召唤","————"}},
	{21,{"响应第1组召唤","————"}},
	{22,{"响应第2组召唤","————"}},
	{23,{"响应第3组召唤","————"}},
	{24,{"响应第4组召唤","————"}},
	{25,{"响应第5组召唤","————"}},
	{26,{"响应第6组召唤","————"}},
	{27,{"响应第7组召唤","————"}},
	{28,{"响应第8组召唤","————"}},
	{29,{"响应第9组召唤","————"}},
	{30,{"响应第10组召唤","————"}},
	{31,{"响应第11组召唤","————"}},
	{32,{"响应第12组召唤","————"}},
	{33,{"响应第13组召唤","————"}},
	{34,{"响应第14组召唤","————"}},
	{35,{"响应第15组召唤","————"}},
	{36,{"响应第16组召唤","————"}},
	{37,{"响应电能量总召唤","————"}},
	{44,{"未知的类型标识符(Ti)","————"}},
	{45,{"未知的传送原因(COT)","————"}},
	{46,{"未知的ASDU公共地址(CA)","————"}},
	{47,{"未知的信息对象地址(IOA)","————"}},
	{48,{"遥控执行软压板状态错误","————"}},
	{49,{"遥控执行时间戳错误","————"}},
	{50,{"遥控执行数字签名认证错误","————"}},
	};
};






// QOI 召唤限定词

// QRP 复位进程命令限定词

// COI 初始化原因QRP
//0 当地电源合上
//1 当地手动复位
//2 远方复位