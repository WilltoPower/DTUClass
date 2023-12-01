/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dturpcserver.h
  *Description: 
    RPC服务端
  *History: 
    1, 创建, wangjs, 2021-8-4
**********************************************************************************/
#ifndef _DTU_RPC_SERVER_H
#define _DTU_RPC_SERVER_H
//#include <rpc/server.h>
#include <rest_rpc/rest_rpc.hpp>
#include <string>
namespace DTU
{
    class DTURPC
    {
    private:
        DTURPC(){}
    public:
        static DTURPC& instance(){
            static DTURPC rpc;
            return rpc;
        }
        bool init(uint16_t port){
            _svr = std::make_unique<rest_rpc::rpc_service::rpc_server>(port,  
                std::thread::hardware_concurrency());
            return (_svr != nullptr);
        }
        // 注册函数
        template<typename T>
        void registerfunc(const std::string& funcName, T func)
        {
            if (_svr)
            {
                _svr->register_handler(funcName, func);
            }
        }
        void async_run(std::size_t worker_threads = 1){
            if (_svr){
//                 _svr->async_run();
                _svr->run();
            }
        }
        void stop(){
            if (_svr){
                // _svr->close_sessions();
                // _svr->stop();
            }
        }
        template<typename T>
        void publish(std::string channel, T& data)
        {
            if (_svr != nullptr)
                _svr->publish(channel, data);
        }
    private:
        std::unique_ptr<rest_rpc::rpc_service::rpc_server> _svr;
    };

};
#endif