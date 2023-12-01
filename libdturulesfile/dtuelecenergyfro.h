/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuelecenergyfro.h
  *Description:
    用于实现dtu潮流反向冻结电能量文件生成
  *History:
    1, 创建, lhy, 2022-08-11
**********************************************************************************/
#ifndef _DTU_ELECENERGY_FRO_H_
#define _DTU_ELECENERGY_FRO_H_
#include <string>

#include <pugixml/pugixml.hpp>

#include <dtubuffer.h>

namespace DTU
{
    class DFroRcd {
        private:
            DFroRcd();

        public:
            static DFroRcd& instance() {
                static DFroRcd fro;
                return fro;
            }
        
        public:
            void set_mode(uint16_t addr, uint16_t mode);
            void add_fro_rcd(DTU::buffer &data);

        private:
            bool open_fro_file(const std::string &fullPath);
            void init_fro_file(const std::string &fullPath);
        
        private:
            pugi::xml_document _doc;
            // 当前操作的文件名
            std::string _CurrentFile;

            uint16_t _addr;
            uint16_t _mode;

    };
}

#endif /* _DTU_ELECENERGY_FRO_H_ */