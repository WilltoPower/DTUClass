/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtufixrecorder.h
  *Description: 
    保存极值数据
  *History: 
    1, 创建, wangjs, 2021-8-20
    2, 加入获取极值文件列表, wangjs, 2021-8-23
**********************************************************************************/
#ifndef _DTU_EXV_RECORDER_H
#define _DTU_EXV_RECORDER_H
#include <dtubuffer.h>
namespace DTU
{
    class DExvRcd
    {
    public:
        static DExvRcd& instance(){
            static DExvRcd rcd;
            return rcd;
        }
    private:
        DExvRcd(){}
        uint16_t _addr;
        uint16_t _mode;
    public:
        void add_exv_data(buffer& data);
        // 极值文件列表
        void get_exv_dir(buffer& data);
        //
        void set_mode(uint16_t addr, uint16_t mode);
    };
};
#endif