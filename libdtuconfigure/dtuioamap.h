/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  sdlioamap.h
  *Description:
    读写配置
  *History:
    1, 创建, lhy, 2023-01-16
**********************************************************************************/
#ifndef _SDL_IOA_MAP_H_
#define _SDL_IOA_MAP_H_

#include "pugixml/pugixml.hpp"

#include "dtudatastruct.h"

#ifndef _WIN32
#define SDL_IOA_FILE_PATH "/config/IOAMapTable.xml"
#else
#define SDL_IOA_FILE_PATH "\\config\\IOAMapTable.xml"
#endif

namespace DTU
{
    class IOAMap
    {
    private:
        IOAMap() {}

    public:
        static IOAMap *instance() {
            static IOAMap ins;
            return &ins;
        }

    public:
        /**
         * @brief 从文件种读取配置
         * 
         * @param filePath 文件相对路径
         * @return true 读取成功
         * @return false 读取失败
         */
        bool readIOAFromFile(const std::string& filePath = SDL_IOA_FILE_PATH);
        /**
         * @brief 保存配置到文件
         * 
         * @param filePath 文件相对路径
         * @return true 保存成功
         * @return false 保存失败
         */
        bool saveIOAToFile(const std::string& filePath = SDL_IOA_FILE_PATH);
        /**
         * @brief 通过内部硬件点表查找规约点表
         * 
         * @param hioa 硬件点表
         * @param ioa 规约点表
         * @return true 查找成功
         * @return false 未找到对应点表关系
         */
        bool mapHIOAtoIOA(const HIOA& hioa, IOA& ioa, CA ca);
        /**
         * @brief 通过规约点表查找内部硬件点表
         * 
         * @param ioa 规约点表
         * @param hioa 硬件点表
         * @return true 查找成功
         * @return false 未找到对应点表关系
         */
        bool mapIOAtoHIOA(const IOA& ioa, HIOA& hioa, CA ca);
        /**
         * @brief 通过规约点表查找内部硬件点表
         * 
         * @param ioa 规约点表
         * @param hioa 硬件点表
         * @return true 查找成功
         * @return false 未找到对应点表关系
         */
        bool mapIOAtoHIOA(const IOA& ioa, HIOA& hioa);
        /**
         * @brief 查找IOA来自于哪一个设备 CA等于0为本机
         * 
         * @param ioa 要查找的外部点表
         * @param ca 单元地址(结果)
         * @return true 正确的查找
         * @return false 未找到
         */
        bool whereIOAFrom(const IOA& ioa, CA& ca);
        /**
         * @brief 判断HIOA是否存在
         * 
         * @param hioa 硬件IOA地址
         * @return true 存在
         * @return false 不存在或不需要上送
         */
        bool isHIOAExist(const HIOA& hioa);
        /**
         * @brief 判断IOA是否存在
         * 
         * @param ioa IOA
         * @return true 存在
         * @return false 不存在
         */
        bool isIOAExist(const IOA& ioa);
        /**
         * @brief IOA是否为备用
         * 
         * @param ioa IOA地址
         * @return true 备用或未找到
         * @return false 非备用
         */
        bool isIOASpare(const IOA& ioa);
        /**
         * @brief IOA是否在突变的情况下主动上送
         * 
         * @param ioa IOA地址
         * @return true 需要主动上送
         * @return false 不需要主动上送或未找到
         */
        bool isIOAActive(const IOA& ioa);
        /**
         * @brief 查找CA号下所有备用点表
         * 
         * @param ca CA公共地址
         * @return std::vector<IOA> 备用点表集
         */
        std::vector<IOA> findIOASpare(CA ca);
        /**
         * @brief 按类型查找CA号下所有备用点表
         * 
         * @param ca CA公共地址
         * @param type 类型
         * @return std::vector<IOA> 备用点表集
         */
        std::vector<IOA> findIOASpareWithType(CA ca, IOAMAPTYPE type);
        std::vector<IOA> findIOASpareWithType(IOAMAPTYPE type);
        AllIOAMap &getAllMAPIOA();
        /**
         * @brief 查询IOA是否为备用
         * 
         * @param ioa 备用IOA
         * @return true 是备用
         * @return false 不是备用 或 未找到
         */
        bool testYTSpare(const IOA& ioa);
        //bool testYTis();

    private:
        AllIOAMap ioamap;
    };
}

#endif  /* _SDL_IOA_MAP_H_ */