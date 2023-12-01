
/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturulesnotify.h
  *Description:
    处理主动上送的规约相关内容
  *History:
    1, 创建, wangjs, 2021-12-23
**********************************************************************************/
#ifndef _DTU_RULES_NOTIFY_H
#define _DUT_RULES_NOTIFY_H
#include <dtuprotocol.h>
namespace DTU
{
    class DRULESNotify
    {
    private:
        DRULESNotify(){}
    public:
        static DRULESNotify& instance(){
            static DRULESNotify notify;
            return notify;
        }
    public:
        /**
         * @brief 规约通知统一接口
         * 
         * @param src 数据
         * @param type 通知类型(101/104)
         * @param isfrombay 通知是否来自间隔单元
         * @param bca 间隔单元地址
         */
        void notify(const dtuprotocol& src, uint32_t type, bool isfrombay = false, int bca = 0);
    private:
        void notify_cos(const dtuprotocol& src, uint32_t type, bool isfrombay = false, int bca = 0);
        void notify_soe(const dtuprotocol& src, uint32_t type, bool isfrombay = false, int bca = 0);
    };
};
#endif