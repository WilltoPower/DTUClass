/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.cpp
  *Description: 
    实现基于SPDLOG的日志功能的封装
  *History: 
    1, 创建, wangjs, 2021-7-2
**********************************************************************************/
#include "dtulog.h"
#include <stdarg.h>
#include <string>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#ifndef _WIN32
#include <execinfo.h>
#endif
#ifdef _WIN32
#include<direct.h>
#endif

#define DEBUG_LOG

// #ifdef _WIN32
// std::string GetProgramDir()
// {
//     //#ifdef _WIN32
// 	char exeFullPath[MAX_PATH]; // Full path
// 	std::string strPath = "";

// 	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
// 	strPath = (std::string)exeFullPath;    // Get full path of the file
// 	int pos = strPath.find_last_of('\\', strPath.length());
// 	return strPath.substr(0, pos);  // Return the directory without the file name
// }
// #endif
class sdllog {
private:
    sdllog(){
        // 创建日志目录
        char szBuf[128] = {};

    // #ifndef _WIN32
    //     getcwd(szBuf, 128);
    //     std::string fullpath = std::string(szBuf) + "/LD/LOG/";
    //     mkdir(fullpath.c_str(), 0777);
    // #else
    //     std::string fullpath = GetProgramDir() + "\\" + SDL_LOG_PATH;
    //     std::string cmd = "md " + fullpath;
    //     system(cmd.c_str());
    // #endif
        //
        spdlog::init_thread_pool(16, 2);
        // 控制台显示
        //auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto ptr_stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        //	日志文件
    #ifdef _WIN32
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("SDL9200.log", 1024 * 1024 * 10, 3);
    #else
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("SDL9200.log", 1024 * 1024 * 10, 3);
    #endif
        std::vector<spdlog::sink_ptr> sinks{ ptr_stdout_sink, rotating_sink };
        auto logger = std::make_shared<spdlog::async_logger>(LOG_PREFIX, sinks.begin(), sinks.end(),
            spdlog::thread_pool(), spdlog::async_overflow_policy::block);

        logger->flush_on(spdlog::level::info);
        //spdlog::flush_every(std::chrono::seconds(3));
        spdlog::register_logger(logger);
        spdlog::set_level(spdlog::level::debug); // Set global log level to info
    }
public:
    static sdllog* GetInstance() {
        static sdllog log;
        return &log;
    }
    ~sdllog(){
        spdlog::drop_all();
    }
    void write_log(const std::string& info, int level){

        auto logger = spdlog::get(LOG_PREFIX);
        if (level == DTU_WARN) {
            logger->warn(info);
        }
        else if (level == DTU_INFO)
        {
            logger->info(info);
        }
        else if (level == DTU_ERROR)
        {
            logger->error(info);
        }
        else if (level == DTU_DEBUG)
        {
#ifdef DEBUG_LOG
            logger->debug(info);
#endif
        }
    }
};

void DTULOG(int level, const char* format, ...)
{
    char buff[1024] = {};
	va_list   arg_ptr;   //定义可变参数指针 
	va_start(arg_ptr, format);   // i为最后一个固定参数
	vsprintf(buff, format, arg_ptr);
	va_end(arg_ptr);        //  清空参数指针
	sdllog::GetInstance()->write_log(std::move(std::string(buff)), level);
}