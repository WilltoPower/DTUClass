/*
工具相关的配置
*/

#pragma once
#include <string>

#include "dtusystemconfig.h"

#define Get_RPC_CFG() dtutoolcfg::instance().GetRPCCFG()

#define Modify_RPC_CFG() dtutoolcfg::instance().ModifyRPCCFG()

class dtutoolcfg
{
	private:
		dtutoolcfg();

	public:
		static dtutoolcfg& instance() {
			static dtutoolcfg sys;
			return sys;
		}

	public:
		// 从文件载入配置
		void load();
		// 保存配置
		void save();
    
		const DTUCFG::EthernetCFG &GetRPCCFG();
		DTUCFG::EthernetCFG &ModifyRPCCFG();

		// 获取波形解析软件路径
		const std::string GetAnalyzeToolPath();

	private:
		// PRC配置
		DTUCFG::EthernetCFG _rpc;
		// 波形解析软件路径
		std::string _analyzeToolPath;
		// 文件完整路径
		std::string _fullPath;
};