
/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtuarchivestransrcd.cpp
  *Description: 
    暂态文件档案
  *History: 
    1, 创建, wangjs, 2021-8-25
**********************************************************************************/
#include "dtuarchivestransrcd.h"
#include "dtustorage.h"
#include <dtulog.h>
#include <dtucommon.h>
#include <dtucmdcode.h>
#include <dtuprotocol.h>
#include <dtunotifymanager.h>
#include "dtutask_dsp.h"

using namespace DTU;
void DArchiveTransRcd::add_archives_data(const buffer& data)
{
    DTransHeader transHeader;
    transHeader.parse(data.const_data(), data.size());
    uint64_t acttime = ((uint64_t)transHeader._fault_seconds*1000000+(uint64_t)transHeader._fault_mircosec);
    std::lock_guard<std::mutex> lock(_lock);
    auto ita = _trans_rcd.find(acttime);
    if (ita != _trans_rcd.end())
    {
        DTULOG(DTU_INFO,(char*)"[暂态录波]追加暂态录波数据%llu", acttime);
        ita->second.emplace_back(data);
        if (transHeader._frame_no == 0xFFFE)
        {
            // 结束，并生成文件
            form_archives_file(ita->second);
            _trans_rcd.erase(ita);
            //

        }
        // 删除
    }
    else
    {
        // 
        DTULOG(DTU_INFO,(char*)"[暂态录波]新增暂态录波数据%llu", acttime);
        _trans_rcd.clear();
        _trans_rcd[acttime].emplace_back(data);
        if (transHeader._frame_no == 0xFFFE)
        {
            // 结束，并生成文件
            form_archives_file(_trans_rcd[acttime]);
            _trans_rcd.clear();
        }

    }
}
// 形成报告文件
void DArchiveTransRcd::form_archives_file(const std::vector<buffer>& data)
{
    if (data.empty()){
        return;
    }
    //
    uint32_t CurNO = DSTORE::instance().get_cur_report_no(ReportTransRcd);
    DTransHeader transHeader;
    transHeader.parse(data[0].const_data(), data[0].size());

    uint64_t acttime = ((uint64_t)transHeader._fault_seconds*1000000+(uint64_t)transHeader._fault_mircosec);

    std::string fileName = create_time_from_format(acttime, "%04u%02u%02u_%02u%02u%02u_%03u");
    char arcName[128] = {};
    // 此时该条报告还未加入到数据库中,CurNO为上一条报告的序号,所以应该+1
    sprintf(arcName, (char*)"BAY%02u_%04u_%s", DSTORE::instance()._address, CurNO + 1, fileName.c_str());
    char fullName[256] = {};
    sprintf(fullName, (char*)"%s/protect/factory/%s", get_exec_dir().c_str(), arcName);

    DTULOG(DTU_INFO,(char*)"[暂态录波]生成厂家格式录波文件%s", fullName);
    FILE* fp = fopen(fullName, "wb+");
    if (fp == NULL){
        DTULOG(DTU_ERROR,(char*)"[暂态录波]无法创建厂家格式文件[%s]:%s", strerror(errno), fullName);
        return;
    }
    fwrite(data[0].const_data(), 1, data[0].size(), fp);

    // 形成录波文件
    // 保存厂家录波
    for(auto i=1;i<data.size();i++)
    {
        fwrite(data[i].const_data()+DTransHeader::_header_length, 
          data[i].size()-DTransHeader::_header_length, 1, fp);
    }
    fclose(fp);
    DTULOG(DTU_INFO,(char*)"[暂态录波]厂家格式数据生成完成%s...", fullName);

    DTUParamAdjust adjustParam(DTU::DSTORE::instance().read_adj_data());

    // 解析数据
    std::vector<DTransRcd> result;
    for(auto& item : data)
    {
        DTransRcd rcd;
        rcd.parse(item.const_data(), item.size());
        result.emplace_back(std::move(rcd));
    }
    // 生成CFG
    create_cfg(arcName, result, adjustParam);
    // 生成DAT
    create_dat(arcName, result);

    buffer arc(72);
    arc.set(0, (char*)&acttime, sizeof(acttime));
    arc.set(sizeof(acttime), arcName, 64);
    // 存储档案
    std::string filestr(arcName);
    DTU::buffer filebuff(filestr.c_str(),filestr.size());
    DSTORE::instance().add_report_data(ReportTransRcd, transHeader._fault_seconds, (transHeader._fault_mircosec / 1000), filebuff);
    
    dtuprotocol proto;
    proto._header = 0xbb66;
    proto._cmd = TX_PC_LS_LB_DATA;
    proto._curLen = arc.size();
    proto._totleLen = arc.size();
    proto._data = arc;
    proto._blockno = 0xFFFF;
    // 对外通知
    DTU::DTUNotifyMgr::instance().notify_dtutools(proto);
}

//生成cfg文件
void DArchiveTransRcd::create_cfg(std::string fileName, const std::vector<DTransRcd>& data, DTUParamAdjust adjParam)
{
    uint32_t totleSamples = 0;
    for(auto& item : data)
    {
        for(auto& cyc : item._cycles)
        {
            totleSamples += cyc._samples.size();
        }
    }
    std::string fullName = get_exec_dir() + "/protect/comtrade/" + fileName + ".cfg";
    FILE* fp = fopen(fullName.c_str(), "wb+");
    if (!fp){
        DTULOG(DTU_ERROR,(char*)"[暂态录波]无法创建CFG文件[%s]:%s", strerror(errno), fullName.c_str());
        return;
    }
    std::string rev_year = "1999";
    uint16_t analognum = 0, digitalnum = 0; //模拟通道数,数字通道数

    char utfbuff[512] = {};
    char gbkbuff[512] = {};
    // 电站信息
    sprintf(utfbuff,(char*)"SDL9200,SDL9200,%s\n", rev_year.c_str());
    if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
        fwrite(gbkbuff,1,strlen(gbkbuff),fp);
    }
    else{
        fwrite(utfbuff,1,strlen(utfbuff),fp);
    }
   
    // 模拟量开关量信息
    memset(utfbuff,0,sizeof(utfbuff));
    memset(gbkbuff,0,sizeof(gbkbuff));
    uint32_t totleChannel = 21+data[0]._header._digital_num;
    sprintf(utfbuff,(char*)"%u,21A,%uD\n",totleChannel, data[0]._header._digital_num);
    if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
        fwrite(gbkbuff,1,strlen(gbkbuff),fp);
    }
    else{
        fwrite(utfbuff,1,strlen(utfbuff),fp);
    }
    // 模拟量详细信息
    // 取一个采样点
    DTransSamples samples = data[0]._cycles[0]._samples[0];
    for(const auto& item : samples.get_mnl_attr())
    {
        memset(utfbuff,0,sizeof(utfbuff));
        memset(gbkbuff,0,sizeof(gbkbuff));

        if((item.first+1) <= 9)
        {
            sprintf(utfbuff,(char*)"%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", item.first+1, 
                std::get<0>(item.second).c_str(),
                std::get<2>(item.second).c_str(),
                adjParam.GetValue(item.first+1, ADJUST_RATIO),
                adjParam.GetValue(item.first+1, ADJUST_INTERCEPT), 0.0f, 1.0f,1.0f);
        }
        else if((item.first+1) <= 11)
        {
            sprintf(utfbuff,(char*)"%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", item.first+1, 
                std::get<0>(item.second).c_str(),
                std::get<2>(item.second).c_str(), 1.0f,1.0f, 0.0f, 1.0f,1.0f);
        }
        else if((item.first+1) <= 19)
        {
            sprintf(utfbuff,(char*)"%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", item.first+1, 
                std::get<0>(item.second).c_str(),
                std::get<2>(item.second).c_str(),
                adjParam.GetValue(item.first-1, ADJUST_RATIO),
                adjParam.GetValue(item.first-1, ADJUST_INTERCEPT), 0.0f, 1.0f,1.0f);
        }
        else
        {
            sprintf(utfbuff,(char*)"%u,%s,,,%s,%.9f,%.9f,%.9f,-32767,32767,%.9f,%.9f,S\n", item.first+1, 
                std::get<0>(item.second).c_str(),
                std::get<2>(item.second).c_str(), 0.001f,30.0f, 0.0f, 1.0f,1.0f);
        }

        if (UTF8ToGBK(utfbuff, strlen(utfbuff), gbkbuff, sizeof(gbkbuff))){
            fwrite(gbkbuff,1,strlen(gbkbuff),fp);
        }
        else{
            fwrite(utfbuff,1,strlen(utfbuff),fp);
        }
    }
    // 硬遥信
    int i=0;
    for(;i<data[0]._header._digital_num;i++)
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
    fprintf(fp, "%u,%u\n",data[0]._header._samples*50, totleSamples);

    // 写入时间
    uint64_t starttime = (uint64_t)data[0]._cycles[0]._header._seconds*1000000+(uint64_t)data[0]._cycles[0]._header._mircosec;
    fprintf(fp,"%s\n", create_comtradetime_from_mirco(starttime).c_str());
    uint64_t faulttime = (uint64_t)data[0]._header._fault_seconds*1000000+(uint64_t)data[0]._header._fault_mircosec;
    fprintf(fp,"%s\n", create_comtradetime_from_mirco(faulttime).c_str());

    //写数据文件类型
    fprintf(fp,"BINARY\n");
    //写时标倍率因子
    float tfactor = (float)(20000 / data[0]._header._samples) / 1000.0f;
    fprintf(fp, "%.2f\n", tfactor);

    fclose(fp);
 	DTULOG(DTU_INFO,(char*)"生成%s...结束", fullName.c_str());
}


void DArchiveTransRcd::create_dat(std::string fileName, const std::vector<DTransRcd>& data)
{
    if (data.size() == 0){
        return;
    }
    uint32_t timeoffset = 20000 / data[0]._header._samples;

    std::string fullName = get_exec_dir() + "/protect/comtrade/" + fileName + ".dat";
    FILE* fp = fopen(fullName.c_str(), "wb+");
    if (!fp){
        DTULOG(DTU_ERROR,(char*)"[暂态录波]无法创建DAT文件[%s]:%s", strerror(errno), fullName.c_str());
        return;
    }

    uint32_t nSample = 1;
    uint32_t timestamp = 0;
    for(const auto& item : data)
    {
        //
        for(const auto& cyc : item._cycles)
        {
            for(const auto& samples : cyc._samples)
            {
                // 序号
                auto s1 = fwrite(&nSample, 1, sizeof(nSample), fp);
                // 时间
                auto s2 = fwrite(&timestamp, 1, sizeof(timestamp), fp);
                // 
                auto s3 = fwrite(samples._samples_data.const_data(), 1, samples._samples_data.size(), fp);
                // 立即写入
                fflush(fp);
                nSample++;
                timestamp+=timeoffset;
            }
        }
    }

    fclose(fp);
    DTULOG(DTU_INFO,(char*)"[暂态录波]生成%s...结束", fullName.c_str());    
}