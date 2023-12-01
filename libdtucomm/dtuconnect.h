/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuconnect.h
  *Description: 
    连接状态检测
  *History: 
    1, 创建, wangjs, 2022-12-09
**********************************************************************************/
#ifndef _DTU_CONNECT_H_
#define _DTU_CONNECT_H_

#include <atomic>
#include <thread>
#include <bitset>
#include "dtubuffer.h"

namespace DTU
{
    class DTUConnectTest
    {
    private:
        DTUConnectTest() {
            cs101_state = false;
            cs104_state = false;
        }

    public:
        static DTUConnectTest *instance() {
            static DTUConnectTest ins;
            return &ins;
        }

    public:
        void init();
        void set_cs101_state(bool state);
        void set_cs104_state(bool state);
        void stop();

    public:
        static void setAddDataToQueue(std::function<void (uint16_t, uint16_t, const buffer&)> func);

    private:
        void updateSOE(uint16_t devaddr, int ca, bool state);

    private:
        void run();

    private:
        bool isPublic = false;
        bool lastState = false;
        bool is_use_101 = false;
        bool is_use_104 = false;
        std::atomic_bool cs101_state;
        std::atomic_bool cs104_state;
        std::atomic_bool running;
        std::unique_ptr<std::thread> runThread;
        std::bitset<10> selfcheck;
    };
}

#endif  /* _DTU_CONNECT_H_ */