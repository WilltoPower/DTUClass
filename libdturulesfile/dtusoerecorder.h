/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtusoerecorder.h
  *Description: 
    soe规约文件形成
  *History: 
    1, 创建, wangjs, 2021-8-23
    2, 加入间隔地址和区分模式，wangjs, 2021-9-24
**********************************************************************************/
#ifndef _DTU_SOE_RECORDER_H
#define _DTU_SOE_RECORDER_H
#include <string>
#include <dtubuffer.h>
namespace DTU
{
    class DSoeRcd
    {
    private:
        DSoeRcd(){}
    public:
        static DSoeRcd& instance(){
            static DSoeRcd rcd;
            return rcd;
        }
        //
        void form_soe_file();
        //
        void set_mode(uint16_t addr, uint16_t mode);
    private:
        bool get_soe_content(const DTU::buffer& data, 
            uint16_t& ioa, std::string& tm, uint16_t& val);

        uint16_t _addr;
        uint16_t _mode;
    };
};
#endif