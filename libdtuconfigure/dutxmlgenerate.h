#ifndef _DTU_GOOSE_XML_GENERATE_H_
#define _DTU_GOOSE_XML_GENERATE_H_

#include <string>
#include <vector>
#include <map>

namespace DTU
{

struct GoosePlusSide {
	bool use;
	int appid;
};

struct GoosePlusEth {
	std::string lan;
};

struct OneDevGoosePlus {
	int devno;
	int appid;
	std::string mac;
	std::string ineth;
	std::string outeth;
	std::vector<GoosePlusSide> mside;
	std::vector<GoosePlusSide> nside;
};

using GooseCFGPlus = std::map<int, OneDevGoosePlus>;

class dtuGooseCFGPlus {
	public:;
		//从文件载入配置 
		bool load(const std::string& fullPath);
		GooseCFGPlus &config();

	private:
		GooseCFGPlus DevGoosePlus;
};

}

#endif