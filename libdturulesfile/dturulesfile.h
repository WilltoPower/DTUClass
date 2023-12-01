/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtufixrecorder.h
  *Description: 
    规约相关文件保存
  *History: 
    1, 创建, wangjs, 2021-8-20
**********************************************************************************/
#ifndef _DTU_RULES_FILES_H
#define _DTU_RULES_FILES_H
#include <dtubuffer.h>
namespace DTU
{
    class DRULESFILE
    {
    public:
        static DRULESFILE& instance(){
            static DRULESFILE rules;
            return rules;
        }
    private:
        DRULESFILE(){}
    public:
        // 设置模式和地址
        void set_mode(uint16_t addr, uint16_t mode);

        // 添加极值数据
        void add_exv_data(buffer& data);
        // 读取极值目录
        void get_exv_dir(buffer& data);

        // 添加日志数据
        void add_log_data(buffer& data);
        // 生成日志文件
        void form_log_file();

        // 定点数据服务启动
        void start_fix_service();
        // 定点数据服务停止
        void stop_fix_service();

        // 读取定点目录
        void get_fix_dir(buffer& data);

        // 生成SOE文件
        void form_soe_file();

        // 添加遥控记录
        void add_co_file(uint16_t fix,int opt,int status = 0);
        // 生成CO遥控操作文件
        void form_co_file();

        // 启动冻结电能量服务
        void start_frz_service();
        // 停止冻结电能量服务
        void stop_frz_service();
        // 添加冻结电能量数据
        void add_frz_rcd(buffer& data);

        // 添加潮流反向电能量
        void add_fro_data(buffer& data);
        
    };
};
#endif