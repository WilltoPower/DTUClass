/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  DTUProtectControl.h
  *Description: 
    保护控制字
  *History: 
    1, 创建, wangjs, 2021-8-10
**********************************************************************************/
#ifndef _DTU_PROTECT_ACT_REPORT_H
#define _DTU_PROTECT_ACT_REPORT_H

// 版本
#define V_PROTACT_VERSION 0x0000
#define O_PROTACT_VERSION 0
#define S_PROTACT_VERSION sizeof(uint16_t)
// 类型
#define V_RPOTACT_TYPE 0x0001
#define O_PROTACT_TYPE (O_PROTACT_VERSION+S_PROTACT_VERSION)
#define S_PROTACT_TYPE sizeof(uint16_t)
// 属性
#define V_PROTACT_ATTR 0x0002
#define O_PROTACT_ATTR (O_PROTACT_TYPE+S_PROTACT_TYPE)
#define S_PROTACT_ATTR sizeof(uint16_t)
// 开入量1
#define V_PROTACT_KI_I 0x0003
#define O_PROTACT_KI_I (O_PROTACT_ATTR+S_PROTACT_ATTR)
#define S_PROTACT_KI_I sizeof(uint16_t)
// 开入量2
#define V_PROTACT_KI_II 0x0004
#define O_PROTACT_KI_II (O_PROTACT_KI_I+S_PROTACT_KI_I)
#define S_PROTACT_KI_II sizeof(uint16_t)
// 保留1
#define V_PROTACT_SAVE_I 0x0005
#define O_PROTACT_SAVE_I (O_PROTACT_KI_II+S_PROTACT_KI_II)
#define S_PROTACT_SAVE_I sizeof(uint16_t)
// 总秒数
#define V_PROTACT_SECONDS 0x0006
#define O_PROTACT_SECONDS (O_PROTACT_SAVE_I+S_PROTACT_SAVE_I)
#define S_PROTACT_SECONDS sizeof(uint32_t)
// 微妙
#define V_PROTACT_MIRCOSEC 0x0007
#define O_PROTACT_MIRCOSEC (O_PROTACT_SECONDS+S_PROTACT_SECONDS)
#define S_PROTACT_MIRCOSEC sizeof(uint32_t)
// 偏移
#define V_PROTACT_TOFFSET 0x0008
#define O_PROTACT_TOFFSET (O_PROTACT_MIRCOSEC+S_PROTACT_MIRCOSEC)
#define S_PROTACT_TOFFSET sizeof(uint32_t)
// 闰秒
#define V_PROTACT_LEAP 0x0009
#define O_PROTACT_LEAP (O_PROTACT_TOFFSET+S_PROTACT_TOFFSET)
#define S_PROTACT_LEAP  sizeof(uint16_t)
// 模拟量IA
#define V_PROTACT_IA 0x000A
#define O_PROTACT_IA (O_PROTACT_LEAP+S_PROTACT_LEAP)
#define S_PROTACT_IA sizeof(uint16_t)
// 模拟量IB
#define V_PROTACT_IB 0x000B
#define O_PROTACT_IB (O_PROTACT_IA+S_PROTACT_IA)
#define S_PROTACT_IB sizeof(uint16_t)
// 模拟量IC
#define V_PROTACT_IC 0x000C
#define O_PROTACT_IC (O_PROTACT_IB+S_PROTACT_IB)
#define S_PROTACT_IC sizeof(uint16_t)
// 模拟量I0
#define V_PROTACT_I0 0x000D
#define O_PROTACT_I0 (O_PROTACT_IC+S_PROTACT_IC)
#define S_PROTACT_I0 sizeof(uint16_t)
// 模拟量I1
#define V_PROTACT_I1 0x000E
#define O_PROTACT_I1 (O_PROTACT_I0+S_PROTACT_I0)
#define S_PROTACT_I1 sizeof(uint16_t)
// 模拟量I2
#define V_PROTACT_I2 0x000F
#define O_PROTACT_I2 (O_PROTACT_I1+S_PROTACT_I1)
#define S_PROTACT_I2 sizeof(uint16_t)
// 保留2
#define V_PROTACT_SAVE_II 0x0010
#define O_PROTACT_SAVE_II (O_PROTACT_I2+S_PROTACT_I2)
#define S_PROTACT_SAVE_II (sizeof(uint16_t)*4)
// 模拟量UA
#define V_PROTACT_UA 0x0011
#define O_PROTACT_UA (O_PROTACT_SAVE_II+S_PROTACT_SAVE_II)
#define S_PROTACT_UA sizeof(uint16_t)
// 模拟量UB
#define V_PROTACT_UB 0x0012
#define O_PROTACT_UB (O_PROTACT_UA+S_PROTACT_UA)
#define S_PROTACT_UB sizeof(uint16_t)
// 模拟量UC
#define V_PROTACT_UC 0x0013
#define O_PROTACT_UC (O_PROTACT_UB+S_PROTACT_UB)
#define S_PROTACT_UC sizeof(uint16_t)
// 模拟量UX
#define V_PROTACT_UX 0x0014
#define O_PROTACT_UX (O_PROTACT_UC+S_PROTACT_UC)
#define S_PROTACT_UX sizeof(uint16_t)
// 模拟量U0
#define V_PROTACT_U0 0x0015
#define O_PROTACT_U0 (O_PROTACT_UX+S_PROTACT_UX)
#define S_PROTACT_U0 sizeof(uint16_t)
// 模拟量U1
#define V_PROTACT_U1 0x0016
#define O_PROTACT_U1 (O_PROTACT_U0+S_PROTACT_U0)
#define S_PROTACT_U1 sizeof(uint16_t)
// 模拟量U2
#define V_PROTACT_U2 0x0017
#define O_PROTACT_U2 (O_PROTACT_U1+S_PROTACT_U1)
#define S_PROTACT_U2 sizeof(uint16_t)
// 模拟量UAB
#define V_PROTACT_UAB 0x0018
#define O_PROTACT_UAB (O_PROTACT_U2+S_PROTACT_U2)
#define S_PROTACT_UAB sizeof(uint16_t)
// 模拟量UBC
#define V_PROTACT_UBC 0x0019
#define O_PROTACT_UBC (O_PROTACT_UAB+S_PROTACT_UAB)
#define S_PROTACT_UBC sizeof(uint16_t)
// 模拟量UCA
#define V_PROTACT_UCA 0x001A
#define O_PROTACT_UCA (O_PROTACT_UBC+S_PROTACT_UBC)
#define S_PROTACT_UCA sizeof(uint16_t)
// 保留3
#define V_PROTACT_SAVE_III 0x001B
#define O_PROTACT_SAVE_III (O_PROTACT_UCA+S_PROTACT_UCA)
#define S_PROTACT_SAVE_III (sizeof(uint16_t)*4)
// 模拟量Angle
#define V_PROTACT_ANGLE 0x001C
#define O_PROTACT_ANGLE (O_PROTACT_SAVE_III+S_PROTACT_SAVE_III)
#define S_PROTACT_ANGLE sizeof(uint16_t)
// 相别
#define V_PROTACT_PHASE 0x001D
#define O_PROTACT_PHASE (O_PROTACT_ANGLE+S_PROTACT_ANGLE)
#define S_PROTACT_PHASE sizeof(uint16_t)
// 保留4
#define V_PROTACT_SAVE_IV 0x001E
#define O_PROTACT_SAVE_IV (O_PROTACT_PHASE+S_PROTACT_PHASE)
#define S_PROTACT_SAVE_IV sizeof(uint16_t)
// 母线Fm
#define V_PROTACT_LINE_F 0x001F
#define O_PROTACT_LINE_F (O_PROTACT_SAVE_IV+S_PROTACT_SAVE_IV)
#define S_PROTACT_LINE_F sizeof(uint16_t)
// 线路Fx
#define V_PROTACT_LINE_FX 0x0020
#define O_PROTACT_LINE_FX (O_PROTACT_LINE_F+S_PROTACT_LINE_F)
#define S_PROTACT_LINE_FX sizeof(uint16_t)
// 保留5
#define V_PROTACT_SAVE_V 0x0021
#define O_PROTACT_SAVE_V (O_PROTACT_LINE_FX+S_PROTACT_LINE_FX)
#define S_PROTACT_SAVE_V (sizeof(uint16_t)*2)

#endif