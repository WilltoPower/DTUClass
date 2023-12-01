/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivesproact.cpp
  *Description: 
    存储保护动作报告
  *History: 
    1, 创建, wangjs, 2021-8-10
**********************************************************************************/
#include "dtuarchivesproact.h"
#include "dtustorage.h"
#include <dtureporthelper.h>
#include <dtunotifymanager.h>
#include <dtucmdcode.h>
#include <dtucommon.h>
#include <dtuerror.h>
#include <dtulog.h>
#include <future>

#define DTU_RPT_PROTITEM_LENGTH 118
// 保护启动(与保护动作类型对应)
#define PROTACT_BEGIN           1
// 整组复归
#define PROTACT_RESETWHOLE      3

using namespace DTU;
using namespace std;

    static vector<tuple<uint32_t, uint32_t>> SimpleReport = {/* 首个动作简报取出内容, 地址+大小 */
        {O_PROTACT_IA,S_PROTACT_IA},         /* 模拟量Ia */
        {O_PROTACT_IB,S_PROTACT_IB},         /* 模拟量Ib */
        {O_PROTACT_IC,S_PROTACT_IC},         /* 模拟量Ic */
        {O_PROTACT_I0,S_PROTACT_I0},         /* 模拟量I0 */
        {O_PROTACT_UA,S_PROTACT_UA},         /* 模拟量Ua */
        {O_PROTACT_UB,S_PROTACT_UB},         /* 模拟量Ub */
        {O_PROTACT_UC,S_PROTACT_UC},         /* 模拟量Uc */
        {O_PROTACT_U0,S_PROTACT_U0},         /* 模拟量U0 */
        {O_PROTACT_UX,S_PROTACT_UX},         /* 模拟量Ux */
        {O_PROTACT_LINE_F,S_PROTACT_LINE_F}, /* 母线Fm */
        {O_PROTACT_LINE_FX,S_PROTACT_LINE_FX}/* 线路Fx */
    };
    static vector<tuple<uint32_t, uint32_t>> AbsolutTime = {
        {O_PROTACT_SECONDS,S_PROTACT_SECONDS},  /* 总秒数 */
        {O_PROTACT_MIRCOSEC,S_PROTACT_MIRCOSEC},/* 总微妙数 */
        {O_PROTACT_LEAP,S_PROTACT_LEAP}         /* 闰秒信息 */
    };
    static vector<tuple<uint32_t, uint32_t>> Slitting = {
        {O_PROTACT_TOFFSET,S_PROTACT_TOFFSET},  /* 偏移时间 */
        {O_PROTACT_TYPE,sizeof(uint32_t)}       /* 保护类型+保护属性 */
    };

void DArchiveProtact::add_archives_data(const buffer& data)
{
    DTU_USER()
    auto ret = DBManager::instance().GetReportInfoByID(ReportPro);
    if (data.size() != ret.size) {
        DTU_THROW((char*)"保护动作报告长度%u不正确,应该为:%u", data.size(), ret.size);
    }
    // 获取时间
    uint32_t seconds = data.get(O_PROTACT_SECONDS, S_PROTACT_SECONDS).value<uint32_t>();
    uint32_t mircosec = data.get(O_PROTACT_MIRCOSEC, S_PROTACT_MIRCOSEC).value<uint32_t>();
    uint64_t acttime = ((uint64_t)seconds*1000000+(uint64_t)mircosec);
    //弹窗
    dtuprotocol ACK_mcu(0xbb66, TX_PC_POP_INFO, 0, 0, 0xFFFF);

    // 是否有前置保护动作
    std::lock_guard<std::mutex> lock(_lock);
    auto ita = _prot_act.find(acttime);
    if (ita != _prot_act.end())
    {
        uint16_t acttype = data.get(O_PROTACT_TYPE, S_PROTACT_TYPE).value<uint16_t>();
        DTULOG(DTU_INFO,(char*)"追加保护动作报告,类型:%u, 时间:%u,%u", acttype, seconds, mircosec);
        _prot_act[acttime].emplace_back(data);
        //准备通知MCU 单次弹窗
        ACK_mcu._data = data;
        ACK_mcu._curLen = data.size();
        ACK_mcu._totleLen = data.size();
        //要加延时，不然LCD收不到应答信号
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        DTU::DTUNotifyMgr::instance().notify_mcu(ACK_mcu);

        if (acttype == PROTACT_RESETWHOLE)
        {
            DTULOG(DTU_INFO,(char*)"保护动作整组复归");
            // 整组复归，标志保护动作结束，生成文件
            form_archives_file(ita->second);
            // 删除
            _prot_act.erase(ita);
        }
    }
    else
    {

        // 判定是否是保护动作开始
        uint16_t acttype = data.get(O_PROTACT_TYPE, S_PROTACT_TYPE).value<uint16_t>();
        // 保护动作启动，开始记录动作报告
        if (acttype == PROTACT_BEGIN)
        {
            // 新生成保护动作报告之前，结束剩余的保护动作数据
            // for(auto& item : _prot_act)
            // {
            //     form_archives_file(item.second);
            // }
            _prot_act.clear();

            DTULOG(DTU_INFO,(char*)"新增保护动作报告, 时间:%u,%u", seconds, mircosec);
            _prot_act[acttime].emplace_back(data);

            //准备通知MCU 单次弹窗
            ACK_mcu._data = data;
            ACK_mcu._curLen = data.size();
            ACK_mcu._totleLen = data.size();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            DTU::DTUNotifyMgr::instance().notify_mcu(ACK_mcu);
        }
        else
        {
            DTULOG(DTU_WARN,(char*)"保护动作的起始动作有误:%u, 时间:%u,%u", acttype, seconds, mircosec);
        }
    }
}

void DArchiveProtact::form_archives_file(const std::vector<buffer>& data)
{
    if (data.empty()){
        return;
    }
    uint32_t seconds = data[0].get(6 * sizeof(uint16_t), sizeof(uint32_t)).value<uint32_t>();
    uint32_t mircosec = data[0].get(6 * sizeof(uint16_t) + sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
    uint64_t acttime = ((uint64_t)seconds*1000000+(uint64_t)mircosec);
    // filepath
    std::string filename = create_filename_from_mirco(acttime);
    std::string execdir = get_exec_dir();
    std::string fullname = execdir+"/protect/protect/"+filename;

    uint32_t totleNum = data.size();

    FILE* fp = fopen(fullname.c_str(),"wb+");
    if (fp)
    {
        // 写入总条数
        fwrite(&totleNum, 1, sizeof(totleNum),fp);
        for(const auto& item : data)
        {
            fwrite(item.const_data(), 1, item.size(), fp);
        }
        fclose(fp);
        DTULOG(DTU_INFO,(char*)"生成保护动作报告文件:%s,报告内容条数%u", fullname.c_str(),totleNum);
    }

    // 写入动作报告档案
    dtuprotocol proto;
    proto._header = 0xbb66;
    proto._cmd = TX_PC_PRO_ACT_INFO_DATA;
    proto._curLen = DTU_RPT_PROTITEM_LENGTH;
    proto._totleLen = DTU_RPT_PROTITEM_LENGTH;
    proto._data.resize(DTU_RPT_PROTITEM_LENGTH);
    proto._blockno = 0xFFFF;

    //在写入buffer的数据偏移量,每一步都要累计
    uint32_t offset = 0;
    uint16_t Recv_u16 = 0;
    uint32_t Recv_u32 = 0;
    // //原数据:动作时间+文件名 48字节
    // proto._data.set(offset, (char*)&acttime, sizeof(acttime));
    // offset += sizeof(acttime);

    // proto._data.set(offset, filename.c_str(), filename.size());
    // offset += 40;

    // ARM数据简报
    // proto._data.set(offset, (char*)&ReportNo, sizeof(uint64_t));
    // offset += sizeof(uint64_t);

    DTU::buffer FirstActData;
    for(auto findAct : data)
    {
        // 查找第一个保护动作 (1为保护启动 3为整组复归)
        if(findAct.get(sizeof(uint16_t),sizeof(uint16_t)).value<uint16_t>() == 1)
        {
            FirstActData = findAct;
            break;
        }
    }

    if(FirstActData.size()==0)
    {
        DTULOG(DTU_WARN,"未发现保护起始动作");
        // 未发现保护起始动作,用保护启动的数据放入
        FirstActData = data[0];
    }

    for(const auto& item : SimpleReport)
    {
        auto DataRet = FirstActData.get(get<0>(item),get<1>(item)).value<uint16_t>();
        proto._data.set(offset, (char*)&DataRet, sizeof(DataRet));
        offset += get<1>(item);
    }

    // 保留   值为零
    proto._data.set(offset, (char*)&Recv_u16, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // 动作信息条数 本次报告条数
    Recv_u32 = data.size();
    proto._data.set(offset, (char*)&Recv_u32, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //绝对时间   4字节总秒数 + 4字节总微秒数 + 2字节闰秒信息
    for(const auto& item : AbsolutTime)
    {
        if( get<1>(item) == sizeof(uint16_t) )
        {
            auto DataRet = data[0].get(get<0>(item),get<1>(item)).value<uint16_t>();
            proto._data.set(offset, (char*)&DataRet, sizeof(DataRet));
            offset += get<1>(item);
        }
        else if( get<1>(item) == sizeof(uint32_t) )
        {
            auto DataRet = data[0].get(get<0>(item),get<1>(item)).value<uint32_t>();
            proto._data.set(offset, (char*)&DataRet, sizeof(DataRet));
            offset += get<1>(item);
        }
    }

    //时间偏移量   保护类型+保护属性
    int i = 0;
    for(const auto& item : data)
    {
        for(const auto& item1 : Slitting)
        {
            auto DataRet = data[i].get(get<0>(item1),get<1>(item1)).value<uint32_t>();
            proto._data.set(offset, (char*)&DataRet, sizeof(DataRet));
            offset += get<1>(item1);
        }
        i++;
    }
    // 添加简报报告
    DSTORE::instance().add_report_data(ReportProSimple,seconds,(mircosec/1000),proto._data);
    // 添加动作报告(发送文件名称)
    DTU::buffer proall(filename.c_str(), filename.size());
    DSTORE::instance().add_report_data(ReportPro,seconds,(mircosec/1000),proall);

    // 对外通知

    // 通知工具
    dtuprotocol prototool;
    prototool = proto;
    prototool._curLen = proall.size();
    prototool._totleLen = proall.size();
    prototool._data = proall;
    DTU::DTUNotifyMgr::instance().notify_dtutools(prototool);

    // 通知MCU
    // 需要延时不然LCD接收不到
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DTULOG(DTU_INFO,(char*)"保护动作报告完成,正在通知MCU...");
    // 添加当前报告号 
    // 此时报告已经添加 获取的已经是最新一条的报告
    uint64_t ReportNo = DSTORE::instance().get_cur_report_no(ReportProSimple);
    proto._data.insert(0,(char*)(&ReportNo),sizeof(ReportNo));
    DTU::DTUNotifyMgr::instance().notify_mcu(proto);
}