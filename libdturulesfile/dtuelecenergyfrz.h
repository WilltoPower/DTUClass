/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuelecenergyfrz.h
  *Description:
    用于实现dtu日冻结电能量生成
  *History:
    1, 创建, lhy, 2022-08-11
**********************************************************************************/
#ifndef _DTU_ELECENERGY_FRZ_H_
#define _DTU_ELECENERGY_FRZ_H_
#include <string>
#include <atomic>

#include <pugixml/pugixml.hpp>

#include <dtubuffer.h>

namespace DTU
{
    class DFrzRcd {
        private:
            DFrzRcd();
        public:
            static DFrzRcd& instance() {
                static DFrzRcd frz;
                return frz;
            }
        
        public:
            // 添加电能量报文
            void add_frz_rcd(DTU::buffer &frzBuffer);
            // 设置模式
            void set_mode(uint16_t addr, uint16_t mode);
            // 生成冻结电能量
            void form_frz_rcd();
            // 停止生成冻结电能量
            void stop_form_frz_rcd();

        private:
            bool open_frz_file(const std::string fullPath);
            void init_frz_file(const std::string fullPath);
            void complete_frz_file(DTU::buffer &data);
        
        private:
            void add_first_frz_rcd(DTU::buffer &data);

        private:
            pugi::xml_document _doc;
            // 当前操作的文件名
            std::string _CurrentFile;
            std::atomic_bool _bstop;
            // 当前时间索引
            std::atomic_int _index;

            uint16_t _addr;
            uint16_t _mode;
    };
}

#endif /* _DTU_ELECENERGY_FRZ_H_ */