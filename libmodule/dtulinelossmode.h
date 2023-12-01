/*********************************************************************************
	*Copyright(C),2021-2025,sddl
	*FileName:  dtulinelossmode.h
	*Description: 
		用于实现线损模块功能
	*History: 
		1, 创建, lhy, 2022-08-10
**********************************************************************************/
#ifndef _DTU_LINE_LOSS_MODE_H_
#define _DTU_LINE_LOSS_MODE_H_

namespace DTU 
{
    class dtuLinelossmode
    {
        public:
            static dtuLinelossmode& instance() {
                static dtuLinelossmode mode;
                return mode;
            }
            ~dtuLinelossmode();
            
        private:
            dtuLinelossmode();

        public:
            bool clockSync();
            void callElecEnergy();

        private:
            int CA = -1;

    };
};

#endif /* _DTU_LINE_LOSS_MODE_H_ */