/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtureportstorage.cpp
  *Description: 
    基于文件映射的参数定值存储系统
  *History: 
    1, 创建, wangjs, 2021-8-9
**********************************************************************************/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "dtureportstorage.h"

#include <dtulog.h>
#include <fstream>
#include <iostream>
#include "dtustorage.h"
#include "dtureporthelper.h"
#include "dtucmdcode.h"
#include <dtucommon.h>

#ifdef _WIN32
#include <stdlib.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

using namespace DTU;
// 加载
bool DReportStorage::load(const DRptCfg& cfg)
{
    DTULOG(DTU_INFO, (char*)"加载报告文件:0x%04X,%s...", cfg._id, cfg._fileName.c_str());
    _cfg = cfg;
    std::ifstream f(cfg._fileName);
    if (!f.good())
    {
        // 创建文件，并写入文件头
        std::ofstream ofs(cfg._fileName, std::ios::binary | std::ios::out);
        ofs.seekp(cfg._max_item*cfg._item_size+2*sizeof(_header));
        ofs.write("", 1);
        ofs.close();

        std::error_code error;
        _rw_mmap = mio::make_mmap_sink(
            cfg._fileName, 0, mio::map_entire_file, error);
        if (error)
        {
            return false;
        }
        _header._currentIndex = 0;
        _header._totleCount = 0;
        _header._currentNo = 1;
        _header._countNo = 1;

        save_header();

        return true;
    }
    std::error_code error;
    _rw_mmap = mio::make_mmap_sink(
        cfg._fileName, 0, mio::map_entire_file, error);
    if (error)
    {
        return false;
    }
#ifdef _WIN32
    memcpy_s(&_header, sizeof(_header), _rw_mmap.begin(), sizeof(_header));
#else
    memcpy(&_header, _rw_mmap.begin(), sizeof(_header));
#endif  
    return true;
}
// 获取当前条目
buffer DReportStorage::get_current_item()
{
    buffer rptdata;
    if (_header._totleCount != 0)
    {
        std::lock_guard<std::mutex> lock(_rw_lock);
        rptdata.append(_rw_mmap.begin()+sizeof(_header)+(_header._currentIndex-1)*_cfg._item_size, _cfg._item_size);
    }
    return std::move(rptdata);
}
// 按序号读取报告
buffer DReportStorage::get_item_by_no(uint64_t no)
{
    // 序号转索引
    uint32_t index = get_index_by_no(no);
    return get_item_by_index(index);
}
// 按索引读取报告
buffer DReportStorage::get_item_by_index(uint32_t index)
{
    DTU_USER()
    buffer rptdata;
    if(index == _header._totleCount)
    {
        index--;
    }
    if (index >= _header._totleCount){
        DTU_THROW((char*)"get_item_by_index 索引值:%u 大于总数%u, 文件:%s", index, _header._totleCount,_cfg._fileName.c_str());
    }
    {
        std::lock_guard<std::mutex> lock(_rw_lock);
        rptdata.append(_rw_mmap.begin()+sizeof(_header)+ index *_cfg._item_size, _cfg._item_size);
    }
    return std::move(rptdata);
}
// 添加报告
void DReportStorage::add_item(const buffer& rpt)
{
    DTU_USER()
    if (rpt.size() != _cfg._item_size)
    {
        DTU_THROW((char*)"add_item 报告长度有误%u, 正确值:%u, 文件:%s", rpt.size(), _cfg._item_size, _cfg._fileName.c_str());
    }

#ifndef _WIN32
    //在覆盖原索引前取出并删除文件
    static std::map<uint16_t,std::tuple<bool,std::vector<std::string>>> clearPath = {
        {RAM_FLAG_PROTRECORDER, {true,{"/COMTRADE/", "/FACTORY/"}}},
        {RAM_FLAG_TRANSRECORDER, {true,{"/protect/comtrade/","/protect/factory/"}}},
        {RAM_FLAG_PROTACT, {false,{"/protect/protect/"}}}
    };

    // 判断id是否需要清理文件
    auto fret = clearPath.find(_cfg._id);
    if(fret != clearPath.end())
    {
        // 判断curNox是否大于max若大于则需要清理文件
        if(_header._currentNo > _cfg._max_item)
        {
            // 判断是录波文件还是动作报告文件
            if(std::get<0>(fret->second))
            {
                // 录波文件清理
                auto result = get_item_by_index((_header._currentNo - 1) % _cfg._max_item);
                std::string filename(result.get(sizeof(uint64_t),64).const_data(),  result.get(sizeof(uint64_t),64).size());
                for(auto item : std::get<1>(fret->second))
                {
                    std::string cmd = "rm -rf " + get_exec_dir() + item + std::string(filename.c_str()) + "*";
                    system(cmd.c_str());
                }
            }
            else
            {
                // 动作报告文件清理
                auto result = get_item_by_index((_header._currentNo - 1) % _cfg._max_item);
                std::string fileName(result.get(sizeof(uint64_t), 40).const_data(), 40);
                std::string filePath = (std::get<1>(fret->second))[0];
                std::string cmd = "rm -rf " + get_exec_dir() + filePath + fileName;
                system(cmd.c_str());
            }
        }
    }
#endif

    {
        std::lock_guard<std::mutex> lock(_rw_lock);
    #ifdef _WIN32
        memcpy_s(_rw_mmap.begin()+sizeof(_header)+_header._currentIndex*_cfg._item_size, _cfg._item_size, 
            rpt.const_data(), _cfg._item_size);
    #else
        memcpy(_rw_mmap.begin()+sizeof(_header)+_header._currentIndex*_cfg._item_size,
            rpt.const_data(), _cfg._item_size);
    #endif
    }

    DTULOG(DTU_INFO,(char*)"添加报告:%u, 当前索引:%u, 序号%llu", _cfg._id,  _header._currentIndex, _header._currentNo);

    _header._currentIndex++;
    _header._currentNo++;
    _header._totleCount++;

    _header._countNo++;

    // 文件编号上限9999条
    if(_header._countNo > 9999)
    {
        _header._countNo = 1;
    }

    // 循环存储, 从头开始覆盖
    if (_header._currentIndex == _cfg._max_item)
    {
        _header._currentIndex = 0;
    }

    if (_header._totleCount > _cfg._max_item)
    {
        _header._totleCount = _cfg._max_item;
    }
    save_header();
}
// 清空报告
void DReportStorage::clear()
{
    DTU_USER()
    std::lock_guard<std::mutex> lock(_rw_lock);
    memset(_rw_mmap.begin(), 0, _rw_mmap.mapped_length());
    _header._currentIndex = 0;
    _header._currentNo = 1;
    _header._totleCount = 0;
#ifdef _WIN32
    memcpy_s(_rw_mmap.begin(), sizeof(_header), &_header, sizeof(_header));
#else
    memcpy(_rw_mmap.begin(), &_header, sizeof(_header));
#endif
    std::error_code error;
	_rw_mmap.sync(error);
	if (error) {	
        const auto& errmsg = error.message();
	    DTU_THROW((char*)"清空文件错误 %s, %d, %s", errmsg.c_str(), __LINE__,__FILE__);
    }
}
void DReportStorage::save_header()
{
    DTU_USER()
    std::lock_guard<std::mutex> lock(_rw_lock);
    if (_rw_mmap.empty()){
        return;
    }
#ifdef _WIN32
    memcpy_s(_rw_mmap.begin(), sizeof(_header), &_header, sizeof(_header));
#else
    memcpy(_rw_mmap.begin(), &_header, sizeof(_header));
#endif
    std::error_code error;
    _rw_mmap.sync(error);
    if (error) 
    {
        DTU_THROW((char*)"保存报告头部失败:%s", _cfg._fileName.c_str());
    }
}
// TOFIX:删除文件的思路
void DReportStorage::flush(const buffer& data)
{
	// 路径\\要前不要后
    // 开始清理 直接删除文件
	// 获取配置文件路径
	bool isExecute = false;
#ifndef WIN32
	char *workpath = getcwd(NULL, 0);
#else
	char *workpath = _getcwd(NULL, 0);
#endif // !WIN32
	if (workpath == NULL)
	{
		DTULOG(DTU_ERROR, (char*)"清理文件发生错误: %s", strerror(errno));
	}
	std::string exeFullPath(workpath);
	std::string cmd;

#ifdef _WIN32
		cmd = "del /q ";
#else
		cmd = "rm -rf ";
#endif
	
    if(/* DTU::REPORTDB::instance().ActionID == */  RAM_FLAG_PROTACT)	//清理动作报告
    {
		isExecute = true;
#ifdef _WIN32
        exeFullPath = exeFullPath + "\\protect\\protect\\*";
#else
		DTULOG(DTU_INFO, (char*)"删除保护动作报告...");
		exeFullPath = exeFullPath + "/protect/protect/*";
#endif
		cmd = cmd + exeFullPath;
    }
    else if(/* DTU::REPORTDB::instance().ActionID ==*/  RAM_FLAG_TRANSRECORDER)//暂态录波
    {
		isExecute = true;
		std::string Protect_Path;
		std::string COMTRADE_Path;
		std::string FACTORY_Path;
#ifdef _WIN32
		Protect_Path = exeFullPath + "\\protect";
		COMTRADE_Path = Protect_Path + "\\comtrade\\*";
		FACTORY_Path = Protect_Path + "\\factory\\*";
#else
		DTULOG(DTU_INFO, (char*)"删除暂态录波文件...");
		Protect_Path = exeFullPath + "/protect";
		COMTRADE_Path = Protect_Path + "/comtrade/*";
		FACTORY_Path = Protect_Path + "/factory/*";
#endif
		cmd = cmd + COMTRADE_Path + " " + FACTORY_Path;
    }
    else if(/* DTU::REPORTDB::instance().ActionID ==*/  RAM_FLAG_PROTRECORDER)//业务录波
    {
		isExecute = true;
		std::string COMTRADE_Path;
		std::string FACTORY_Path;
#ifdef _WIN32
        COMTRADE_Path = exeFullPath + "\\COMTRADE\\*";
        FACTORY_Path = exeFullPath + "\\FACTORY\\*";
#else
		DTULOG(DTU_INFO, (char*)"删除业务录波文件...");
        COMTRADE_Path = exeFullPath + "/COMTRADE/*";
        FACTORY_Path = exeFullPath + "/FACTORY/*";
#endif
		cmd = cmd + COMTRADE_Path + " " + FACTORY_Path;
    }
    
	//执行删除文件
	if (isExecute)
	{
		system(cmd.c_str());
	}
	
    //清理索引文件
    clear();
    if (data.size() == 0){
        return;
    }
    std::lock_guard<std::mutex> lock(_rw_lock);
    if (_rw_mmap.empty()){
        return;
    }
#ifdef _WIN32
    memcpy_s(_rw_mmap.begin(), data.size(), data.const_data(), data.size());
	memcpy_s(&_header, sizeof(_header), _rw_mmap.begin(), sizeof(_header));
#else
    memcpy(_rw_mmap.begin(), data.const_data(), data.size());
	memcpy(&_header, _rw_mmap.begin(), sizeof(_header));
#endif
}

uint32_t DReportStorage::get_index_by_no(uint64_t serialno)
{
    // 大于总的序号 或 序号为0  则索要最新一条
    if(serialno > _header._currentNo || serialno==0)
    {
        serialno = _header._currentNo;
    }

    // 当前总计数大于上限计数
    if(serialno > (uint64_t)(_cfg._max_item))
    {
        uint64_t index = serialno % (uint64_t)(_cfg._max_item + 1);
        return (uint32_t)index - 1;
    }
    else
    {
        return serialno - 1;
    }
}

DReportHeader DReportStorage::get_header()
{
	return _header;
}

void DReportStorage::get_content(buffer& data){
    data.remove();
    std::lock_guard<std::mutex> lock(_rw_lock);
    data.append(_rw_mmap.begin(), _rw_mmap.mapped_length());
}

uint32_t DReportStorage::get_item_size()
{
    return _cfg._item_size;
}