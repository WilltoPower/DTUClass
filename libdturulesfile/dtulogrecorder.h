/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulogrecorder.h
  *Description: 
    保存日志数据
  *History: 
    1, 创建, wangjs, 2021-8-23
**********************************************************************************/
#ifndef _DTU_LOG_RECORDER_H
#define _DTU_LOG_RECORDER_H
#include <dtubuffer.h>
#include <list>
#include <mutex>
#include <pugixml/pugixml.hpp>
namespace DTU
{
    class DLogRcd
    {
    private:
        DLogRcd(){}
    public:
        static DLogRcd& instance(){
            static DLogRcd rcd;
            return rcd;
        }
        //
        void add_log_data(const buffer& data);
        // 生成日志
        void form_log_file();
        //
        void set_mode(uint16_t addr, uint16_t mode);
    private:
        bool get_log_content(const buffer& data, uint16_t& type, 
            std::string& time, std::string& content, uint16_t& val);
    private:
        std::mutex _queue_lock;
        std::list<buffer> _log_queue;
        //
        uint16_t _addr;
        uint16_t _mode;
    };
};
#endif