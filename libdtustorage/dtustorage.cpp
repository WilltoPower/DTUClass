#include "dtustorage.h"
#include "dtuarchivesdb.h"
#include <pugixml/pugixml.hpp>
#include <dtucommon.h>
#include <dtulog.h>
#include <dtuparamconfig.h>
#include <dtustructs.h>
#include "dtuioamap.h"
#include "dtusystemconfig.h"

using namespace DTU;

// 清理全部报告(声明)
void clearallReport(std::vector<std::string> paths);
// 清理部分报告(声明)
void clearReport(std::vector<std::string> paths, std::vector<std::string> files);

DSTORE::~DSTORE(){
}
// 加载全部的存储设置
void DSTORE::load(std::string fullpath)
{
    // 加载报告配置
    // REPORTDB::instance().load();
    // 加载整定参数
    _adj.load();
    // 加载数据库参数
    DBManager::instance().init(fullpath);
    DBManager::instance().setReportClearAllCallback(clearallReport);
    DBManager::instance().setReportClearOneCallback(clearReport);
}
////////////////////////////////////////
// 保存定值
void DSTORE::write_setting_data(uint16_t id, const DTU::buffer& data, uint32_t group)
{
    try
    {
        dtuParam param(id);
        param.unpack(data);
        if(!DBManager::instance().saveParamValue(param))
            DTULOG(DTU_ERROR,"write_setting_data()保存定值失败");
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::write_setting_data %s", e.what());
    }

}

// 读取定值
void DSTORE::read_setting_data(uint16_t pid,  DTU::buffer& data, uint32_t group)
{
    try
    {
        dtuParam param(pid);
        if(!DBManager::instance().readParamValue(param))
            DTULOG(DTU_ERROR,"read_setting_data()读取定值失败");
        data = param.pack();
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::read_setting_data()发生错误%s", e.what());
    }

}

// 定值区切换
bool DSTORE::change_setting_num(uint16_t group,DBManager::DSPSendFunc func,uint16_t reboot)
{
    try
    {
        DTULOG(DTU_INFO,"设置定值区:%u", group);
        return DBManager::instance().ChangeCurGroup(group,func,reboot);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::change_setting_num %s", e.what());
        return false;
    }
}

// 定值区切换
bool DSTORE::change_default_setting_num(DBManager::DSPSendFunc func,uint16_t reboot)
{
    try
    {
        uint16_t group = get_current_group();
        DTULOG(DTU_INFO,"设置默认定值区:%u", group);
        return DBManager::instance().ChangeCurGroup(group,func,reboot);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::change_default_setting_num %s", e.what());
        return false;
    }
}

uint32_t DSTORE::get_current_group()
{
    return DBManager::instance().GetCurrGroup();
}
uint32_t DSTORE::get_max_group()
{
    return DBManager::instance().GetMaxiGroup();
}
uint32_t DSTORE::get_current_edit_group()
{
    return DBManager::instance().GetEditGroup();
}

// 预设定值
bool DSTORE::preset_setting_data(const DTU::buffer& data)
{

    // 如果预置过一次
    if (PreSettingFirstFlag)
        return true;
    else
        PreSettingFirstFlag = true;

    settingInfo setinfo;
    setinfo.parse(data);
    for(const auto& item : setinfo._settings)
    {
        // // 如果不是本设备则跳过本次循环
        // if (!DBManager::instance().testFixidBelongCurDevice(item._addr))
        //     continue;
        
        // 该点不可以修改
        if (item._addr == 0x5019) {
            PreSettingFlag = false;
            return false;
        }
        else {
            PreSettingFlag = true;
            curTime = get_current_mills();
        }


        HIOA hioa;
        if (!IOAMap::instance()->mapIOAtoHIOA(item._addr, hioa, DTUCFG::DSYSCFG::instance().devno)) {
            DTULOG(DTU_INFO, "遥调 IOA[0x04X] 点表未找到内部对应点表", item._addr);
            PreSettingFlag = false; // 如果发现错误的点表直接返回失败即可,多定值预置只要有一个错误,整个命令就算错误
            return false;
        }

        // 根据点表号预设定值
        item._value.dump(0, item._value.size());
        if(!DBManager::instance().setPreParamValue(DBManager::instance().FixidMapOuttoin(hioa), item._value))
        {
            DTU_USER()
            DTULOG(DTU_ERROR,(char*)"点表值[0x%04X]写定值预置错误", hioa);
            PreSettingFlag = false;
            return false;
        }
    }

    return true;
}

// 写入预设
void DSTORE::save_presetting_data(DBManager::DSPSendFunc func,uint16_t reboot)
{
    if(!DBManager::instance().confirmPreParamValue(func,reboot))
        DTULOG(DTU_ERROR, (char*)"执行确认定值区失败");
}

// 撤销预设
void DSTORE::revert_setting_data()
{
    DBManager::instance().cancelPreParamValue();
}


uint16_t DSTORE::get_paramid_by_cmd(uint16_t cmd)
{
    return DBManager::instance().Get_ParamID_By_CMD(cmd);
}

void DSTORE::seletc_edit_sg(uint32_t group)
{
    try
    {
        if (group == DBManager::instance().GetEditGroup())
            return;
        DBManager::instance().SetEditGroup(group);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::seletc_edit_sg %s", e.what());
    }
}

bool DSTORE::setting_copy(uint32_t src,uint32_t dest)
{
    try
    {
        return DBManager::instance().copyAllParamGroup(src,dest);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"setting_copy发生错误:%s", e.what());
        return false;
    }
    return false;
}

////////////////////////////////////////
// 添加报告数据
void DSTORE::add_report_data(uint16_t reportid, uint32_t s, uint32_t ms, const DTU::buffer& data)
{
    try
    {
        DBManager::instance().addReport(reportid,s,ms,data);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::add_report_data %s", e.what());
    }
}

// 根据序号读取报告
DTU::buffer DSTORE::get_report_by_serial(uint16_t reportid, uint32_t serialno)
{
    try
    {
        DTU::buffer data;
        DBManager::instance().readReportByIndex(reportid,serialno,data);
        return data;
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::get_report_by_serial %s", e.what());
    }
    return DTU::buffer();
}

uint32_t DSTORE::get_cur_report_no(uint16_t reportid)
{
    try
    {
        return DBManager::instance().GetCurReportNoByID(reportid);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"get_report_new_no %s", e.what());
    }
    return 0;
}

bool DSTORE::clear_report(uint16_t reportid)
{
    try
    {
        return DBManager::instance().clearReport(reportid);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::clear_report %s", e.what());
    }
	return false;
}

uint32_t DSTORE::get_report_itemsize(uint16_t reportid)
{
    try
    {
        auto ret = DBManager::instance().GetReportInfoByID(reportid);
        return ret.size;
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::get_report_itemsize %s", e.what());
    }
    return 0;
}

uint16_t DSTORE::get_reportid_by_cmd(uint16_t cmd)
{
    try
    {
        return DBManager::instance().Get_ReportID_By_CMD(cmd);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::get_reportid_by_cmd %s", e.what());
    }
    return 0;
}

void DSTORE::get_report_range(uint16_t reportid, int min, int max,DTU::ReportBufferAttr& data)
{
    try
    {
        DBManager::instance().readReport(reportid,min,max,data);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::get_all_report %s", e.what());
    }    
}

#ifndef _WIN32
//////////////////////////////////////
// 添加档案数据
void DSTORE::add_achives_data(uint16_t id, const DTU::buffer& data)
{
    try{
        ARCHIVEDB::instance().add_archives(id, data);
    }
    catch(std::exception& e)
    {
        DTULOG(DTU_ERROR,(char*)"DSTORE::add_achives_data %s", e.what());
    }    

    
}
#endif

void DSTORE::load_adj_data()
{
    std::lock_guard<std::mutex> LOCK(_adj_lock);
    _adj.load();
}

void DSTORE::write_adj_data(const DTU::buffer& data)
{
    std::lock_guard<std::mutex> LOCK(_adj_lock);
    _adj.parse(data.const_data(), data.size());
}

DTU::buffer DSTORE::read_adj_data()
{
    std::lock_guard<std::mutex> LOCK(_adj_lock);
    return _adj.Adjdata();
}

void DSTORE::save_adj_data()
{
    std::lock_guard<std::mutex> LOCK(_adj_lock);
    _adj.save();
}

#if !_WIN32

void DSTORE::save_selfcheck_data(const DTU::buffer& data)
{
    auto paramLen = DTU::DParamConfig::instance().get_info_length(InfomSelfCheck);
	if (data.size() != paramLen) 
    {
		DTULOG(DTU_ERROR, (char*)"save_selfcheck_data 数据0x%04X长度%u不正确,应该为%u",0x000B, data.size(), paramLen);
        return;
    }

    char buf[128];

	auto a9 = data.get(10, 4).value<uint32_t>();
	auto a10= data.get(14, 4).value<uint32_t>();

	sprintf(buf,"%04uY%02uM%02uD %02u:%02u:%02u %08u", data.get(0, 2).value<uint16_t>(),
                                                     data.get(2, 1).value<uint8_t>(),
                                                     data.get(3, 1).value<uint8_t>(),
                                                     data.get(4, 1).value<uint8_t>(),
                                                     data.get(5, 1).value<uint8_t>(),
                                                     data.get(6, 1).value<uint8_t>(),
                                                     (((data.get(7, 1).value<uint8_t>()) << 8) & 0xFF00) | data.get(8, 1).value<uint8_t>());
    pugi::xml_document doc;
    std::string FullPath = get_exec_dir() + "/protect/selfcheck/selfcheckLog.xml";
    pugi::xml_parse_result ret = doc.load_file(FullPath.c_str());
    if(ret.status != pugi::status_ok)
    {/* 打开失败 */
        init_selfcheck_data(FullPath);
        doc.reset();
        ret = doc.load_file(FullPath.c_str());
        if(ret.status != pugi::status_ok)
        {
            DTULOG(DTU_ERROR,"save_selfcheck_data()无法打开selfcheckLog.xml文件");
        }
    }
    pugi::xml_node xml_node_selfcheckLog = doc.child("selfcheckLog");
    
    //读取总条数
    auto curCount = xml_node_selfcheckLog.attribute("num").as_uint();
    curCount++;

    pugi::xml_node xml_node_log = xml_node_selfcheckLog.append_child("LOG");
    xml_node_log.append_attribute("no") = curCount;//编号
    xml_node_log.append_attribute("tm") = buf;//时间
    std::string front;
    for(int i = 1; i <= (data.size() - 10)/4; i++)
    {
        front = "a" + std::to_string(i);
        char num[12] = {0};
        sprintf(num, "0x%08X", data.get(10 + (i-1)*4, 4).value<uint32_t>());
        xml_node_log.append_attribute(front.c_str()) = num;
    }

    xml_node_selfcheckLog.attribute("num") = curCount;

    // 如果超过所记条数则从头开始删除一条
    // 最多:15万条
    if(curCount > 150000)
    {
        xml_node_selfcheckLog.remove_child(xml_node_selfcheckLog.first_child());
    }
    doc.save_file(FullPath.c_str());
}

void DSTORE::init_selfcheck_data(std::string FullPath)
{
    pugi::xml_document doc;
    doc.reset();
    pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
    pugi::xml_node xml_self_check = doc.append_child("selfcheckLog");
    xml_self_check.append_attribute("num") = "0";
    doc.save_file(FullPath.c_str());
}

#endif

void clearReport(std::vector<std::string> paths, std::vector<std::string> files)
{
	std::string cmdPre = "rm -rf ";
	std::string path = get_exec_dir();

	for (auto& OneFilePath : paths)
	{
		for (auto& OneFile : files)
		{
			std::string FullCmd = cmdPre + path + OneFilePath + "/" + OneFile + "*";
            system(FullCmd.c_str());
		}
	}
}

void clearallReport(std::vector<std::string> paths)
{
	std::string cmdPre = "rm -rf ";
	std::string path = get_exec_dir();
	for (auto& item : paths)
	{
        std::string FullCmd = cmdPre + path + item + "/*";
        system(FullCmd.c_str());
	}
}
