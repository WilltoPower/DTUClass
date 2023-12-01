#ifndef _DTU_SOE_MAP_H_
#define _DTU_SOE_MAP_H_

#include <mutex>
#include <map>

#include "dtudatastruct.h"

namespace DTU
{
    class SOEMap
    {
    private:
        SOEMap() {}

    public:
        static SOEMap *instance() {
            static SOEMap ins;
            return &ins;
        }

    public:
        /**
         * @brief 需要在数据库之后进行初始化
         * 
         */
        void init();
        /**
         * @brief 更新遥信状态
         * 
         * @param devioa DSP硬件地址
         * @param state 状态
         * @return true 更新成功
         * @return false 未找到或更新失败
         */
        bool updateYXStateByDevno(uint16_t devioa, bool state);
        bool updateYXStateByHIOA(HIOA hioa, bool state);
        /**
         * @brief 获取遥信状态
         * 
         * @param hioa 硬件IOA
         * @param state 遥信状态(结果)
         * @return true 获取成功
         * @return false 未找到或更新失败
         */
        bool getYXState(HIOA hioa, bool& state);
        /**
         * @brief 获取软遥信描述
         * 
         * @param ioa 
         * @return std::string 
         */
        std::string getYXDesc(HIOA hioa);
        /**
         * @brief 获取所有状态
         * 
         * @return std::map<HIOA, bool>& 获取所有压板状态
         */
        std::map<HIOA, bool> &getAllState();

    private:
        std::map<HIOA, bool> soemap;
        std::map<HIOA, std::string> soestrmap;
        std::mutex lock;

    };
}

#endif