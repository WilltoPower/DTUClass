/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivesworkrcd.cpp
  *Description: 
    业务录波档案
  *History: 
    1, 创建, wangjs, 2021-8-26
**********************************************************************************/
#include "dtuarchivesworkrcd.h"
#include "dtustorage.h"
#include <dtucommon.h>
#include <dtucmdcode.h>
#include <dtulog.h>
#include <dtuprotocol.h>
#include <dtunotifymanager.h>
#include "dtutask_dsp.h"

using namespace DTU;

void DArchiveWorkRcd::add_archives_data(const buffer& data)
{
    static uint32_t cyclelength = 8864;
    if (data.size() < cyclelength){
        return;
    }
    std::lock_guard<std::mutex> lock(_lock);
    uint32_t offset = 0;
    while(offset < data.size())
    {
        _work_rcd.emplace_back(data.get(offset, cyclelength));
        offset+=cyclelength;
    }
    while(_work_rcd.size()>=20)
    {
        form_archives_file(_work_rcd);
        _work_rcd.erase(_work_rcd.begin(), _work_rcd.begin()+20);
    }
    if (!_work_rcd.empty()){
        DTULOG(DTU_WARN, "DArchiveWorkRcd 有残留周波数据%u, 删除", _work_rcd.size());
    }
}

// 形成报告文件
void DArchiveWorkRcd::form_archives_file(const std::vector<buffer>& data)
{
    if (data.empty()){
        return;
    }
    // 
    uint32_t CurNo = DSTORE::instance().get_cur_report_no(ReportWorkRcd);
    DWorkCycleHeader header;

    if(!(header.parse(data[0].const_data(), header._header_length)))
    {
        std::string fullName = get_exec_dir() + "/protect/selfcheck/LastErrorDataWorkRcd.txt";
        DTULOG(DTU_WARN, "[业务录波]发生解析错误,写文件:%s", fullName.c_str());
        FILE* fp = fopen(fullName.c_str(), "wb+");
        if (!fp){
            DTULOG(DTU_ERROR, "[业务录波]无法打开错误记录文件%s", fullName.c_str());
            return;
        }
        for(auto &item:data)
        {
            fwrite(item.const_data(),1,item.size(), fp);
        }
        fclose(fp);
        DTULOG(DTU_WARN, "[业务录波]写文件完成:%s", fullName.c_str());
        return;
    }

    uint64_t acttime = ((uint64_t)header._seconds*1000000+(uint64_t)header._mircosec);

    std::string fileName = create_time_from_format(acttime, "%04u%02u%02u_%02u%02u%02u_%03u");

    char arcName[128] = {};
    // 此时该条报告还未加入到数据库中,CurNO为上一条报告的序号,所以应该+1
    sprintf(arcName,  "BAY%02u_%04u_%s", DSTORE::instance()._address, CurNo + 1, fileName.c_str());

    char fullName[256] = {};
    sprintf(fullName,  "%s/FACTORY/%s", get_exec_dir().c_str(), arcName);
    DTULOG(DTU_INFO, "[业务录波]生成厂家格式文件:%s", fullName);
    FILE* fp = fopen(fullName, "ab+");
    if (fp == NULL){
        DTULOG(DTU_ERROR, "[业务录波]无法创建厂家格式文件文件[%s]:%s", strerror(errno), fullName);
        return;
    }
    // 保存厂家录波
    for(auto i=0;i<data.size();i++)
    {
        fwrite(data[i].const_data(), 1, data[i].size(), fp);
    }
    fclose(fp);
    DTULOG(DTU_INFO,(char*)"[业务录波]厂家格式数据生成完成%s...", fullName);

    DTUParamAdjust adjustParam(DTU::DSTORE::instance().read_adj_data());

    // 形成COMTRADE
    create_cfg(arcName,adjustParam);
    create_dat(arcName);

    // 形成档案
    buffer arc(72);
    arc.set(0, (char*)&acttime, sizeof(acttime));
    arc.set(sizeof(acttime), arcName, 64);

    // 存储档案
    std::string filestr(arcName);
    DTU::buffer filebuff(filestr.c_str(),filestr.size());
    DSTORE::instance().add_report_data(ReportWorkRcd,header._seconds,(header._mircosec / 1000),filebuff);

    dtuprotocol proto;
    proto._header = 0xbb66;
    proto._cmd = TX_PC_ZTLB_DATA;
    proto._curLen = arc.size();
    proto._totleLen = arc.size();
    proto._data = arc;
    proto._blockno = 0xFFFF;
    // 对外通知
    DTU::DTUNotifyMgr::instance().notify_dtutools(proto);
}

//生成cfg文件
void DArchiveWorkRcd::create_cfg(std::string fileName, DTUParamAdjust adjParam)
{
    if (_work_rcd.empty())
    {
        return;
    }
   
    // 统计点数
    uint32_t totleSamples = 0;
    for(auto i=0;i<20;i++)
    {
        DWorkCycle cyc;
        cyc.parse(_work_rcd[i].const_data(), _work_rcd[i].size());
        totleSamples+= cyc._samples.size();
    }

    std::string fullName = get_exec_dir() + "/COMTRADE/" + fileName + ".cfg";
    DTULOG(DTU_INFO, "[业务录波]创建文件:%s", fullName.c_str());
    FILE* fp = fopen(fullName.c_str(), "wb+");
    if (!fp){
        DTULOG(DTU_ERROR, "[业务录波]无法创建CFG文件[%s]:%s", strerror(errno), fullName.c_str());
        return;
    }
    std::string rev_year = "1999";
    uint16_t analognum = 0, digitalnum = 0; //模拟通道数,数字通道数

    char utfbuff[512] = {};
    char gbkbuff[512] = {};
    // 电站信息
    sprintf(utfbuff, "SDL9200,SDL9200,%s\n", rev_year.c_str());
    if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
        fwrite(gbkbuff,1,strlen(gbkbuff),fp);
    }
    else{
        fwrite(utfbuff,1,strlen(utfbuff),fp);
    }
   
    DWorkCycle workCyc;
    workCyc.parse(_work_rcd[0].const_data(), _work_rcd[0].size());

    DWorkCycle workFaultCyc;
    workFaultCyc.parse(_work_rcd[10].const_data(), _work_rcd[10].size());

    // 模拟量开关量信息
    memset(utfbuff,0,sizeof(utfbuff));
    memset(gbkbuff,0,sizeof(gbkbuff));
    uint32_t totleChannel = workCyc._header._cur_num+
        workCyc._header._vol_num+workCyc._header._ki_num + workCyc._header._zt_ground;

    sprintf(utfbuff, "%u,%uA,%uD\n",totleChannel, 
        workCyc._header._cur_num+workCyc._header._vol_num+workCyc._header._zt_ground, 
        workCyc._header._ki_num);

    if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
        fwrite(gbkbuff,1,strlen(gbkbuff),fp);
    }
    else{
        fwrite(utfbuff,1,strlen(utfbuff),fp);
    }
   
    // 模拟量详细信息
    // 取一个采样点
    DWorkSamples samples = workCyc._samples[0];
    for(auto i=0;i<workCyc._header._cur_num+workCyc._header._vol_num+workCyc._header._zt_ground;i++)
    {
        memset(utfbuff,0,sizeof(utfbuff));
        memset(gbkbuff,0,sizeof(gbkbuff));
        auto ita =samples.get_mnl_attr().find(i);
        if (ita != samples.get_mnl_attr().end())
        {
            //GetValue是按照17路存取的要进行转换
            if((i+1) <= 9)
            {
                sprintf(utfbuff, "%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", i+1, 
                    std::get<0>(ita->second).c_str(),
                    std::get<2>(ita->second).c_str(),
                    adjParam.GetValue(i+1, ADJUST_RATIO),
                    adjParam.GetValue(i+1, ADJUST_INTERCEPT), 0.0f, 1.0f,1.0f);
            }
            else if((i+1) <= 11)
            {
                sprintf(utfbuff, "%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", i+1, 
                    std::get<0>(ita->second).c_str(),
                    std::get<2>(ita->second).c_str(), 1.0f, 0.0f, 0.0f, 1.0f,1.0f);
            }
            else if((i+1) <= 19)
            {
                sprintf(utfbuff, "%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", i+1, 
                    std::get<0>(ita->second).c_str(),
                    std::get<2>(ita->second).c_str(),
                    adjParam.GetValue(i-1, ADJUST_RATIO),
                    adjParam.GetValue(i-1, ADJUST_INTERCEPT), 0.0f, 1.0f,1.0f);
            }
            else
            {
                sprintf(utfbuff, "%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", i+1, 
                    std::get<0>(ita->second).c_str(),
                    std::get<2>(ita->second).c_str(), 1.0f, 0.0f, 0.0f, 1.0f,1.0f);
            }
            
            if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
                fwrite(gbkbuff,1,strlen(gbkbuff),fp);
            }
            else{
                fwrite(utfbuff,1,strlen(utfbuff),fp);
            }
   
        }
    }
    // 硬遥信
    int i=0;
    for(;i<workCyc._header._ki_num;i++)
    {
        std::string desc = "备用";
        auto ita = samples.get_dig_attr().find(i+1);
        if (ita != samples.get_dig_attr().end())
        {
            desc = ita->second;
        }
        memset(utfbuff,0,sizeof(utfbuff));
        memset(gbkbuff,0,sizeof(gbkbuff));
        sprintf(utfbuff,"%u,%s,,,%u\n",i+1,desc.c_str(),0);
        if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
            fwrite(gbkbuff,1,strlen(gbkbuff),fp);
        }
        else{
            fwrite(utfbuff,1,strlen(utfbuff),fp);
        }
   
    }
    //
    // 标称通道频率
    int lf = 50;
    fprintf(fp,"%u\n",lf);
    // 频率信息
    fprintf(fp,"%u\n",1);
    fprintf(fp, "%u,%u\n",workCyc._header._freq, totleSamples);
    // 写入时间
    uint64_t starttime = (uint64_t)workCyc._header._seconds*1000000+(uint64_t)workCyc._header._mircosec;
    fprintf(fp,"%s\n", create_comtradetime_from_mirco(starttime).c_str());
    uint64_t faulttime = (uint64_t)workFaultCyc._header._seconds*1000000+(uint64_t)workFaultCyc._header._mircosec;
    fprintf(fp,"%s\n", create_comtradetime_from_mirco(faulttime).c_str());
    
    //写数据文件类型
    fprintf(fp,"BINARY\n");
    //写时标倍率因子
    float tfactor = 1;
    fprintf(fp, "%.2f\n", tfactor);

    fclose(fp);
 	DTULOG(DTU_INFO, "[业务录波]生成%s.cfg...结束", fileName.c_str());    
}

void DArchiveWorkRcd::create_dat(std::string fileName)
{
    if (_work_rcd.empty())
    {
        return;
    }
    const uint32_t timeoffset = 100;
    
    std::string fullName = get_exec_dir() + "/COMTRADE/" + fileName + ".dat";
    DTULOG(DTU_INFO, "[业务录波]创建文件:%s", fullName.c_str());
    FILE* fp = fopen(fullName.c_str(), "ab+");
    if (!fp){
        DTULOG(DTU_ERROR, "[业务录波]无法创建DAT文件[%s]:%s", strerror(errno), fullName.c_str());
        return;
    }

    uint32_t nSample = 1;
    uint32_t timestamp = 0;
    int count = 1;
    for(const auto& item : _work_rcd)
    {
        DWorkCycle cyc;
        if(!cyc.parse(item.const_data(), item.size()))
        {
            DTULOG(DTU_ERROR, "[业务录波]DArchiveWorkRcd::create_dat 发生解析错误");
        }
        //
        for(const auto& samples : cyc._samples)
        {
             // 序号
            fwrite(&nSample, 1, sizeof(nSample), fp);
            // 时间
            fwrite(&timestamp, 1, sizeof(timestamp), fp);
            // 
            fwrite(samples._samples_data.const_data(), DWorkSamples::_samples_length, 1, fp);
            // 立即写入
            fflush(fp);
            nSample++;
            timestamp+=timeoffset;
        }
    }
    fclose(fp);
    DTULOG(DTU_INFO, "[业务录波]生成%s...结束", fileName.c_str());
}