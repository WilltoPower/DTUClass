/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  DTUCmdCode.h
  *Description: 
    定义与DSP通信的命令码
  *History: 
    1, 创建, wangjs, 2021-7-30
    2，加入定值区切换命令和读取当前区号命令, wangjs, 2021-8-16
	  3，加入读取操作记录，告警记录，SOE记录的命令, wangjs, 2021-8-18
    4，加入极值，日志等标志位，修改操作记录，告警，保护动作的标志位, wangjs, 2021-8-19
    5，修改了业务录波和暂态录波的RAM标志, wangjs, 2021-8-26
    6. 加入上送当前定值区号定义,lhy 2021-9-1
    7. 修改命令码,lhy 2022-5-16
**********************************************************************************/
#ifndef _DTU_CMD_CODE_H
#define _DTU_CMD_CODE_H
/*********************************** ARM->DSP通信 ********************************/
// 写操作
// 数据下发                   //XX:完成修改
#define PC_W_CLK                    0xAAA0    //写时钟
#define PC_W_PUB_FIX                0xAAA1    //写公共定值
#define PC_W_YB_ON_OFF_INFO         0xAAA2    //写软压板定值
#define PC_W_PRO_FIX                0xAAA3    //写常规保护定值
#define PC_W_AUTORECLOSE_FIX        0xAAA4    //写自动重合闸定值
#define PC_W_FA_FIX                 0xAAA5    //写就地FA定值
#define PC_W_DFA_FIX                0xAAA6    //写分布式FA定值
#define PC_W_TQHZ_FIX               0xAAA7    //写同期合闸定值
#define PC_W_AUTO_SPLIT_FIX         0xAAA8    //写自动解列定值
#define PC_W_XDLGND_FIX             0xAAA9    //写小电流接地定值
#define PC_W_LINEBRKALARM_FIX       0xAAAA    //写线路断线告警定值
#define PC_W_POWERDRIVER_FIX        0xAAAB    //写传动开关定值
#define PC_W_AUTOCFG_FIX            0xAAAC    //写自动化参数定值
#define PC_W_COMM_FIX               0xAAAD    //写通信定值(LCD使用)
#define PC_W_INT_FIX                0xAAAE    //写设备定值(原内部定值)
#define PC_W_ADJ_LCD_FIX            0xAAAF    //LCD写整定定值(不用)
#define PC_W_ADJ_FIX                0xAAB0    //写整定定值
#define PC_W_GOOSE_TABLE_PKG        0xAAB1    //下发GOOSE表信息

// 命令下发
#define PC_W_SELF_CHECK             0xAAC0    //自检
#define PC_W_RST_SIG_YF             0xAAC1    //远方信号复归
#define PC_W_DSP_REBOOT             0xAAC2    //前置机重启
#define PC_W_YK_SET		              0xAAC3    //遥控预设命令
#define PC_W_YK_CANCLE		          0xAAC4    //遥控取消命令
#define PC_W_YK_FZ_YF		            0xAAC5    //远方遥控分闸命令
#define PC_W_YK_HZ_YF	              0xAAC6    //远方遥控合闸命令

#define PC_W_ACK                    0xAAC7    //回应应答信号
#define PC_W_NACK                   0xAAC8    //回应非应答信号
#define PC_W_CRC_BAD                0xAAC9    //回应CRC校验错误

#define PC_W_YK_FZ_JD		            0xAACA    //就地遥控分闸命令
#define PC_W_YK_HZ_JD	              0xAACB    //就地遥控合闸命令
#define PC_W_RST_SIG_JD             0xAACC    //就地信号复归
#define PC_W_YK_HH_QD	              0xAACD    //活化启动命令
#define PC_W_YK_HH_TC               0xAACE    //活化退出命令
#define PC_W_LED_TX_LINK_UP         0xAACF    //通讯指示灯点亮
#define PC_W_LED_TX_LINK_DOWN       0xAAD0    //通讯指示灯熄灭
#define PC_W_BLK_RST                0xAAD1    //下发闭锁复归命令
#define PC_W_FIX_AREA_NUM           0xAAD2    //下发定值区号(切换定值区,不下发)
#define PC_W_POWER_DRIVER           0xAAD3    //下发开关传动命令
#define PC_W_POWER_DRIVER_SYB_ON    0xAAD4    //开关传动软压板投入
#define PC_W_GOOSE_COMM_YC          0xAAD5    //下发GOOSE通信异常命令
#define PC_W_GOOSE_COMM_RST         0xAAD6    //下发GOOSE通信异常恢复命令
#define PC_W_SETTING_COPY           0xAAD7    //定值区拷贝(不下发)
#define PC_W_POWER_DRIVER_PRE       0xAADC    //开关传动预置

// 测试命令
#define PC_W_TEST_RELAY     		    0xAAD8    //测试继电器
#define PC_W_TEST_LED       		    0xAAD9    //测试LED灯
#define PC_W_TEST_LB        		    0xAADA    //手动录波
#define PC_W_TEST_FORMAT      		  0xAADB    //出厂设置

// 读操作
// 读定值
#define PC_R_PUB_FIX                0xAAE1    //读公共定值
#define PC_R_YB_STATE_INFO          0xAAE2    //读取软压板信息
#define PC_R_PRO_FIX                0xAAE3    //读常规保护定值
#define PC_R_AUTORECLOSE_FIX        0xAAE4    //读自动重合闸定值
#define PC_R_FA_FIX                 0xAAE5    //读FA定值
#define PC_R_DFA_FIX                0xAAE6    //读分布式FA定值
#define PC_R_TQHZ_FIX               0xAAE7    //读同期合闸定值
#define PC_R_AUTO_SPLIT_FIX         0xAAE8    //读自动解列定值
#define PC_R_XDLGND_FIX             0xAAE9    //读小电流接地定值
#define PC_R_LINEBRKALARM_FIX       0xAAEA    //读线路断线告警定值
#define PC_R_POWERDRIVER_FIX        0xAAEB    //读传动开关定值
#define PC_R_AUTOCFG_FIX            0xAAEC    //读自动化参数定值
#define PC_R_COMM                   0xAAED    //读通信定值
#define PC_R_INT_FIX                0xAAEE    //读设备定值
#define PC_R_ADJ_LCD_FIX            0xAAEF    //LCD读整定定值
#define PC_R_ADJ_FIX                0xAB08    //读整定定值

// 读数据
#define PC_R_CLK                    0xAAF0    //读实时时钟
#define PC_R_YC_DATA        	      0xAAF1    //读遥测数据
#define PC_R_HYX_DATA               0xAAF2    //读硬件遥信数据(已废弃,请使用读遥信命令)
#define PC_R_RST               	    0xAAF3    //读复位报告
#define PC_R_CHECK                  0xAAF4    //读自检信息
#define PC_R_OPER_INFO              0xAAF5    //读取操作报告
#define PC_R_PRO_ACT_INFO           0xAAF6    //读保护动作报告
#define PC_R_SOE_INFO               0xAAF7    //读取SOE记录信息
#define PC_R_SOFT_PROG              0xAAF8    //读程序版本号
#define PC_R_LO_SAP_DATA            0xAAF9    //读低速实时采样数据
#define PC_R_LO_ADJ_DATA            0xAAFA    //读低速整定采样数据
#define PC_R_LO_FAUL                0xAAFB    //读低速故障录波数据(保护)
#define PC_R_PROFUN_STATE           0xAAFC    //读保护功能状态信息
#define PC_R_MAIN_MENU_INFO         0xAAFD    //读主界面信息数据
#define PC_R_ALARM_INFO             0xAAFE    //读告警报告
#define PC_R_SYX_INFO               0xAAFF    //读遥信数据(已废弃,请使用读遥信命令)
#define PC_R_COS_DATA               0xAB01    //读COS数据
#define PC_R_ZTLB_DATA              0xAB02    //读取暂态录波数据101相关
#define PC_R_EXV_DATA               0xAB03    //读取极值数据
#define PC_R_LOG_DATA               0xAB04    //读取日志数据
#define PC_R_POP_INFO               0xAB05    //读弹窗信息
#define PC_R_PRI_PRO_INFO           0xAB06    //读私有版本信息
#define PC_R_FIX_AREA_INFO          0xAB07    //读当前定值区信息
#define PC_R_GOOSE_INFO             0xAB09    //读GOOSE包信息
#define PC_R_GOOSE_TABLE_INFO       0xAB0A    //读GOOSE表信息
#define PC_R_XY                     0xAB0B    //读遥信(按字节排布,总长度1024字节)

/*********************************** DSP->DARM通信 ********************************/
// 上送定值
#define  TX_PC_PUB_FIX              0xBB90    //公共定值
#define  TX_PC_SOFT_YB_FIX          0xBB91    //软压板定值
#define  TX_PC_PRO_FIX              0xBB92    //常规保护定值
#define  TX_PC_AUTORECLOSE_FIX      0xBB93    //自动重合闸定值
#define  TX_PC_FA_FIX               0xBB94    //FA定值
#define  TX_PC_DFA_FIX           	  0xBB95    //分布式FA定值
#define  TX_PC_TQHZ_FIX             0xBB96    //同期合闸定值
#define  TX_PC_AUTOSPLIT_FIX        0xBB97    //自动解列定值
#define  TX_PC_XDLGND_FIX           0xBB98    //小电流接地定值
#define  TX_PC_LINEBRKALARM_FIX     0xBB99    //线路断线告警定值
#define  TX_PC_POWERDRIVER_FIX      0xBB9A    //传动开关定值
#define  TX_PC_AUTOCFG_FIX          0xBB9B    //自动化参数定值
#define  TX_PC_COMM_FIX             0xBB9C    //通信定值
#define  TX_PC_INT_FIX              0xBB9D    //设备定值
#define  TX_PC_ADJ_LCD_FIX          0xBB9E    //LCD整定定值
#define  TX_PC_ADJ_FIX              0xBB9F    //整定定值

// 上送数据
#define  TX_PC_CLK                  0xBBB0    //时钟数据
#define  TX_PC_YC_DATA              0xBBC1    //遥测数据
#define  TX_PC_YX_DATA              0xBBC2    //硬件遥信数据
#define  TX_PC_RST_DATA             0xBBC3    //复位信息
#define  TX_PC_SELF_DATA            0xBBC4    //自检信息
#define  TX_PC_OPER_REP_DATA        0xBBC5    //上送操作报告
#define  TX_PC_PRO_ACT_INFO_DATA    0xBBC6    //保护动作报告
#define  TX_PC_SOE_INFO             0xBBC7    //上送SOE
#define  TX_PC_PRV_DATA             0xBBC8    //版本信息

#define  TX_PC_LS_SAP_DATA          0xBBC9    //工频实时数据
#define  TX_PC_LS_ADJ_DATA          0xBBCA    //工频整定数据
#define  TX_PC_LS_LB_DATA           0xBBCB    //工频录波数据(保护)
#define  TX_PC_PROFUN_STATE         0xBBCC    //保护功能状态信息
#define  TX_PC_MAIN_MENU_INFO       0xBBCD    //主界面信息数据
#define  TX_PC_ALARM_REP_INFO       0xBBCE    //上送告警报告信息
#define  TX_PC_SYX_INFO             0xBBCF    //上送软件遥信信息
#define  TX_PC_COS_DATA             0xBBD0    //读COS数据
#define  TX_PC_ZTLB_DATA            0xBBD1    //上送暂态录波数据101
#define  TX_PC_EXV_DATA             0xBBD2    //上送极值数据
#define  TX_PC_LOG_DATA             0xBBD3    //上送日志数据
#define  TX_PC_POP_INFO             0xBBD4    //上送弹窗报告信息
#define  TX_PC_PRI_PRO_INFO         0xBBD5    //上送私有版本信息
#define  TX_PC_LED_STATUS           0xBBD6    //上送指示灯当前状态
#define  TX_PC_FIX_AREA_INFO        0xBBD7    //上送当前定值区信息
#define  TX_PC_GOOSE_INFO          	0xBBD8    //上送GOOSE信息
#define  TX_PC_GOOSE_TABLE_INFO     0xBBD9    //上送GOOSE表信息
#define  TX_PC_XY                   0xBBDA    //上送遥信

//上送应答信号
#define  TX_PC_ACK                  0xBBE0    //ACK
#define  TX_PC_NACK                 0xBBE1    //NACK
#define  TX_PC_CRC_BAD              0xBBE2    //检验错误

// 升级包
#define PC_W_FIR_PACKET             0xAAB7     //写固件升级包
#define PC_W_ARM_PACKET             0xCCF1     //写ARM升级包
#define PC_W_DSP_PACKET             0xCCF2     //写DSP升级包
#define PC_W_CONFIG_PACKET          0xCCF3     //写配置升级包
#define PC_W_ST_RECORD              0xCCFF

/************************************ 标志区 **************************************/
#define RAM_FLAG_GOOSE                0x00000000    // GOOSE包
#define RAM_FLAG_SOE                  0x00000001    // SOE
#define RAM_FLAG_COS                  0x00000002    // COS
#define RAM_FLAG_PROTRECORDER         0x00000003    // 业务录波
#define RAM_FLAG_PROTACT              0x00000004    // 保护动作信息
#define RAM_FLAG_WARN                 0x00000005    // 告警记录
#define RAM_FLAG_OPTRCD               0x00000006    // 操作记录
#define RAM_FLAG_SELFCHK              0x00000007    // 自检数据
#define RAM_FLAG_RESET                0x00000008    // 复位数据(保留)
#define RAM_FLAG_EXTREME              0x00000009    // 极值
#define RAM_FLAG_LOG                  0x0000000A    // 日志
#define RAM_FLAG_TRANSRECORDER        0x0000000B    // 暂态录波

#define NULL_ORDER                    0x0000        //空命令

#define DTU_FILE_PROTACT					    0x0000        // 读取保护动作报告文件
#define DTU_FILE_PROTARC					    0x0001        // 读取保护动作报告档案
#define DTU_FILE_COMTRADE             0x0002        // 读取COMTRADE文件
#define DTU_FILE_WARN						      0x0003	      // 读取告警记录文件
#define DTU_FILE_OPT							    0x0004	      // 读取操作记录文件
#define DTU_FILE_SOE							    0x0005        // 读取SOE记录文件
#define DTU_FILE_TRANSARC					    0x0006	      // 读取暂态录波档案
#define DTU_FILE_WORKSRC					    0x0007    		// 读取业务录波档案
#define DTU_FILE_TRANS                0x0008        // 暂态录波文件
#define DTU_FILE_WORK                 0x0009        // 业务录波文件

#define DTU_CLEAR_PROTARC             0x0010        // 清空录波档案
#define DTU_CLEAR_WARN                0x0011        // 清空告警记录
#define DTU_CLEAR_OPT                 0x0012        // 清空操作记录
#define DTU_CLEAR_SOE                 0x0013        // 清空SOE记录

#define DTU_HISTORY_SOE               0x0020        // 规约SOE
#define DTU_HISTORY_CO                0x0021        // 遥控记录
#define DTU_HISTORY_EXV               0x0022        // 极值文件
#define DTU_HISTORY_FIXPT             0x0023        // 定点文件
#define DTU_HISTORY_LOG               0x0024        // 规约日志
#define DTU_HISTORY_SELFCHECK		  0x0025	    // 自检文件
#define DTU_HISTORY_FIXPTDIR          0x0026        // 定点文件列表
#define DTU_HISTORY_EXVDIR            0x0027        // 极值文件列表



#define F_OPT_DIR           1     // 文件目录读取
#define F_OPT_DIR_ACK       2     // 目录召唤确认
#define F_OPT_RFILE_ACT     3     // 读文件激活
#define F_OPT_RFILE_ACT_ACK 4     // 读文件激活确认
#define F_OPT_RFILE_DATA    5     // 文件数据
#define F_OPT_RFILE_CON     6     // 读文件确认
#define F_OPT_WFILE_ACT     7     // 写文件激活
#define F_OPT_WFILE_ACT_ACK 8     // 写文件激活确认
#define F_OPT_WFILE_DATA    9     // 写文件数据
#define F_OPT_WFILE_CON     10    // 写文件数据确认

// 自定命令
#define DTU_GET_SYS_CONFIG        0xFF01         // 从后台获取系统配置
#define PC_W_ARM_REBOOT           0xFFD1         // ARM重启
#define PC_W_NO_CMD               0x0000         // 无命令
#define PC_W_FIX_AREA_INFO        0xFFD2         // 写定值区
#define RAM_FLAG_CO               0x0000000C     // 命令区
#define DTU_GET_PROTO_CONFIG	  0xFF02	     // 后台获取规约配置

#endif