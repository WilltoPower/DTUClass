/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtufixrecorder.h
  *Description: 
    保存定点数据
  *History: 
    1, 创建, wangjs, 2021-8-20
    2, 加入获取定点文件列表的功能， wangjs, 2021-8-23
**********************************************************************************/
#ifndef _DTU_FIX_RECORDER_H
#define _DTU_FIX_RECORDER_H
#include <dtubuffer.h>
#include <mio/mmap.hpp>
#include <pugixml/pugixml.hpp>
namespace DTU
{
    class DFixRcd
    {
    public:
        static DFixRcd& instance(){
            static DFixRcd rcd;
            return rcd;
        }
    private:
        DFixRcd();
    public:
        // 生成定点文件
        void form_fix_rcd();
        // 停止
        void stop_form_fix_rcd();
        // 获取当前定点文件目录
        void get_fix_dir(DTU::buffer& files);
        //
        void set_mode(uint16_t addr, uint16_t mode);
    private:
        // 打开定点文件
        bool open_fix_file(const std::string& file);
        // 初始化文件
        void init_fix_file(const std::string& file);
        // 
        void complete_fix_file();
        // 添加定点数据
        void add_fix_rcd(uint32_t index, std::string fixtime);
    private:
        pugi::xml_document _doc;
        std::atomic_bool _bstop;
        std::string _current_file;

        uint16_t _addr;
        uint16_t _mode;
    };
};
#endif