/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtucorecorder.h
  *Description: 
    co规约文件形成
  *History: 
    1, 创建, lhy, 2021-11-26
**********************************************************************************/
#ifndef _DTU_CO_RECORDER_H
#define _DTU_CO_RECORDER_H
#include <string>
#include <dtubuffer.h>

#define OC_CMD_SEL 1
#define OC_CMD_OPT 2
#define OC_CMD_CEL 3

#define OC_CMD_SO  0
#define OC_CMD_SC  1
#define OC_CMD_DE  2
namespace DTU
{
    class DCoRcd
    {
    private:
        DCoRcd(){}
    public:
        static DCoRcd& instance(){
            static DCoRcd rcd;
            return rcd;
        }
        void add_co_file(uint16_t fix,int operate,int status = 0);
        //
        void form_co_file();
        //
        void set_mode(uint16_t addr, uint16_t mode);
    private:
        bool get_co_content(const DTU::buffer& data, 
            uint16_t& ioa, std::string& tm,uint16_t& opt, uint16_t& val);
        // uint16_t getRets(uint16_t cmd);
        // int16_t getRetv(uint16_t cmd);
        // std::string getRetstr(uint16_t opt);
        uint16_t _addr;
        uint16_t _mode;
    };
};
#endif