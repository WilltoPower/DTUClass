/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtustorage.h
  *Description: 
    管理所有的定值，报告，录波数据等存储
  *History: 
    1, 创建, wangjs, 2021-8-11
**********************************************************************************/
#ifndef _DTU_STORAGE_H
#define _DTU_STORAGE_H
#include <dtubuffer.h>
#include <dtustructs.h>
#include "dtustoragedef.h"
#include <mutex>
#include <functional>
#include "dtudbmanager.h"
namespace DTU
{
    using DSPSendFunc = std::function<int(uint16_t, const DTU::buffer&, uint16_t)>;
    class DSTORE
    {
    public:
        static DSTORE& instance(){
            static DSTORE store;
            return store;
        }
        ~DSTORE();
        // 加载全部的存储设置
        void load(std::string fullpath);
        ////////////////////////////////////////
        /**
         * @brief 保存定值到数据库
         * 
         * @param id ParamID号
         * @param data 定值数据
         * @param group 组号
         */
        void write_setting_data(uint16_t id, const DTU::buffer& data, uint32_t group);
        //@brief 保存定值到数据库
        // int write_setting_data(uint16_t id, uint16_t vid, const DTU::buffer& data, uint32_t group);
        /**
         * @brief 从数据库读取定值
         * 
         * @param pid ParamID号
         * @param data 定值数据
         * @param group 组号
         */
        void read_setting_data(uint16_t pid, DTU::buffer& data, uint32_t group);
        /**
         * @brief 定值区切换
         * 
         * @param group 目标组号
         * @param func 下发定值功能函数
         * @param reboot 是否重启(如无特殊要求默认为1)
         * @return true 切换成功
         * @return false 切换失败
         */
        bool change_setting_num(uint16_t group,DSPSendFunc func,uint16_t reboot = 1);
        /**
         * @brief 定值区切换到数据库记录的当前定值区
         * 
         * @param func 下发定值功能函数
         * @param reboot 是否重启(如无特殊要求默认为1)
         * @return true 切换成功
         * @return false 切换失败
         */
        bool change_default_setting_num(DSPSendFunc func,uint16_t reboot = 1);
		// 读取当前定值区号
		uint32_t get_current_group();
        // 获取最大区号
        uint32_t get_max_group();
        // 获取当前编辑区
        uint32_t get_current_edit_group();
        // 选择预设值要放置的定值区
        void seletc_edit_sg(uint32_t group);
        // 预设定值
        bool preset_setting_data(const DTU::buffer& data);
        // 撤销预设
        void revert_setting_data();
        // 写入预设
        void save_presetting_data(DSPSendFunc func,uint16_t reboot = 1);
        // 通过命令获取Paramid值
        uint16_t get_paramid_by_cmd(uint16_t cmd);
        #define GET_PARAM_ID_BY_CMD(cmd) DTU::DSTORE::instance().get_paramid_by_cmd(cmd)
        // 定值区复制
        bool setting_copy(uint32_t src,uint32_t dest);

    public:
        ////////////////////////////////////////报告相关
        // 添加报告数据
        void add_report_data(uint16_t reportid, uint32_t s, uint32_t ms, const DTU::buffer& data);
        // 根据序号读取报告(序号从1开始)
        DTU::buffer get_report_by_serial(uint16_t reportid, uint32_t serialno);
        // 获取当前最新的报告序号
        // TOTEST:测试所有包含这个函数的位置
        uint32_t get_cur_report_no(uint16_t reportid);
        // 清空报告
        bool clear_report(uint16_t reportid);
        // 获取报告长度
        uint32_t get_report_itemsize(uint16_t reportid);
        // 通过命令获取ReportID
        uint16_t get_reportid_by_cmd(uint16_t cmd);
        // 按范围读取报告
        void get_report_range(uint16_t reportid, int min, int max, DTU::ReportBufferAttr& data);

    public:
#ifndef _WIN32
        //////////////////////////////////////
        // 添加档案数据
        void add_achives_data(uint16_t id, const DTU::buffer& data);
        // 模式
        uint16_t _mode;
#endif
        // 间隔号
        uint16_t _address;
        ////////////////////////////////////////
        // 整定参数
        void load_adj_data();

        void write_adj_data(const DTU::buffer& data);

        DTU::buffer read_adj_data();

        void save_adj_data();

        ////////////////////////////////////////
        // 自检保存
        #if !_WIN32
        void save_selfcheck_data(const DTU::buffer& data);
        void init_selfcheck_data(std::string FullPath);
        #endif
    private:
        // 整定参数
        DTUParamAdjust _adj;
        std::mutex _adj_lock;

    public:
        bool PreSettingFlag = true;  // 同一主站，定值是否预设过标志，同一主站定值如果未被预设则不会执行
        uint64_t curTime = 0x00;
        bool PreSettingFirstFlag = false; // 是否被A主站预置,如果被A主站预置,则B主站预置操作将不会执行，直到A主站确认操作
        // 现在是一个一个主站预置定值后,再预置就不会有反应
        int curOptStation = 0x00; // 当前正在操作的主站(主站的IP地址) 断线会清空这个地址 当是0的时候才会进行预设操作,否则直接返回成功
    };
};
#endif