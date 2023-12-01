#ifndef _DTU_VERSION_H_
#define _DTU_VERSION_H_

#include <string>

#define MAIN_VERSION 20
#define SUB_VERSION 23
#define INNER_VERSION 02
#define STAGE_VERSION 14
#define CMAKE_TIME "2023/11/28 18:19:48"

#define COMPILE_TIME std::string(__DATE__) + " " + std::string(__TIME__)

struct ARMVersion {
	const std::string armversion = "V" + std::to_string(MAIN_VERSION) + ".R" + std::to_string(SUB_VERSION) + ".C" + 
                                std::to_string(INNER_VERSION) + ".B" + std::to_string(STAGE_VERSION);
	const std::string compiletime = COMPILE_TIME;
	const std::string cmaketime = std::string(CMAKE_TIME);
};

#endif
