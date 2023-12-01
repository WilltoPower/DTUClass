#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "dtudbmanager.h"

#include "sqlite3/sqlite3pp.h"
#include "dtucmdcode.h"
#include "dtulog.h"
#include "dtucommon.h"
#include <thread>
#include <mutex>

using namespace DTU;

#define GroupNum 5

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

// 控件类型
#define SWITCH		0	// check
#define COMBO		1	// combobox
#define SPIN		2	// spinbox
#define DSPIN		3	// doublespinbox
#define EDIT		4	// editline
#define LABLE		5	// label
#define IPWIDGET	6	// IPwidget(自定义控件类型)

using AllParamValue = std::map<uint16_t, std::vector<std::string>>;

// 四遥上送信息计数(计算每组偏移的个数)
static std::map<MapFixno, int> mapRMSystem = {
	{MAP_YX, 0},
	{MAP_YC, 0},
	{MAP_YK, 0},
	{MAP_YT, 0},
	{MAP_AU, 0},
};

/*--------------------------------------- 功能函数 --------------------------------------------------*/
#ifdef _WIN32
static std::string UTF8ToGBK(const std::string& strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	wmemset(wszGBK, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, (LPCCH)strUTF8.c_str(), -1, wszGBK, len);

	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	delete[]szGBK;
	delete[]wszGBK;
	return strTemp;
}

std::string GBKToUTF8(std::string chGBK)
{
	DWORD dWideBufSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(chGBK.c_str()), -1, NULL, 0);
	wchar_t* pWideBuf = new wchar_t[dWideBufSize];
	wmemset(pWideBuf, 0, dWideBufSize);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(chGBK.c_str()), -1, pWideBuf, dWideBufSize);

	DWORD dUTF8BufSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pWideBuf, -1, NULL, 0, NULL, NULL);

	char* pUTF8Buf = new char[dUTF8BufSize];
	memset(pUTF8Buf, 0, dUTF8BufSize);
	WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pWideBuf, -1, pUTF8Buf, dUTF8BufSize, NULL, NULL);

	std::string ret(pUTF8Buf);
	delete[]pWideBuf;
	delete[]pUTF8Buf;

	return ret;
}
#endif

/*--------------------------------------- 数据库操作 --------------------------------------------------*/
#define checkDB(str,ret) if(str.empty())return ret;
class DBOperate {
private:
	DBOperate() {}
public:
	static DBOperate& instance() {
		static DBOperate mgr;
		return mgr;
	}
public:
	void SetDBPath(std::string path)
	{
		DBFilePath = path;
	}
	// 读取定值区相关信息
	bool read_group(uint16_t& curgroup, uint16_t& editgroup, uint16_t& maxgroup)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableGroup";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				std::tie(curgroup, editgroup, maxgroup) = (*i).get_columns<int, int, int>(1, 2, 3);
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_group()读取发生错误:%s",e.what());
			return false;
		}
		return true;
	}
	// 保存定值区相关信息
	bool save_group(uint16_t curgroup, uint16_t editgroup)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				char sql[128] = {};
				sprintf(sql, "UPDATE TableGroup SET CURGROUP=%u, EDITGROUP=%u WHERE ID=0",
					curgroup, editgroup);
				sqlite3pp::command cmd(db, sql);
				if (SQLITE_OK != cmd.execute())
					xct.rollback();
				else
					xct.commit();
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "save_group()保存发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 读所有定值索引
	bool read_param_index(dtuParamIndexTable& attrs)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableParamIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				attrs[(*i).get<int>(0)] = dtuOneParamIndex (
					(*i).get<uint16_t>(0), (*i).get<int>(1), (*i).get<const char*>(2) );
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_param_table_index()读取配置表发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 读取所有定值信息
	bool read_param_table(dtuOneParamIndex& attrs)
	{
		try
		{
			checkDB(DBFilePath,false);
			int nCount = 0;
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableParamInfomation WHERE PARAMID=" + std::to_string(attrs.pid);
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				std::string liststr;
				const char* list = (*i).get<const char*>(8);
				if (list != nullptr)
				{
					liststr = list;
				}

				bool use = ((*i).get<int>(14) == 1) ? true : false;

				attrs.info[(*i).get<int>(0)] = dtuOneParamInfo (
					(*i).get<uint16_t>(0), (*i).get<const char*>(1), (*i).get<uint16_t>(2),
					(*i).get<int>(3), (*i).get<double>(4), (*i).get<double>(5),
					(*i).get<double>(6), (*i).get<const char*>(7), liststr,
					(*i).get<uint8_t>(9), (*i).get<const char*>(10),(*i).get<uint8_t>(11), 
					(*i).get<const char*>(12), (*i).get<int>(13) , use);
				
				if (use)
					nCount++;
			}

			if (attrs.pid == ParamAutomation)
				mapRMSystem[MAP_AU] = mapRMSystem[MAP_AU] + nCount;
			else 
				mapRMSystem[MAP_YT] = mapRMSystem[MAP_YT] + nCount;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_param_table()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 读报告索引
	bool read_report_index(dtuReportIndexTable& attrs)
	{
		try 
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());

			for (auto& item : attrs)
			{
				int id = 0;
				std::string sql = "SELECT * FROM TableReportInfomation WHERE REPORTID=" + std::to_string(item.second.reportid);
				sqlite3pp::query qry(db, sql.c_str());
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					dtuOneReportInfo attr;
					attr.id = id;
					attr.desc = (*i).get<const char*>(0);
					attr.size = (*i).get<int>(1);
					attr.offset = (*i).get<int>(3);
					item.second.rinfo[id] = attr;
					id++;
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "读取报告索引发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 读报告信息
	bool read_report_table(dtuReportIndexTable& attrs)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableReportIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				dtuOneReportIndex attr;
				attr.reportid = (*i).get<int>(0);
				attr.desc = (*i).get<const char*>(1);
				attr.size = (*i).get<int>(2);
				attr.type = static_cast<ReportType>((*i).get<int>(3));
				attr.paths = split_str((*i).get<const char*>(4),";");
				attr.table = (*i).get<const char*>(5);
				attr.maxCount = (*i).get<int>(6);
				attr.curNo = (*i).get<int>(7);
				attrs[attr.reportid] = attr;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "读取报告信息发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 读取定值
	bool read_param_value(AllParamValue& pvalue, int maxgroupno)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				std::string sql = "SELECT * FROM TableParamGroupValue WHERE ADDR = ";
				for (auto &item:pvalue)
				{
					std::string selectSQL = sql + std::to_string(item.first);
					sqlite3pp::query qry(db, selectSQL.c_str());
					for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
					{
						for (int icount = 0; icount < maxgroupno + 1; icount++)
							pvalue[(*i).get<int>(0)][icount] = (*i).get<const char*>(icount + 1);
					}
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_param_value()发生未知错误:%s", e.what());
		}
		return true;
	}
	// 保存定值
	bool save_param_value(AllParamValue& pvalue, uint16_t curgroup)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				bool bSuccess = true;
				for (const auto& item : pvalue)
				{
					char sql[128] = {};
					sprintf(sql, "UPDATE TableParamGroupValue SET VALUE%u='%s' WHERE ADDR=%u",
						curgroup, item.second[curgroup].c_str(),item.first);

					sqlite3pp::command cmd(db, sql);
					if (SQLITE_OK != cmd.execute()) 
					{
						bSuccess = false;
						break;
					}
				}
				if (bSuccess) {
					xct.commit();
				}
				else {
					xct.rollback();
					return false;
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "save_param_value()发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 清空定值区
	bool clear_group(uint16_t groupno)
	{
		DTULOG(DTU_INFO, "清空预设区");
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			char sql[128] = {};
			if(groupno==0)
				sprintf(sql, "UPDATE TableParamGroupValue SET VALUE%u = 'NULL'",groupno);
			else
				sprintf(sql, "UPDATE TableParamGroupValue SET VALUE%u = '0.0'", groupno);
			sqlite3pp::command cmd(db, sql);
			if (SQLITE_OK != cmd.execute())
				return false;
			else
				return true;
		}
		catch (std::exception &e)
		{
			DTULOG(DTU_ERROR, "clear_pre_group()发生错误:%s", e.what());
			return false;
		}
	}
	// 定值区拷贝
	bool setting_copy(uint16_t sgropup,uint16_t dgroup)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			char sql[128] = {};
			sprintf(sql,"UPDATE TableParamGroupValue SET VALUE%u = VALUE%u",dgroup,sgropup);
			sqlite3pp::command cmd(db, sql);
			if (SQLITE_OK != cmd.execute())
				return false;
			else
				return true;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "setting_copy()发生错误:%s", e.what());
			return false;
		}
	}
	// 预设值
	bool pre_value_by_fixid(uint16_t fixid,std::string value)
	{
		try
		{
			checkDB(DBFilePath,false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			char sql[128] = {};
			sprintf(sql, "UPDATE TableParamGroupValue SET VALUE0 = '%s' WHERE ADDR = %u", value.c_str(),fixid);
			sqlite3pp::command cmd(db, sql);
			if (SQLITE_OK != cmd.execute()) {
				xct.rollback();
				return false;
			}
			else {
				if (SQLITE_OK != xct.commit())
					return false;
				else
					return true;
				return true;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "pre_value_by_fixid()发生错误:%s", e.what());
			return false;
		}
	}
	// 清空预设
	bool clear_pre_value()
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				char sql[128] = {};
				sprintf(sql, "UPDATE TableParamGroupValue SET VALUE0='NULL' WHERE VALUE0!='NULL'");
				sqlite3pp::command cmd(db, sql);
				if (SQLITE_OK != cmd.execute()) {
					xct.rollback();
					return false;
				}
				else {
					xct.commit();
					return true;
				}
					
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, " clear_pre_value()发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 通过点表值获取ParamID号
	uint16_t get_paramid_by_fixid(uint16_t fixid)
	{
		try
		{
			checkDB(DBFilePath, 0);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT PARAMID FROM TableParamInfomation WHERE ADDR=" + std::to_string(fixid);
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				return (*i).get<uint16_t>(0);
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "get_paramid_by_fixid()发生错误:%s", e.what());
			return 0;
		}
		return 0;
	}
	// 确认预设
	std::vector<uint16_t> confirm_pre_param(uint16_t destgroup)
	{
		std::vector<uint16_t> paramidGroup;
		try
		{
			checkDB(DBFilePath, paramidGroup);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			char sql[128] = {};
			sprintf(sql, "UPDATE TableParamGroupValue SET VALUE%u = VALUE0 WHERE VALUE0!='NULL'", destgroup);
			sqlite3pp::command cmd(db, sql);
			if (SQLITE_OK != cmd.execute())
				return paramidGroup;
			else
			{
				std::string retSQL = 
					"SELECT DISTINCT PARAMID FROM (SELECT * FROM TableParamInfomation INNER JOIN TableParamGroupValue ON TableParamInfomation.ADDR = TableParamGroupValue.ADDR WHERE TableParamGroupValue.VALUE0!='NULL')";
				sqlite3pp::query qry(db, retSQL.c_str());
				if (SQLITE_OK != cmd.execute())
					return paramidGroup;
				else
				{
					for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
						paramidGroup.emplace_back((*i).get<uint16_t>(0));
					return paramidGroup;
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "confirm_pre_param()发生错误:%s", e.what());
			return paramidGroup;
		}
	}
	// 获取遥控表信息
	bool read_rmctrl_table(dtuRmctrlTable &table)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableRmctrlIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				dtuRmctrlItem item{ (*i).get<uint16_t>(0) ,(*i).get<const char*>(1) ,
									static_cast<bool>((*i).get<uint16_t>(2)) ,(*i).get<uint16_t>(3) };
				table[item.fixid] = item;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_rmctrl_table()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	
	bool read_infomation_table(dtuInfoIndex &table)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableInfoInfomation WHERE INFOID=" + std::to_string(table.iid);
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				dtuInfoOneItem item{ 
					(*i).get<uint16_t>(0), (*i).get<const char*>(1), (*i).get<uint16_t>(2),
					(*i).get<uint16_t>(3), (*i).get<uint16_t>(4), (*i).get<uint16_t>(5),
					(*i).get<const char*>(6), (*i).get<int>(7)
				};
				table.info[item.fixid] = item;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_infomation_table()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	bool read_infomation_index(dtuInfoTable &index)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableInfoIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				dtuInfoIndex table;
				table.iid = (*i).get<uint16_t>(0);
				table.desc = (*i).get<const char*>(1);
				table.size = (*i).get<uint16_t>(2);
				index[table.iid] = table;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_infomation_index()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 读取CRC索引
	bool read_crc_index(dtuParamCheckIndex &crcindex)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableCRCCheck";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				crcindex[(*i).get<uint16_t>(0)] = { (*i).get<uint16_t>(1), 
					(*i).get<const char*>(2), 0.0};
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_crc_index()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	bool update_addr(std::string table, uint16_t older, uint16_t newer)
	{
		try
		{
			bool oldcheck = check_addr(table, older);
			bool newcheck = check_addr(table, newer);
			if (oldcheck == false && newcheck == true) 
			{
				checkDB(DBFilePath, false);
				sqlite3pp::database db(DBFilePath.c_str());
				sqlite3pp::transaction xct(db, true);
				{
					bool bSuccess = true;

					char sql[128] = {};
					sprintf(sql, "UPDATE %s SET ADDR=%u WHERE ADDR=%u", table.c_str(), newer, older);
					sqlite3pp::command cmd(db, sql);
					if (SQLITE_OK != cmd.execute()) {
						bSuccess = false;
					}

					if (bSuccess)
						xct.commit();
					else
						xct.rollback();
					return bSuccess;
				}
			}
			else
			{
				if (oldcheck) {
					DTULOG(DTU_ERROR,"未知的旧点表值[0x%04X]",older);
				}
				if (!newcheck) {
					DTULOG(DTU_ERROR, "新点表值重复[0x%04X]", newer);
				}
				return false;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "update_addr()发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 地址唯一性检查
	bool check_addr(std::string table,uint16_t addr)
	{
		try
		{
			checkDB(DBFilePath, 0);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				char sql[128] = {};
				sprintf(sql, "SELECT COUNT(ADDR) FROM (SELECT ADDR FROM %s WHERE ADDR=%d)", 
						table.c_str(),addr);
				sqlite3pp::query qry(db, sql);
				int count = 0;
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					count = (*i).get<int>(0);
				}
				if (count > 0) {
					return false;
				}
				else if (count == 0) {
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "check_addr()发生未知错误:%s", e.what());
			return false;
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// 更新报告
	bool updatereport(uint16_t reportid, std::string tablename,std::vector<std::string> &files, ReportType type)
	{
		try
		{
			checkDB(DBFilePath, 0);
			sqlite3pp::database db(DBFilePath.c_str());
			int count = 0;
			sqlite3pp::transaction xct(db, true);
			{
				char sql[256] = {};
				sprintf(sql, "SELECT MAX(RINDEX) - (SELECT MAXCOUNT FROM TableReportIndex WHERE REPORTID=%d) FROM %s", reportid, tablename.c_str());
				sqlite3pp::query qry(db, sql);
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					count = (*i).get<int>(0);
				}
			}

			if(type == ReportTypeFile)
			{
				// 添加要删除的文件名
				char sql[128] = {};
				sprintf(sql, "SELECT FILE FROM %s WHERE RINDEX<=%d", tablename.c_str(), count);
				sqlite3pp::query qry(db, sql);
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					files.emplace_back((*i).get<const char*>(0));
				}
			}

			// 报告数量超过上限
			if (count > 0)
			{
				// 删除最前面的几个报告
				char sql[256] = {};
				// 删除多余报告
				// 更新索引
				sprintf(sql, "DELETE FROM %s WHERE RINDEX<=%d;UPDATE %s SET RINDEX=RINDEX-%d;",
					tablename.c_str(), count, tablename.c_str(), count);
				sqlite3pp::command cmd(db, sql);
				if (SQLITE_OK != cmd.execute_all())
					xct.rollback();
				else
					xct.commit();
			}

			return true;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "updatereport() 发生未知错误:%s", e.what());
			return false;
		}
		return false;
	}
	// 更新报告序号
	bool updatereportno(uint16_t reportid, int no)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				bool bSuccess = true;

				char sql[128] = {};
				sprintf(sql, "UPDATE TableReportIndex SET CURNO=%d WHERE REPORTID=%u",no, reportid);

				sqlite3pp::command cmd(db, sql);
				if (SQLITE_OK != cmd.execute()) {
					bSuccess = false;
				}
				
				if (bSuccess)
					xct.commit();
				else
					xct.rollback();
				return bSuccess;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "updatereportno()发生错误:%s", e.what());
			return false;
		}
		return true;
	}
	// 统计某个报告表条数
	int count(std::string tablename)
	{
		try
		{
			checkDB(DBFilePath, 0);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				char sql[128] = {};
				sprintf(sql, "SELECT COUNT(RINDEX) FROM %s",tablename.c_str());
				sqlite3pp::query qry(db, sql);
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					return (*i).get<int>(0);
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "count()发生未知错误:%s", e.what());
			return 0;
		}
		return 0;
	}
	// 添加报告数据
	bool addreportdata(std::string tablename, const DTU::buffer& data, uint32_t time_s, uint32_t time_ms, ReportType type)
	{
		bool runOK = true;
		for (int errorCount=0;errorCount<3;errorCount++)
		{
			try
			{
				checkDB(DBFilePath, false);
				sqlite3pp::database db(DBFilePath.c_str());
				sqlite3pp::transaction xct(db, true);

				std::string sql;
				switch (type)
				{
				case ReportTypeNoFile: {
					sql = "INSERT INTO " + tablename + " (TIMES,TIMEUS,DATA) VALUES (" + std::to_string(time_s) + "," + std::to_string(time_ms) + ",?)";
				}; break;
				case ReportTypeFile: {
					std::string filename(data.const_data(), data.size());
					sql = "INSERT INTO " + tablename + " (TIMES,TIMEUS,FILE) VALUES (" + std::to_string(time_s) + "," + std::to_string(time_ms) + ",'" + filename + "')";
				}; break;
				}
				sqlite3pp::command cmd(db, sql.c_str());

				switch (type)
				{
					case ReportTypeNoFile: {
						cmd.bind(1, data.const_data(), data.size(), sqlite3pp::nocopy);
					}break;
					case ReportTypeFile: {
					}break;
				}

				if (SQLITE_OK != cmd.execute()) {
					xct.rollback();
					return false;
				}
				else {
					xct.commit();
					return true;
				}
			}
			catch (std::exception& e)
			{
				DTULOG(DTU_ERROR, "addreportdata()发生错误:%s", e.what());
				runOK = false;
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				DTULOG(DTU_WARN, "addreportdata()正在进行重试%d/%d", errorCount+1, 3);
				// return false;
			}

			if (runOK)
				break;	// 如果运行成功则退出
		}

		return runOK;
	}
	// 读取单条报告
	bool readreportdata(std::string tablename,uint32_t tablesize,int index, DTU::buffer& data, ReportType type)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			char sql[128] = {};
			switch(type) 
			{
				case ReportTypeNoFile: {
					sprintf(sql, "SELECT DATA FROM %s WHERE RINDEX=%d", tablename.c_str(), index);
				}break;
				case ReportTypeFile: {
					sprintf(sql, "SELECT FILE FROM %s WHERE RINDEX=%d", tablename.c_str(), index);
				}break;
				default: {
					DTULOG(DTU_ERROR,"readreportdata() 未知的ReportType:%d",static_cast<int>(type));
					return false;
				}
			}

			sqlite3pp::query qry(db, sql);
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				data.remove();
				switch (type)
				{
				case ReportTypeNoFile: {
					data.append((char*)((*i).get<void const*>(0)), tablesize);
				}break;
				case ReportTypeFile: {
					std::string temp = (*i).get<const char*>(0);
					data.append((char*)(temp.c_str()), temp.size());
				}break;
				}
			}
			return true;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "readreaportdata() 读取发生错误:%s", e.what());
			return false;
		}
	}
	// 按范围读取报告
	bool readreportdata(std::string tablename, uint32_t tablesize, int min,int max, ReportBufferAttr& data, ReportType type)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			
			char sql[128];
			switch (type)
			{
				case ReportTypeNoFile: {
					sprintf(sql, "SELECT * FROM %s WHERE RINDEX>=%d AND RINDEX<=%d", tablename.c_str(), min, max);
				}break;
				case ReportTypeFile: {
					sprintf(sql, "SELECT * FROM %s WHERE RINDEX>=%d AND RINDEX<=%d", tablename.c_str(), min, max);
				}break;
			}

			sqlite3pp::query qry(db, sql);
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				DTU::buffer oneItem;
				switch (type)
				{
				case ReportTypeNoFile: {
					oneItem.append((char*)((*i).get<void const*>(3)), tablesize);
				}break;
				case ReportTypeFile: {
					std::string onefile = (*i).get<const char*>(3);
					oneItem.append((char*)(onefile.c_str()),onefile.size());
				}break;
				}
				std::get<0>(data[(*i).get<int>(0)]) = (*i).get<int>(1);	// s
				std::get<1>(data[(*i).get<int>(0)]) = (*i).get<int>(2);	// ms
				std::get<2>(data[(*i).get<int>(0)]) = oneItem;			// 报告数据
			}
			return true;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "readreaportdata() 读取发生错误:%s", e.what());
			return false;
		}
	}
	// 清空表
	bool cleartable(std::string table)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);
			{
				bool bSuccess = true;
				char sql[384] = {};
				//清除所有内容
				//清除自增加列计数DELETE FROM sqlite_sequence WHERE name = '%s';
				sprintf(sql,"DELETE FROM %s;",table.c_str());
				sqlite3pp::command cmd(db, sql);
				if (SQLITE_OK != cmd.execute_all()) {
					bSuccess = false;
				}

				if (bSuccess)
					xct.commit();
				else
					xct.rollback();
				return bSuccess;
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "cleartable()发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 读取遥信索引表
	bool read_soe_index(dtuSOEIndex& index)
	{
		try
		{
			checkDB(DBFilePath, false);
			int nCount = 0;
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableSOEIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				bool use = ((*i).get<int>(4) == 1) ? true : false;
				index[(*i).get<uint16_t>(1)] = { (*i).get<uint16_t>(0) ,(*i).get<uint16_t>(1),
				(*i).get<const char*>(2),(*i).get<const char*>(3) , use};
				
				if (use)
					nCount++;
			}

			mapRMSystem[MAP_YX] = mapRMSystem[MAP_YX] + nCount;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_soe_index()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 读取遥测索引表
	bool read_cos_index(dtuCOSIndex& index)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			int nCount = 0;
			std::string sql = "SELECT * FROM TableCOSIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				bool use = ((*i).get<int>(4) == 1) ? true : false;
				index[(*i).get<uint16_t>(1)] = { (*i).get<uint16_t>(0) ,(*i).get<uint16_t>(1),
				(*i).get<const char*>(2),(*i).get<const char*>(3) , use};

				if (use)
					nCount++;
			}

			mapRMSystem[MAP_YC] = mapRMSystem[MAP_YC] + nCount;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_cos_index()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 读取遥控索引表
	bool read_rmc_index(dtuRmctrlIndex& index)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			int nCount = 0;
			std::string sql = "SELECT * FROM TableRMCIndex";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				bool use = ((*i).get<int>(4) == 1) ? true : false;
				index[(*i).get<uint16_t>(0)] = { (*i).get<uint16_t>(0) ,(*i).get<const char*>(1),
				(*i).get<uint16_t>(2),(*i).get<uint16_t>(3) , use };

				if (use)
					nCount++;
			}

			mapRMSystem[MAP_YK] = mapRMSystem[MAP_YK] + nCount;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "read_rmc_index()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 通过描述查找点表号
	bool get_fixid_by_desc(std::string desc,std::string tablename,uint16_t &fixid,std::string additional)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			sqlite3pp::transaction xct(db, true);

			#ifdef _WIN32
			desc = GBKToUTF8(desc);		// WIN下从GBK转换到UTF8
			#endif

			{
				std::string sql = "SELECT ADDR FROM " + tablename + " WHERE DESC='" + desc + "'" + additional;
				sqlite3pp::query qry(db, sql.c_str());
				for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
				{
					fixid = (*i).get<uint16_t>(0);
					return true;
				}
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "get_fixid_by_desc()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 获取映射信息
	bool get_map_infomation(FIXNOMAP& data)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableMap";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				oneFIXNOMAP item;
				item.tableno = (*i).get<int>(0);
				item.begin = (*i).get<int>(1);
				item.end = (*i).get<int>(2);
				item.desc = (*i).get<const char*>(3);
				item.offset = (*i).get<int>(4);

				data.emplace_back(item);
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "get_map_infomation()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

	// 获取用户信息
	bool get_user_infomation(UserInfo& data)
	{
		try
		{
			checkDB(DBFilePath, false);
			sqlite3pp::database db(DBFilePath.c_str());
			std::string sql = "SELECT * FROM TableUserManage";
			sqlite3pp::query qry(db, sql.c_str());
			for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
			{
				oneUserInfo item;
				item.name = (*i).get<const char*>(1);
				item.password = (*i).get<const char*>(2);
				item.power = static_cast<UPOWER>((*i).get<int>(3));
				if((*i).get<const char*>(4))
					item.word = (*i).get<const char*>(4);

				data.emplace_back(item);
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "get_user_infomation()读取发生错误:%s", e.what());
			return false;
		}
		return true;
	}

private:
	std::string DBFilePath;
};

/*--------------------------------------- 定值区管理 --------------------------------------------------*/
class SettingGroupMgr {
private:
	SettingGroupMgr() {
		if (!this->init())
			DTULOG(DTU_ERROR, "SettingGroupMgr()初始化定值区失败!");
	}
public:
	static SettingGroupMgr& instance() {
		static SettingGroupMgr attr;
		return attr;
	}
public:
	bool init()
	{
		static bool isInit = false;
		if (isInit)
			return true;
		else
			isInit = true;
		if (!DBOperate::instance().read_group(_curr_group, _edit_group, _max_group))
			return false;
		return true;
	}
	void set_cur_group(uint16_t cgroup)
	{
		if (cgroup <= _max_group) {
			if (!DBOperate::instance().save_group(cgroup, _edit_group)) {
				DTULOG(DTU_ERROR,"保存当前区失败");
				return;
			}
			_curr_group = cgroup;
		}
	}
	void set_edit_group(uint16_t egroup)
	{
		if (egroup <= _max_group) {
			if (!DBOperate::instance().save_group(_curr_group, egroup)) {
				DTULOG(DTU_ERROR, "保存编辑区失败");
				return;
			}
			_edit_group = egroup;
		}
	}
	uint16_t get_edit_group() const
	{
		return _edit_group;
	}
	uint16_t get_cur_group() const
	{
		return _curr_group;
	}
	uint16_t max_group() const
	{
		return _max_group;
	}
private:
	uint16_t _edit_group = 1;
	uint16_t _curr_group = 1;
	uint16_t _max_group = 4;
};
#define CURRENT_GROUP() SettingGroupMgr::instance().get_cur_group()

#define EDIT_GROUP() SettingGroupMgr::instance().get_edit_group()

#define MAX_GROUP() SettingGroupMgr::instance().max_group()
/*------------------------------------- 不可变更信息管理 -----------------------------------------------*/
class UnchangeInfoMgr {
private:
	UnchangeInfoMgr() {
		if (!this->init())
			DTULOG(DTU_ERROR, "加载不可变更信息失败");
	}
public:
	static UnchangeInfoMgr& instance() {
		static UnchangeInfoMgr attr;
		return attr;
	}
public:
	// 初始化加载
	bool init()
	{
		// 确保只加载一次
		static bool isInit = false;

		if (isInit)
			return true;
		else
			isInit = true;

		// 总返回结果
		bool result = true;
		// 单个函数返回结果
		bool ret = true;
		// 加载定值
		ret = DBOperate::instance().read_param_index(Paramindextable);
		result = ret && result;
		for (auto &item : Paramindextable)
		{
			if (!DBOperate::instance().read_param_table(item.second))
			{
				ret = ret && false;
				DTULOG(DTU_ERROR, "初始化定值[0x%04d]失败", item.first);
				continue;
			}
		}
		// 加载报告(下面两个函数顺序不可颠倒)
		ret = DBOperate::instance().read_report_table(Archindextable);
		result = ret && result;
		ret = DBOperate::instance().read_report_index(Archindextable);
		result = ret && result;
		// 加载遥控表
		ret = DBOperate::instance().read_rmctrl_table(RmctrlTable);
		result = ret && result;
		// 加载信息表
		ret = DBOperate::instance().read_infomation_index(InfoTable);
		result = ret && result;
		for (auto& item : InfoTable)
		{
			if (!DBOperate::instance().read_infomation_table(item.second))
			{
				ret = ret && false;
				DTULOG(DTU_ERROR, "初始化信息表[0x%04d]失败", item.first);
				continue;
			}
		}
		// 加载SOE索引
		ret = DBOperate::instance().read_soe_index(SOEIndex);
		result = ret && result;
		// 加载COS索引
		ret = DBOperate::instance().read_cos_index(COSIndex);
		result = ret && result;
		// 加载遥控索引
		ret = DBOperate::instance().read_rmc_index(RMCIndex);
		result = ret && result;
		// 获取映射信息
		ret = DBOperate::instance().get_map_infomation(MAPTable);
		result = ret && result;
		// 加载用户信息表
		ret = DBOperate::instance().get_user_infomation(UserTable);
		result = ret && result;
		return ret;
	}
	// 获取定值信息
	const dtuParamIndexTable &GetParamInfo()
	{
		return Paramindextable;
	}
	// 通过ParamID获取参数信息
	const dtuOneParamIndex &GetParamInfoByID(uint16_t paramid)
	{
		auto ret = Paramindextable.find(paramid);
		if (ret != Paramindextable.end())
			return (*ret).second;
		else
			DTULOG(DTU_ERROR, "未找到参数配置,参数ID:[0x%04d]", paramid);
		const static dtuOneParamIndex fault;
		return fault;
	}
	// 通过点表号获取该点表的信息
	const dtuOneParamInfo &GetOneParamInfoByFix(uint16_t fixid)
	{
		for (const auto &item : Paramindextable)
		{
			for (const auto& item1 : item.second.info)
			{
				if (item1.first == fixid)
					return item1.second;
			}
		}
		static dtuOneParamInfo fault;
		return fault;
	}
	// 获取所有报告信息
	const dtuReportIndexTable &GetReportInfo()
	{
		return Archindextable;
	}
	// 按报告ID获取获取报告信息
	const dtuOneReportIndex&GetReportInfoByID(uint16_t reportid)
	{
		auto ret = Archindextable.find(reportid);
		if (ret != Archindextable.end())
			return (*ret).second;
		else
			DTULOG(DTU_ERROR, "未找到报告配置,报告ID:[0x%04d]", reportid);
		const static dtuOneReportIndex fault{};
		return fault;
	}
	// 根据点表号创建定值表
	bool CreateAllParamValueByParamID(AllParamValue &apv,uint16_t paramid,int maxGroup)
	{
		auto ita = Paramindextable.find(paramid);
		if (ita != Paramindextable.end())
		{
			apv.clear();
			for (auto& item : ita->second.info)
			{
				apv[item.second.fixid] = std::vector<std::string>();
				for (int m = 0; m < maxGroup + 1; m++)
					apv[item.second.fixid].emplace_back(std::string("0.0"));
			}
			return true;
		}
		else
		{
			DTULOG(DTU_ERROR, "CreateAllParamValueByParamID()错误的paramid[0x%04d]", paramid);
			return false;
		}

	}
	// 获取遥控表
	const dtuRmctrlTable &GetRmctrlTable()
	{
		return RmctrlTable;
	}
	// 获取信息查看表信息
	const dtuInfoTable &GetInfomationTable()
	{
		return InfoTable;
	}
	// 获取InfoID获取某一信息表信息
	const dtuInfoIndex &GetInfomationTableByID(uint16_t infoID)
	{
		auto ret = InfoTable.find(infoID);
		if (ret != InfoTable.end())
			return (*ret).second;
		else
			DTULOG(DTU_ERROR, "未找到参数配置,信息ID:[0x%04d]", infoID);
		const static dtuInfoIndex fault;
		return fault;
	}
	// 获取SOE索引表
	const dtuSOEIndex &GetSOEIndex()
	{
		return SOEIndex;
	}
	// 获取COS索引表
	const dtuCOSIndex &GetCOSIndex()
	{
		return COSIndex;
	}
	// 获取遥控映射索引
	const dtuRmctrlIndex& GetRMCIndex()
	{
		return RMCIndex;
	}
	// 获取映射信息
	const FIXNOMAP& GetMapTable()
	{
		return MAPTable;
	}
	// 获取用户信息
	const UserInfo& GetUserTable()
	{
		return UserTable;
	}


private:
	dtuParamIndexTable Paramindextable;
	dtuReportIndexTable Archindextable;
	dtuRmctrlTable RmctrlTable;
	dtuInfoTable InfoTable;
	dtuSOEIndex SOEIndex;
	dtuCOSIndex COSIndex;
	dtuRmctrlIndex RMCIndex;
	FIXNOMAP MAPTable;
	UserInfo UserTable;

};

/*--------------------------------------- 读写定值处理 ------------------------------------------------*/
class dtuOneParameterItem {
public:
	dtuOneParameterItem() {}
	dtuOneParameterItem(uint16_t id) :fixid(id) {}
public:
	DTU::buffer valueToBuff(ParamType type,uint32_t size,uint16_t groupno) const
	{
		DTU::buffer v;
		if (groupno > vvalue.size() - 1)
			return v;
		v.resize(size);
		switch (type)
		{
		case PType8_t: {
			uint8_t sv = (uint8_t)strtol(vvalue[groupno].c_str(), nullptr, 10);
			v.set(0, (char*)&sv,sizeof(sv));
		}break;
		case PType16_t: {
			uint16_t sv = (uint16_t)strtol(vvalue[groupno].c_str(), nullptr, 10);
			v.set(0, (char*)&sv, sizeof(sv));
		}break;
		case PType32_t: {
			uint32_t sv = (uint32_t)strtol(vvalue[groupno].c_str(), nullptr, 10);
			v.set(0, (char*)&sv, sizeof(sv));
		}break;
		case PTypeFLO_t: {
			float sv = strtofloat(vvalue[groupno].c_str());
			v.set(0, (char*)&sv, sizeof(sv));
		}break;
		case PTypeSTR_t: {
			if(vvalue[groupno].size() < size)
				v.set(0, vvalue[groupno].c_str(), vvalue[groupno].size());
			else
				v.set(0, vvalue[groupno].c_str(), size);
		}break;
		case PTypeIPW_t: {
			uint32_t ip = IPToInt(vvalue[groupno].c_str());
			v.set(0,(char*)&ip,sizeof(ip));
		}break;
		default: {
			DTULOG(DTU_ERROR, "未知的数据类型:%u", type);
		}
		}
		return v;
	}
	std::string buffToStr(DTU::buffer buff, ParamType type)
	{
		try
		{
			switch (type)
			{
			case PType8_t: {
				uint8_t v = buff.value<uint8_t>();
				return std::to_string(v);
			}break;
			case PType16_t: {
				uint16_t v = buff.value<uint16_t>();
				return std::to_string(v);
			}break;
			case PType32_t: {
				uint32_t v = buff.value<uint32_t>();
				return std::to_string(v);
			}break;
			case PTypeFLO_t: {
				float v = buff.value<float>();
				return std::to_string(v);
			}break;
			case PTypeSTR_t: {
				return std::string(buff.data(),buff.size());
			}break;
			case PTypeIPW_t: {
				uint32_t v = buff.value<uint32_t>();
				return IntToIP(v);
			}break;
			default: {
				DTULOG(DTU_ERROR, "buffToStr()未知的数据类型:%u", type);
				return "";
			}
			}
		}
		catch (std::exception &e)
		{
			DTULOG(DTU_ERROR, "buffToStr()发生未知错误:%s", e.what());
			return "";
		}
	}
	void SetValue(const DTU::buffer& buff, ParamType type, uint16_t groupno, uint16_t fixid)
	{
		this->vvalue[groupno] = buffToStr(buff, type);
		this->fixid = fixid;
	}
	void SetValue(const std::string& value, uint16_t groupno, uint16_t fixid)
	{
		this->vvalue[groupno] = value;
		this->fixid = fixid;
	}
	////
	std::string &value(uint16_t groupno)
	{
		auto test = vvalue.size();
		if (groupno > vvalue.size() - 1)
		{
			static std::string fault; //组号不对直接返回空
			return fault;
		}
		return vvalue[groupno];
	}
	bool value(uint16_t fixid,uint16_t groupno,std::string val)
	{
		if (groupno > vvalue.size() - 1)
			return false;
		this->fixid = fixid;
		vvalue[groupno] = val;
		return true;
	}
	std::vector<std::string> &values()
	{
		return vvalue;
	}
private:
	uint16_t fixid = 0;
	std::vector<std::string> vvalue;
};

class dtuParameter {
public:
	dtuParameter(uint16_t paramid,bool isMultiGroup) : _id(paramid)
	{
		for (const auto& item : UnchangeInfoMgr::instance().GetParamInfoByID(paramid).info)
		{
			params[item.first] = dtuOneParameterItem(item.first);
			int count = 0;
			if (isMultiGroup)
				count = MAX_GROUP() + 1;
			else
				count = 2;
			for (int i = 0; i < count; i++)
				params[item.first].values().emplace_back(std::string("0.0"));
		}
	}
public:

	std::string get_value(uint16_t id, uint16_t group)
	{
		auto ita = params.find(id);
		if (ita != params.end())
			return ita->second.value(group);
		return "";
	}

	std::vector<std::string> get_value(uint16_t group)
	{
		std::vector<std::string> ret;
		for (auto& item : params)
		{
			ret.emplace_back(item.second.value(group));
		}
		return ret;
	}

	bool set_value(uint16_t id, const std::string& value, uint16_t group)
	{
		auto ita = params.find(id);
		if (ita != params.end())
			return ita->second.value(id, group, value);
		else
			return false;
	}

	DTU::buffer pack(uint16_t group)
	{
		const dtuOneParamIndex& attr = UnchangeInfoMgr::instance().GetParamInfoByID(_id);
		DTU::buffer buff;
		buff.resize(attr.size);
		try 
		{
			for (const auto& item : params)
			{
				uint32_t offset = attr.info.at(item.first).offset;
				DTU::buffer insertdata = item.second.valueToBuff(static_cast<ParamType>(attr.info.at(item.first).type),
					attr.info.at(item.first).size, group);
				buff.set(offset, insertdata);
			}
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR, "dtu_parameter::serialize 参数%u错误%s", _id, e.what());
			return DTU::buffer();
		}
		return buff;
	}

	bool unpack(const DTU::buffer& buff, uint16_t group)
	{
		const dtuOneParamIndex& attr = UnchangeInfoMgr::instance().GetParamInfoByID(_id);
		if (buff.size() < attr.size)
			return false;
		for (auto& item : params)
		{
			uint32_t size = attr.info.at(item.first).size;
			uint32_t offset = attr.info.at(item.first).offset;
			ParamType type = static_cast<ParamType>(attr.info.at(item.first).type);
			item.second.SetValue(item.second.buffToStr(buff.get(offset, size), type), group, item.first);
		}
		return true;
	}

	// 定值区复制
	bool setting_copy(uint16_t srcgroup,uint16_t dstgroup)
	{
		if (srcgroup > MAX_GROUP() || dstgroup > MAX_GROUP())
		{
			DTULOG(DTU_ERROR, "setting_copy()定值区超出范围[1-%u]", MAX_GROUP());
			return false;
		}
		AllParamValue retmap;
		for (auto& item : params)
		{
			item.second.value(dstgroup) = item.second.value(srcgroup);
			retmap[item.first] = item.second.values();
		}
		//return DBOperate::instance().save_param_value(retmap, dstgroup);
		return true;
	}
	
	int count()
	{
		return params.size();
	}
	
	std::string GetParamAttributeBuff()
	{
		std::string ret;
		dtuOneParamIndex oneindex = UnchangeInfoMgr::instance().GetParamInfoByID(_id);
		for (auto& item : oneindex.info)
		{
			ret = ret + std::to_string(item.second.fixid);
			ret = ret + item.second.desc;
			ret = ret + item.second.unit;
			ret = ret + std::to_string(item.second.max);
			ret = ret + std::to_string(item.second.min);
		}
		return ret;
	}

public:
	// 参数ID
	uint16_t _id;
	std::map<uint16_t, dtuOneParameterItem> params;
};

/*--------------------------------------- 读写报告处理 ------------------------------------------------*/
class dtuArchiveMgr 
{
private:
	dtuArchiveMgr() {
		init();
	}
	~dtuArchiveMgr() {}
public:
	static dtuArchiveMgr& instance() {
		static dtuArchiveMgr attr;
		return attr;
	}
public:
	bool init()
	{
		static bool isInit = false;
		if (isInit)
			return true;
		else
			isInit = true;
		Archivemap = UnchangeInfoMgr::instance().GetReportInfo();
		return true;
	}

	// 添加报告
	bool add_report(uint16_t reportid, uint32_t time_s, uint32_t time_ms, const DTU::buffer& buf)
	{
		std::lock_guard<std::mutex> lock(_lock);
		auto ita = Archivemap.find(reportid);
		if (ita != Archivemap.end()) {
			if (ita->second.curNo < ita->second.maxCount) {
				// 未达到上限 添加报告
				bool ret = DBOperate::instance().addreportdata(ita->second.table, buf, time_s, time_ms, ita->second.type);
				if (ret) {
					ita->second.curNo++;
					DBOperate::instance().updatereportno(ita->second.reportid,ita->second.curNo);
				}
				return ret;
			}
			else {
				std::vector<std::string> fileList;
				// 达到上限 先添加报告 再 更新报告
				bool ret = DBOperate::instance().addreportdata(ita->second.table, buf, time_s, time_ms, ita->second.type);
				if(ret)
					ret = DBOperate::instance().updatereport(reportid, ita->second.table, fileList, ita->second.type);
				if ((ita->second.type == ReportTypeFile) && (fileList.size() > 0))
				{
					// 删除文件
					if (ClearOneCallback != nullptr) {
						ClearOneCallback(ita->second.paths, fileList);
					}
					else {
						ret = false;
						DTULOG(DTU_WARN, "部分文件清理函数未设置,文件未被清理");
					}
				}
				return ret;
			}
		}
		else {
			DTULOG(DTU_ERROR, "错误的报告类型:%u", reportid);
			return false;
		}
	}
	// 读取单条报告
	bool read_report(uint16_t reportid, uint32_t no, DTU::buffer& data)
	{
		std::lock_guard<std::mutex> lock(_lock);
		auto ita = Archivemap.find(reportid);
		if (ita != Archivemap.end()) {
			return DBOperate::instance().readreportdata(ita->second.table, ita->second.size, no, data, ita->second.type);
		}
		else
			return false;
	}
	// 根具范围获取报告
	bool read_report(uint16_t reportid, int min,int max, ReportBufferAttr &data)
	{
		std::lock_guard<std::mutex> lock(_lock);
		auto ita = Archivemap.find(reportid);
		if (ita != Archivemap.end())
			return DBOperate::instance().readreportdata(ita->second.table, ita->second.size, min, max, data, ita->second.type);
		else
			return false;
	}
	// 获取当前存储的序号
	uint32_t get_report_no(uint16_t reportid)
	{
		std::lock_guard<std::mutex> lock(_lock);
		auto ita = Archivemap.find(reportid);
		if (ita != Archivemap.end()) {
			return ita->second.curNo;
		}
		else {
			DTULOG(DTU_ERROR,"未知的ReportID:%u",reportid);
			return 0;
		}
	}
	// 清空表
	bool clearReport(uint16_t reportid)
	{
		DTULOG(DTU_INFO,"ReportID[0x%04u]清空报告",reportid);
		std::lock_guard<std::mutex> lock(_lock);
		try
		{
			auto ita = Archivemap.find(reportid);
			if (ita != Archivemap.end()) {
				// 清理数据库
				DBOperate::instance().cleartable(ita->second.table);
				// 更新报告索引号
				ita->second.curNo = 0;
				DBOperate::instance().updatereportno(ita->second.reportid, ita->second.curNo);
				// 如果是带文件的报告则还要删除文件
				if (ita->second.type == ReportTypeFile) {
					if (ClearAllCallback != nullptr) {
						ClearAllCallback(ita->second.paths);
					}
					else {
						DTULOG(DTU_WARN, "全部文件清理函数未设置,文件未被清理");
					}
				}
				return true;
			}
			else
				return false;
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR,"clearReport() 发生错误:%s",e.what());
			return false;
		}
	}
	// 设置文件夹前置路径
	void setFilePre(std::string path)
	{
		FilePre = path;
	}
	// 设置清理单个文件回调
	void set_clear_one_callback(ClearOneFileCallback callback)
	{
		ClearOneCallback = callback;
	}
	// 设置清理所有文件回调
	void set_clear_all_Callback(ClearAllFileCallback callback)
	{
		ClearAllCallback = callback;
	}

private:
	dtuReportIndexTable Archivemap;
	ClearAllFileCallback ClearAllCallback = nullptr;
	ClearOneFileCallback ClearOneCallback = nullptr;
	std::string FilePre;
	std::mutex _lock;	// 当要修改CurNo时进行加锁处理
};

/*--------------------------------------- 对外开放处理1 -----------------------------------------------*/
const static std::vector<ParamID> MultiGroupVec = { 
	ParamRoutine,ParamAutoReclose,ParamAutoLocal,ParamDistributFA,ParamSynchronousClose,
	ParamAutoSplit,ParamSmallCurrent,ParamDisconnWarn,ParamDriveSwitch,
};

dtuParam::dtuParam(ParamID paramid)
{
	// 检查改定值是否是多定值区定值
	auto ita = find(MultiGroupVec.begin(), MultiGroupVec.end(), paramid);
	if (ita != MultiGroupVec.end())
		isChange = true;
	// 设置定值类型
	_paramid = static_cast<uint16_t>(paramid);
	// 创建参数指针
	params = new dtuParameter(static_cast<uint16_t>(paramid),isChange);
	if (params == nullptr)
		DTULOG(DTU_ERROR, "dtuParam初始化失败");
}

dtuParam::dtuParam(uint16_t paramid)
{
	// 检查改定值是否是多定值区定值
	auto ita = find(MultiGroupVec.begin(), MultiGroupVec.end(), static_cast<uint16_t>(paramid));
	if (ita != MultiGroupVec.end())
		isChange = true;
	// 设置定值类型
	_paramid = paramid;
	// 创建参数指针
	params = new dtuParameter(paramid, isChange);
	if (params == nullptr)
		DTULOG(DTU_ERROR, "dtuParam初始化失败");
}

dtuParam::~dtuParam()
{
	if (params)
		delete static_cast<dtuParameter*>(params);
}

bool dtuParam::GetCurParamValue(std::vector<std::string> &values)
{
	if (!params)return false;
	if (isChange)
		values = static_cast<dtuParameter*>(params)->get_value(CURRENT_GROUP());
	else
		values = static_cast<dtuParameter*>(params)->get_value(1);
	if (values.empty())
	{
		DTULOG(DTU_ERROR, "GetAllParamValue()失败");
		return false;
	}
	return true;
}

bool dtuParam::GetValueByFixid(uint16_t id, std::string& value)
{
	if (!params)return false;
	if(isChange)
		value = static_cast<dtuParameter*>(params)->get_value(id, CURRENT_GROUP());
	else
		value = static_cast<dtuParameter*>(params)->get_value(id, 1);
	if (value.empty())
	{
		DTULOG(DTU_ERROR, "GetValueByFixid()失败,当前定值[%02d]可能未找到点表[0x%04X]", _paramid, id);
		return false;
	}
	return true;
}

bool dtuParam::SetValueByFixid(uint16_t id, const std::string& value)
{
	bool ret = false;
	if (!params)return ret;
	if(isChange)
		ret = static_cast<dtuParameter*>(params)->set_value(id, value, CURRENT_GROUP());
	else
		ret = static_cast<dtuParameter*>(params)->set_value(id, value, 1);
	if (!ret)
		DTULOG(DTU_ERROR, "SetValueByFixid()失败,当前定值[%02d]可能未找到点表[0x%04X]", _paramid, id);
	return ret;
}

bool dtuParam::PreValueByFixid(uint16_t id, const std::string& value)
{
	bool ret = false;
	if (!params)return ret;
	ret = static_cast<dtuParameter*>(params)->set_value(id, value, 0);
	if (!ret)
		DTULOG(DTU_ERROR, "PreValueByFixid()失败,当前定值[%02d]可能未找到点表[0x%04X]", _paramid, id);
	return ret;
}

bool dtuParam::copy(uint16_t srcgroupno, uint16_t dstgroupno)
{
	if (!params)return false;
	if (dstgroupno > GroupNum)return false;
	return static_cast<dtuParameter*>(params)->setting_copy(srcgroupno, dstgroupno);
}

DTU::buffer dtuParam::pack()
{
	if (!params)return DTU::buffer();
	if(isChange)
		return static_cast<dtuParameter*>(params)->pack(CURRENT_GROUP());
	else
		return static_cast<dtuParameter*>(params)->pack(1);
}

bool dtuParam::unpack(const DTU::buffer& buff)
{
	if (!params)return false;
	if (isChange)
		return static_cast<dtuParameter*>(params)->unpack(buff, CURRENT_GROUP());
	else
		return static_cast<dtuParameter*>(params)->unpack(buff, 1);
}

uint16_t dtuParam::GetParamID()
{
	if (!params)return 0;
	return static_cast<dtuParameter*>(params)->_id;
}

int dtuParam::GetMaxGroup()
{
	if (isChange)
		return MAX_GROUP();
	else
		return 1;
}

std::string dtuParam::GetParamAttributeBuff()
{
	return static_cast<dtuParameter*>(params)->GetParamAttributeBuff();
}

int dtuParam::count()
{
	return static_cast<dtuParameter*>(params)->count();
}

void dtuParam::GetAllParamValue(AllParamValue& param)
{
	if (!params)return;
	for (auto& item : static_cast<dtuParameter*>(params)->params)
	{
		param[item.first] = item.second.values();
	}
}

void dtuParam::SetAllParamValue(AllParamValue& param)
{
	if (!params)return;
	for (auto& item : static_cast<dtuParameter*>(params)->params)
	{
		item.second.values() = param[item.first];
	}
}

/*--------------------------------------- 对外开放处理2 -----------------------------------------------*/
DBManager::DBManager(){}

DBManager::~DBManager(){}

bool DBManager::init(std::string dbPath)
{
	// 确保只加载一次
	static bool isInit = false;
	if (isInit)
		return true;
	else
		isInit = true;
	DTULOG(DTU_INFO, "初始化数据库[%s]", dbPath.c_str());
	// 
	bool ret = true;
	// 数据库读写初始化
	DBOperate::instance().SetDBPath(dbPath);
	// 清空预设区
	DBOperate::instance().clear_group(0);
	// 定值区初始化
	ret = ret && SettingGroupMgr::instance().init();
	// 不可变更信息初始化
	ret = ret && UnchangeInfoMgr::instance().init();

	StaticMAPtable = UnchangeInfoMgr::instance().GetMapTable();
	return ret;
}

bool DBManager::readParamValue(dtuParam &param)
{
	dtuParam::AllParamValue allparam;
	UnchangeInfoMgr::instance().CreateAllParamValueByParamID(allparam,param.GetParamID(),param.GetMaxGroup());
	DBOperate::instance().read_param_value(allparam,param.GetMaxGroup());
	if (allparam.size() == 0)
		return false;
	param.SetAllParamValue(allparam);
	return true;
}

bool DBManager::saveParamValue(dtuParam &param)
{
	dtuParam::AllParamValue allparam;
	param.GetAllParamValue(allparam);
	//DBOperate::instance().read_param_value(allparam, param.GetMaxGroup());
	return DBOperate::instance().save_param_value(allparam, min(param.GetMaxGroup(), CURRENT_GROUP()));
}

uint16_t DBManager::GetParamIDByFixid(uint16_t fixid)
{
	return DBOperate::instance().get_paramid_by_fixid(fixid);
}

bool DBManager::setPreParamValue(uint16_t fixid, std::string value)
{
	return DBOperate::instance().pre_value_by_fixid(fixid,value);
}

bool DBManager::setPreParamValue(uint16_t fixid, DTU::buffer value)
{
	dtuOneParamInfo ret = UnchangeInfoMgr::instance().GetOneParamInfoByFix(fixid);
	dtuOneParameterItem item;
	std::string valuestr = item.buffToStr(value,static_cast<ParamType>(ret.type));

	// 转换失败直接返回
	if (valuestr == "")
		return false;

	double tempval =std::atof(valuestr.c_str());
	switch(ret.type)
	{
	case PType8_t:
	case PType16_t:
	case PType32_t:
	case PTypeFLO_t: { // 这几种类型需要判断范围是否正确
		if (tempval < ret.min || tempval > ret.max)
			return false;
	}break;
	case PTypeSTR_t: // 这两种数据不做处理
	case PTypeIPW_t: break;
	}

	return DBOperate::instance().pre_value_by_fixid(fixid, valuestr);
}

bool DBManager::confirmPreParamValue(DSPSendFunc func,uint16_t reboot)
{
	try
	{
		uint16_t egroup = EDIT_GROUP();
		uint16_t cgroup = CURRENT_GROUP();
		// 第一步确认预置,返回修改的ParamID号
		auto ret = DBOperate::instance().confirm_pre_param(EDIT_GROUP());
		// 第二步将修改过的定值下发到DSP
		// 如果预置区是当前定值区则将定值下发
		if (cgroup == egroup)
		{
			if (ret.size() > 0) {
				DTULOG(DTU_INFO, "定值区[%d]定值下发到DSP", cgroup);
			}	
			else {
				DTULOG(DTU_ERROR, "定值区[%d]定值下发到DSP失败,下发定值号为空", cgroup);
				return false;
			}

			for (auto item : ret)
			{
				uint16_t cmd = Get_W_CMD_By_ParamID(item);
				dtuParam param(item);
				readParamValue(param);
				func(cmd, param.pack(), reboot);
			}
		}
		// 第三步清理预设的值
		DBOperate::instance().clear_pre_value();
		return true;
	}
	catch(std::exception &e)
	{
		DTULOG(DTU_ERROR, "预设固化定值发生错误:%s", e.what());
		return false;
	}
}

bool DBManager::cancelPreParamValue()
{
	// 清除预设区的值
	return DBOperate::instance().clear_group(0);
}

bool DBManager::copyAllParamGroup(uint16_t src, uint16_t dest)
{
	if (dest == CURRENT_GROUP())
	{
		DTULOG(DTU_ERROR, "定值区复制不可复制到前定值区");
		return false;
	}
	if (dest < 1 || dest > MAX_GROUP())
		return false;
	if (src < 1 || src > MAX_GROUP())
		return false;
	if (src == dest)
		return true;
	return DBOperate::instance().setting_copy(src,dest);
}

bool DBManager::setAllParamToDSP(DSPSendFunc sendfunc, uint16_t reboot)
{
	// 多定值区定值表
	// 切换定值区的时候只把多定值区定值下发即可
	const static std::vector<ParamID> sendlist = {
		ParamRoutine,ParamAutoReclose,ParamAutoLocal,ParamDistributFA,ParamSynchronousClose,
		ParamAutoSplit,ParamSmallCurrent,ParamDisconnWarn,ParamDriveSwitch
	};
	try 
	{
		for (auto item : sendlist)
		{
			dtuParam Param(item);
			readParamValue(Param);
			uint16_t cmd = Get_W_CMD_By_ParamID(item);
			sendfunc(cmd, Param.pack(), reboot);
			//延时 防止下发过快
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		return true;
	}
	catch(std::exception &e)
	{
		DTULOG(DTU_ERROR, "setAllParam()发生错误:%s", e.what());
		return false;
	}
}

uint16_t DBManager::GetEditGroup()
{
	return SettingGroupMgr::instance().get_edit_group();
}

uint16_t DBManager::GetCurrGroup()
{
	return SettingGroupMgr::instance().get_cur_group();
}

uint16_t DBManager::GetMaxiGroup()
{
	return SettingGroupMgr::instance().max_group();
}

bool DBManager::SetEditGroup(uint16_t groupno)
{
	DTULOG(DTU_INFO, "选择编辑定值区[%u]", groupno);
	SettingGroupMgr::instance().set_edit_group(groupno);
	return true;
}

bool DBManager::SetCurrGroup(uint16_t groupno)
{
	SettingGroupMgr::instance().set_cur_group(groupno);
	return true;
}

bool DBManager::ChangeCurGroup(uint16_t targetgroup, DSPSendFunc callback, uint16_t reboot)
{
	try
	{
		if (targetgroup > MAX_GROUP() || targetgroup < 1)
		{
			DTULOG(DTU_ERROR, "定值区号%02u错误", targetgroup);
			return false;
		}
		uint16_t curgroup = CURRENT_GROUP();

		DTULOG(DTU_INFO, "切换定值区 目标定值区[%d] 当前定值区[%d] [%s]", targetgroup, curgroup, (targetgroup==curgroup) ? "不切换": "切换");
		if (curgroup == targetgroup)
			return true;

		// 设置当前定值区号
		SettingGroupMgr::instance().set_cur_group(targetgroup);
		// 下发新定值区所有定值
		if (!setAllParamToDSP(callback, reboot))
		{
			DTULOG(DTU_ERROR, "切换定值区发生错误,回退定值区号");
			SettingGroupMgr::instance().set_cur_group(curgroup);
			return false;
		}
		return true;
	}
	catch (std::exception &e)
	{
		DTULOG(DTU_ERROR, "切换定值区发生错误%s\n", e.what());
		return false;
	}
}

const dtuReportIndexTable& DBManager::GetReportInfo()
{
	return UnchangeInfoMgr::instance().GetReportInfo();
}

const dtuOneReportIndex& DBManager::GetReportInfoByID(uint16_t reportid)
{
	return UnchangeInfoMgr::instance().GetReportInfoByID(reportid);
}

uint32_t DBManager::GetCurReportNoByID(uint16_t reportid)
{
	return dtuArchiveMgr::instance().get_report_no(reportid);
}

bool DBManager::addReport(uint16_t reportid, uint32_t time_s, uint32_t time_ms, const DTU::buffer& data)
{
	return dtuArchiveMgr::instance().add_report(reportid, time_s, time_ms, data);
}

bool DBManager::readReportByIndex(uint16_t reportid,uint32_t index,DTU::buffer &data)
{
	return dtuArchiveMgr::instance().read_report(reportid,index,data);
}

bool DBManager::readReport(uint16_t reportid, int min, int max, ReportBufferAttr& data)
{
	return dtuArchiveMgr::instance().read_report(reportid,min,max,data);
}

bool DBManager::clearReport(uint16_t reportid)
{
	// 如果是动作报告要连动作简报一起删除
	if (reportid == ReportPro) {
		dtuArchiveMgr::instance().clearReport(ReportProSimple);
	}
	return dtuArchiveMgr::instance().clearReport(reportid);
}

void DBManager::setReportFilePathPre(std::string path)
{
	dtuArchiveMgr::instance().setFilePre(path);
}

void DBManager::setReportClearOneCallback(ClearOneFileCallback callback)
{
	dtuArchiveMgr::instance().set_clear_one_callback(callback);
}

void DBManager::setReportClearAllCallback(ClearAllFileCallback callback)
{
	dtuArchiveMgr::instance().set_clear_all_Callback(callback);
}

#define CMD_W 0x00
#define CMD_R 0x01
#define CMD_T 0x02

const static std::map<ParamID, std::tuple<uint16_t, uint16_t,uint16_t>> Param_RW_CMD_Map = {
	{ParamPublic			,{PC_W_PUB_FIX,			PC_R_PUB_FIX,			TX_PC_PUB_FIX}},			//公共定值
	{ParamSoftPress			,{PC_W_YB_ON_OFF_INFO,	PC_R_YB_STATE_INFO,		TX_PC_SOFT_YB_FIX}},		//压板信息
	{ParamGroupNo           ,{PC_W_FIX_AREA_INFO,   PC_R_FIX_AREA_INFO,     TX_PC_FIX_AREA_INFO}},      //定值区读写
	{ParamRoutine			,{PC_W_PRO_FIX,			PC_R_PRO_FIX,			TX_PC_PRO_FIX}},			//常规保护定值
	{ParamAutoReclose		,{PC_W_AUTORECLOSE_FIX,	PC_R_AUTORECLOSE_FIX,	TX_PC_AUTORECLOSE_FIX}},	//自动重合闸
	{ParamAutoLocal			,{PC_W_FA_FIX,			PC_R_FA_FIX,			TX_PC_FA_FIX}},				//就地馈线自动化FA
	{ParamDistributFA		,{PC_W_DFA_FIX,			PC_R_DFA_FIX,			TX_PC_DFA_FIX}},			//分布式FA定值
	{ParamSynchronousClose	,{PC_W_TQHZ_FIX,		PC_R_TQHZ_FIX,			TX_PC_TQHZ_FIX}},			//同期合闸定值
	{ParamAutoSplit			,{PC_W_AUTO_SPLIT_FIX,	PC_R_AUTO_SPLIT_FIX,	TX_PC_AUTOSPLIT_FIX}},		//自动解列定值
	{ParamSmallCurrent		,{PC_W_XDLGND_FIX,		PC_R_XDLGND_FIX,		TX_PC_XDLGND_FIX}},			//小电流接地定值
	{ParamDisconnWarn		,{PC_W_LINEBRKALARM_FIX,PC_R_LINEBRKALARM_FIX,	TX_PC_LINEBRKALARM_FIX}},	//线路断线告警定值
	{ParamDriveSwitch		,{PC_W_POWERDRIVER_FIX,	PC_R_POWERDRIVER_FIX,	TX_PC_POWERDRIVER_FIX}},	//传动开关定值
	{ParamAutomation		,{PC_W_AUTOCFG_FIX,		PC_R_AUTOCFG_FIX,		TX_PC_AUTOCFG_FIX}},		//自动化参数定值
	{ParamCommunication		,{PC_W_COMM_FIX,		PC_R_COMM,				TX_PC_COMM_FIX}},			//通信定值
	{ParamDevice			,{PC_W_INT_FIX,			PC_R_INT_FIX,			TX_PC_INT_FIX}},			//内部定值
	{ParamAdjust			,{PC_W_ADJ_LCD_FIX,		PC_R_ADJ_LCD_FIX,		TX_PC_ADJ_LCD_FIX}},		//LCD整定定值
};

uint16_t DBManager::Get_R_CMD_By_ParamID(ParamID pid)
{
	auto ita = Param_RW_CMD_Map.find(pid);
	if (ita != Param_RW_CMD_Map.end())
		return std::get<CMD_R>((*ita).second);
	else
		return ERROR_RET;
}

uint16_t DBManager::Get_R_CMD_By_ParamID(uint16_t pid)
{
	auto ita = Param_RW_CMD_Map.find(static_cast<ParamID>(pid));
	if (ita != Param_RW_CMD_Map.end())
		return std::get<CMD_R>((*ita).second);
	else
		return ERROR_RET;
}

uint16_t DBManager::Get_W_CMD_By_ParamID(ParamID pid)
{
	auto ita = Param_RW_CMD_Map.find(pid);
	if (ita != Param_RW_CMD_Map.end())
		return std::get<CMD_W>((*ita).second);
	else
		return ERROR_RET;
}

uint16_t DBManager::Get_W_CMD_By_ParamID(uint16_t pid)
{
	auto ita = Param_RW_CMD_Map.find(static_cast<ParamID>(pid));
	if (ita != Param_RW_CMD_Map.end())
		return std::get<CMD_W>((*ita).second);
	else
		return ERROR_RET;
}

uint16_t DBManager::Get_ParamID_By_CMD(uint16_t cmd)
{
	for (const auto &item : Param_RW_CMD_Map)
	{
		if (std::get<0>(item.second) == cmd)
			return static_cast<uint16_t>(item.first);
		if (std::get<1>(item.second) == cmd)
			return static_cast<uint16_t>(item.first);
		if (std::get<2>(item.second) == cmd)
			return static_cast<uint16_t>(item.first);
	}
	return ERROR_RET;
}

static std::map<uint16_t, uint16_t> Report_RW_CMD_Map = {
	{ReportPro,PC_R_PRO_ACT_INFO},	// 保护动作报告
	{ReportTransRcd,PC_R_LO_FAUL},	// 保护录波档案
	{ReportWorkRcd,PC_R_ZTLB_DATA},	// 业务录波档案
	{ReportSOE,PC_R_SOE_INFO},		// SOE报告
	{ReportWAR,PC_R_ALARM_INFO},	// 告警报告
	{ReportOPT,PC_R_OPER_INFO},		// 操作报告
};

uint16_t DBManager::Get_ReportID_By_CMD(uint16_t cmd)
{
	for (auto& item : Report_RW_CMD_Map)
	{
		if (item.second == cmd)
			return item.first;
	}
	return 0;
}

const dtuParamIndexTable& DBManager::GetParamInfo()
{
	return UnchangeInfoMgr::instance().GetParamInfo();
}

const dtuOneParamIndex& DBManager::GetParamInfoByID(uint16_t paramid)
{
	return UnchangeInfoMgr::instance().GetParamInfoByID(paramid);
}

const dtuOneParamInfo& DBManager::GetOneParamInfoByFix(uint16_t fixid)
{
	return UnchangeInfoMgr::instance().GetOneParamInfoByFix(fixid);
}

uint16_t DBManager::GetParamFixidByDesc(std::string desc)
{
	uint16_t fixid = 0;
	DBOperate::instance().get_fixid_by_desc(desc, "TableParamInfomation", fixid, " AND PARAMID!=14");
	return fixid;
}

dtuOneParamInfo DBManager::GetOneParamItemByFixid(uint16_t fixid)
{
	auto& ret = UnchangeInfoMgr::instance().GetParamInfo();
	for (auto& item : ret)
	{
		for (auto& item1 : item.second.info)
		{
			if (fixid == item1.second.fixid)
				return item1.second;
		}
	}
	static dtuOneParamInfo errorret;
	return errorret;
}

const dtuRmctrlTable& DBManager::GetRmctrlTable()
{
	return UnchangeInfoMgr::instance().GetRmctrlTable();
}

const dtuInfoTable& DBManager::GetInfomationTable()
{
	return UnchangeInfoMgr::instance().GetInfomationTable();
}

const dtuInfoIndex& DBManager::GetInfomationTableByIndex(uint16_t infoID)
{
	return UnchangeInfoMgr::instance().GetInfomationTableByID(infoID);
}

uint16_t DBManager::GetInfomFixidByDesc(std::string desc)
{
	uint16_t fixid = 0;
	DBOperate::instance().get_fixid_by_desc(desc, "TableInfoInfomation", fixid, "");
	return fixid;
}

dtuInfoOneItem DBManager::GetOneInfoItemByFixid(uint16_t fixid)
{
	auto &ret = UnchangeInfoMgr::instance().GetInfomationTable();
	for (auto& item : ret)
	{
		for (auto& item1 : item.second.info)
		{
			if (item1.second.fixid == fixid)
				return item1.second;
		}
	}
	static dtuInfoOneItem errorret;
	return errorret;
}

dtuParamCheckIndex DBManager::GetParamCheckTable()
{
	dtuParamCheckIndex crcindex;
	DBOperate::instance().read_crc_index(crcindex);
	return crcindex;
}

std::string DBManager::GetParamAttributeCheckBuff()
{
	const static std::vector<ParamID> sendlist = {
		ParamPublic,ParamSoftPress,ParamGroupNo,ParamRoutine,
		ParamAutoReclose,ParamAutoLocal,ParamDistributFA,
		ParamSynchronousClose,ParamAutoSplit,ParamSmallCurrent,
		ParamDisconnWarn,ParamDriveSwitch,ParamAutomation,
		ParamCommunication,ParamDevice,ParamAdjust,
	};
	try
	{
		std::string ret;
		int ParamCount = 0;
		for (auto item : sendlist)
		{
			dtuParam Param(item);
			ret = ret + Param.GetParamAttributeBuff();
			ParamCount = ParamCount + Param.count();
		}
		ret = std::to_string(ParamCount) + ret;
		return ret;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "GetParamAttributeBuff()发生错误:%s", e.what());
		return std::string("");
	}
}

std::string DBManager::GetParamCheckBuff()
{
	const static std::vector<ParamID> sendlist = {
		ParamPublic,ParamSoftPress,ParamGroupNo,ParamRoutine,
		ParamAutoReclose,ParamAutoLocal,ParamDistributFA,
		ParamSynchronousClose,ParamAutoSplit,ParamSmallCurrent,
		ParamDisconnWarn,ParamDriveSwitch,ParamAutomation,
		ParamCommunication,ParamDevice,ParamAdjust,
	};
	try
	{
		std::string ret;
		int ParamCount = 0;
		for (auto item : sendlist)
		{
			dtuParam Param(item);
			DBManager::instance().readParamValue(Param);
			std::vector<std::string> vstr;
			Param.GetCurParamValue(vstr);
			for (auto& item : vstr)
			{
				ret = ret + item;
			}
			ParamCount = ParamCount + Param.count();
		}
		ret = std::to_string(ParamCount) + ret;
		return ret;
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "GetParamCheckBuff()发生错误:%s", e.what());
		return std::string("");
	}
}

const dtuSOEIndex& DBManager::GetSOEIndex()
{
	return UnchangeInfoMgr::instance().GetSOEIndex();
}

const dtuCOSIndex& DBManager::GetCOSIndex()
{
	return UnchangeInfoMgr::instance().GetCOSIndex();
}

const dtuRmctrlIndex& DBManager::GetRMCIndex()
{
	return UnchangeInfoMgr::instance().GetRMCIndex();
}

static std::map<MapFixno, std::string> tmap = {
	{MAP_YX,"TableSOEIndex"},			// 遥信
	{MAP_YC,"TableCOSIndex"},		    // 遥测
	{MAP_YK,"TableRMCIndex"},			// 遥控
	{MAP_YT,"TableParamInfomation"},	// 遥调
	{MAP_AU,"TableParamInfomation"},	// 自动化参数
	};

bool isFixidWithinTheScope(MapFixno type, uint16_t outfixid)
{
	bool result = false;
	auto &ret = UnchangeInfoMgr::instance().GetMapTable();
	switch (type)
	{
		case MAP_YX: {	// 遥信
			if (outfixid >= ret[0].begin && outfixid <= ret[0].end)
				result = true;
			break;
		}
		case MAP_YC: {	// 遥测
			if (outfixid >= ret[1].begin && outfixid <= ret[1].end)
				result = true;
			break;
		}	
		case MAP_YK: {	// 遥控
			if (outfixid >= ret[3].begin && outfixid <= ret[3].end)
				result = true;
			break;
		}	
		case MAP_YT: {	// 遥调
			if (outfixid >= ret[2].begin && outfixid <= ret[2].end)
				result = true;
			break;
		}
		case MAP_AU: {	// 自动化参数
			if (outfixid >= ret[4].begin && outfixid <= ret[4].end)
				result = true;
			break;
		}
	}
	return result;
}

bool DBManager::CheckFixnoReady(MapFixno maptype, uint16_t outfixno)
{
	bool result = false;
	auto ret = tmap.find(maptype);
	if (ret != tmap.end()) {
		// 先检查是否在范围内
		if(isFixidWithinTheScope(maptype, outfixno))
			result = DBOperate::instance().check_addr(ret->second,
								this->FixidMapOuttoin(maptype, outfixno));
	}
	return result;
}

bool DBManager::ModifyFixno(MapFixno maptype, uint16_t older, uint16_t newer)
{
	auto ret = tmap.find(maptype);
	if (ret != tmap.end()) {

		return DBOperate::instance().update_addr(ret->second,
								this->FixidMapOuttoin(maptype, older),
								this->FixidMapOuttoin(maptype, newer));
	}
	else {
		return false;
	}
}

dtuYXIndex DBManager::GetYXIndex(YX_TYPE type)
{
	dtuYXIndex attr;
	auto &ret = UnchangeInfoMgr::instance().GetSOEIndex();
	switch (type)
	{
	// 硬件遥信4字节,前32位
	case YX_HYX: {
		for (auto& item : ret)
		{
			if (item.second.innerno <= 32) {
				dtuOneYXItem one;
				one.fixno = item.second.fixno;
				one.offset = item.second.innerno - 1;
				one.desc = item.second.desc;
				one.use = item.second.use;
				one.devno = item.second.innerno;
				attr[item.second.fixno] = one;
			}
			else {
				break;
			}
		}
	}; break;
	// 软件遥信32字节,256位(注意当软遥信扩充时需要修改这里)
	case YX_SYX: {
		for (auto& item : ret)
		{
			if ((item.second.innerno > 0x30) && (item.second.innerno <= (0x30 + 256))) {
				dtuOneYXItem one;
				one.fixno = item.second.fixno;
				one.offset = item.second.innerno - 0x30 - 1;
				one.desc = item.second.desc;
				one.use = item.second.use;
				one.devno = item.second.innerno;
				attr[item.second.fixno] = one;
			}
			else if(item.second.innerno > (0x30 + 256)) {
				break;
			}
		}
	}; break;
	case YX_ALL: {
		for (auto& item : ret)
		{
			dtuOneYXItem one;
			one.fixno = item.second.fixno;
			one.offset = item.second.innerno - 1;	// 第一个字节放的是0x0001
			one.desc = item.second.desc;
			one.use = item.second.use;
			one.devno = item.second.innerno;
			attr[item.second.fixno] = one;
		}
	}; break;
	default:
		DTULOG(DTU_WARN,"错误的遥信标识");
	}
	return attr;
}

uint16_t DBManager::GetSOEMapFixidByinID(uint16_t infixid)
{
	dtuSOEIndex ret = UnchangeInfoMgr::instance().GetSOEIndex();
	auto ita = ret.find(infixid);
	if (ita != ret.end())
	{
		// 基础地址 + 设备偏移地址 + 设备间隔地址 + 值地址
		return StaticMAPtable[0].begin + this->devno * (mapRMSystem[MAP_YX] + StaticMAPtable[0].offset) + ita->second.fixno;
	}
	else
	{
		DTULOG(DTU_WARN,"GetSOEMapFixidByinID() 错误的内部点表号[0x%04X]", infixid);
		// 错误返回0
		return 0;
	}
}

uint16_t DBManager::GetCOSMapFixidByinID(uint16_t infixid)
{
	dtuCOSIndex ret = UnchangeInfoMgr::instance().GetCOSIndex();
	auto ita = ret.find(infixid);
	if (ita != ret.end())
	{
		// 基础地址 + 设备偏移地址 + 设备间隔地址 + 值地址
		return StaticMAPtable[0].begin + this->devno * (mapRMSystem[MAP_YC] + StaticMAPtable[0].offset) + ita->second.fixno;
	}
	else
	{
		DTULOG(DTU_WARN, "GetCOSMapFixidByinID() 错误的内部点表号[0x%04X]", infixid);
		return 0;
	}
}

uint16_t DBManager::GetRMCMapFixidByoutID(uint16_t outfixid, int opewhat, bool &exec)
{
	uint16_t result = 0;
	dtuRmctrlIndex ret = UnchangeInfoMgr::instance().GetRMCIndex();

	outfixid = this->FixidMapOuttoin(MAP_YK, outfixid);
	
	auto ita = ret.find(outfixid);
	if (ita != ret.end())
	{
		if (ita->second.inoperate1 == ita->second.inoperate2)
			exec = false;
		else
			exec = true;

		if (opewhat == 1)
			result = ita->second.inoperate1;
		else if(opewhat == 2)
			result = ita->second.inoperate2;
		else
			result = ita->second.inoperate1;
	}
	else
	{
		result = 0;
	}
	return result;
}

uint16_t DBManager::GetRMCMapFixidByinID(uint16_t infixid)
{
	uint16_t result = 0;
	dtuRmctrlIndex ret = UnchangeInfoMgr::instance().GetRMCIndex();
	for (auto& item : ret)
	{
		if (item.second.inoperate1 == infixid || item.second.inoperate2 == infixid) {
			result = this->FixidMapIntoout(MAP_YK, item.second.addr);
		}
	}
	return result;
}

uint16_t DBManager::GetParamMapFixidByoutID(uint16_t outfixid)
{
	if (this->paramsize == 0)
		this->paramsize = this->CalculateRuleParamSize();
	return outfixid - StaticMAPtable[2].begin - this->devno * (mapRMSystem[MAP_YT] + StaticMAPtable[2].offset);
}

uint16_t DBManager::GetParamMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo)
{
	if (this->paramsize == 0)
		this->paramsize = this->CalculateRuleParamSize();

	if (isfrombay) {
		return StaticMAPtable[2].begin + baydennoo * (mapRMSystem[MAP_YT] + StaticMAPtable[2].offset) + infixid;
	}

	return StaticMAPtable[2].begin + this->devno * (mapRMSystem[MAP_YT] + StaticMAPtable[2].offset) + infixid;
}

uint16_t DBManager::GetYCMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo)
{
	uint16_t result = 0;
	bool isfind = false;
	uint16_t temp = 0;
	auto& rmtable = DBManager::instance().GetCOSIndex();
	for (const auto &item : rmtable)
	{
		if (item.second.innerno == infixid) {
			temp = item.second.fixno;
			isfind = true;
			break;
		}
	}

	if (isfrombay) {
		if (isfind)
			result = StaticMAPtable[1].begin + baydennoo * (mapRMSystem[MAP_YC] + StaticMAPtable[1].offset) + temp;
		return result;
	}

	if (isfind)
		result = StaticMAPtable[1].begin + this->devno * (mapRMSystem[MAP_YC] + StaticMAPtable[1].offset) + temp;

	return result;
}

uint16_t DBManager::GetYCMapFixidByoutID(uint16_t outfixid)
{
	auto& rmtable = DBManager::instance().GetCOSIndex();
	return outfixid - StaticMAPtable[1].begin - this->devno * (mapRMSystem[MAP_YC] + StaticMAPtable[1].offset);
}

uint16_t DBManager::GetAutoMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo)
{
	auto& autotable = GetParamInfoByID(ParamAutomation);

	if (isfrombay) {
		return StaticMAPtable[2].begin + baydennoo * (mapRMSystem[MAP_AU] + StaticMAPtable[2].offset) + infixid;
	}

	return StaticMAPtable[2].begin + this->devno * (mapRMSystem[MAP_AU] + StaticMAPtable[2].offset) + infixid;
}

uint16_t DBManager::GetAutoMapFixidByoutID(uint16_t outfixid)
{
	auto& autotable = GetParamInfoByID(ParamAutomation);
	return outfixid - StaticMAPtable[2].begin - this->devno * (mapRMSystem[MAP_AU] + StaticMAPtable[2].offset);
}

uint16_t DBManager::GetYXMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo)
{
	uint16_t result = 0;
	
	auto &rmtable = DBManager::instance().GetSOEIndex();

	auto ita = rmtable.find(infixid);

	if (isfrombay) {
		if (ita != rmtable.end())
			result = StaticMAPtable[0].begin + baydennoo * (mapRMSystem[MAP_YX] + StaticMAPtable[0].offset) + ita->second.fixno;
		
		return result;
	}

	if (ita != rmtable.end())
		result = StaticMAPtable[0].begin + this->devno * (mapRMSystem[MAP_YX] + StaticMAPtable[0].offset) + ita->second.fixno;

	return result;
}

uint16_t DBManager::GetYXMapFixidByoutID(uint16_t outfixid)
{
	auto& rmtable = DBManager::instance().GetSOEIndex();
	return outfixid - StaticMAPtable[0].begin - this->devno * (mapRMSystem[MAP_YX] + StaticMAPtable[0].offset);
}

uint16_t DBManager::GetYKMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo)
{
	auto &ret = UnchangeInfoMgr::instance().GetRMCIndex();

	if (isfrombay) {
		return StaticMAPtable[3].begin + baydennoo * (mapRMSystem[MAP_YK] + StaticMAPtable[3].offset) + infixid;
	}

	return StaticMAPtable[3].begin + this->devno * (mapRMSystem[MAP_YK] + StaticMAPtable[3].offset) + infixid;
}

uint16_t DBManager::GetYKMapFixidByoutID(uint16_t outfixid)
{
	auto& ret = UnchangeInfoMgr::instance().GetRMCIndex();
	return outfixid - StaticMAPtable[3].begin - this->devno * (mapRMSystem[MAP_YK] + StaticMAPtable[3].offset);
}

uint16_t DBManager::FixidMapOuttoin(MapFixno type, uint16_t outfixid)
{
	uint16_t result = 0;
	switch (type) 
	{
		case MAP_YX:{
			result = this->GetYXMapFixidByoutID(outfixid);
			break;
		}
		case MAP_YC:{
			result = this->GetYCMapFixidByoutID(outfixid);
			break;
		}
		case MAP_YK:{
			result = this->GetYKMapFixidByoutID(outfixid);
			break;
		}
		case MAP_YT:{
			result = this->GetParamMapFixidByoutID(outfixid);
			break;
		}
		case MAP_AU:{
			result = this->GetAutoMapFixidByoutID(outfixid);
			break;
		}
	}
	return result;
}

uint16_t DBManager::FixidMapOuttoin(uint16_t outfixid)
{
	uint16_t result = 0;

	if (outfixid >= StaticMAPtable[0].begin && outfixid <= StaticMAPtable[0].end)
		result = FixidMapOuttoin(MAP_YX, outfixid);
	else if(outfixid >= StaticMAPtable[1].begin && outfixid <= StaticMAPtable[1].end)
		result = FixidMapOuttoin(MAP_YC, outfixid);
	else if(outfixid >= StaticMAPtable[2].begin && outfixid <= StaticMAPtable[2].end)
		result = FixidMapOuttoin(MAP_YT, outfixid);
	else if(outfixid >= StaticMAPtable[3].begin && outfixid <= StaticMAPtable[3].end)
		result = FixidMapOuttoin(MAP_YK, outfixid);
	else if(outfixid >= StaticMAPtable[4].begin && outfixid <= StaticMAPtable[4].end)
		result = FixidMapOuttoin(MAP_AU, outfixid);
	
	return result;
}

uint16_t DBManager::FixidMapIntoout(MapFixno type, uint16_t infixid, bool isfrombay, int baydennoo)
{
	uint16_t result = 0;
	switch (type)
	{
	case MAP_YX: {
		result = this->GetYXMapFixidByinID(infixid, isfrombay, baydennoo);
		break;
	}
	case MAP_YC: {
		result = this->GetYCMapFixidByinID(infixid, isfrombay, baydennoo);
		break;
	}
	case MAP_YK: {
		result = this->GetYKMapFixidByinID(infixid, isfrombay, baydennoo);
		break;
	}
	case MAP_YT: {
		result = this->GetParamMapFixidByinID(infixid, isfrombay, baydennoo);
		break;
	}
	case MAP_AU: {
		result = this->GetAutoMapFixidByinID(infixid, isfrombay, baydennoo);
		break;
	}
	}
	return result;
}

int DBManager::CalculateRuleParamSize()
{
	auto& yttable = GetParamInfo();
	int size = 0;
	for (auto& item : yttable)
	{
		if (item.second.pid == ParamAutomation || item.second.pid == ParamDevice ||
			item.second.pid == ParamCommunication || item.second.pid == ParamAdjust)
			continue;
		size = size + item.second.info.size();
	}
	this->paramsize = size;
	return size;
}

void DBManager::SetDevNo(int devno)
{
	this->isSetDevno = true;
	this->devno = devno;
}

bool DBManager::isDevNoSet()
{
	return this->isSetDevno;
}

enum TABLEFROM {
	FROM_MAP_YX = 0x01,	/* 遥信 */
	FROM_MAP_YC = 0x02,	/* 遥测 */
	FROM_MAP_YT = 0x03,	/* 定值 */
	FROM_MAP_YK = 0x04,	/* 遥控 */
	FROM_MAP_AU = 0x05,	/* 自动化参数 */
};

DBManager::FIXIDINFO DBManager::whereFixFrom(uint16_t outfixid)
{
	FIXIDINFO result;
	int size = 0;
	auto &ret = UnchangeInfoMgr::instance().GetMapTable();
	for (auto& item : ret)
	{
		if (outfixid >= item.begin && outfixid <= item.end)
		{
			result.tableno = item.tableno;
			switch (item.tableno)
			{
			case FROM_MAP_YX: {
				size = mapRMSystem[MAP_YX];
				break;
			}
			case FROM_MAP_YC: {
				size = mapRMSystem[MAP_YC];
				break;
			}
			case FROM_MAP_YT: {
				if (this->paramsize == 0)
					this->CalculateRuleParamSize();
				size = mapRMSystem[MAP_YT];
				break;
			}
			case FROM_MAP_YK: {
				size = mapRMSystem[MAP_YK];
				break;
			}
			case FROM_MAP_AU: {
				size = mapRMSystem[MAP_AU];
				break;
			}
			}
			result.devno = (outfixid - item.begin) / (size + item.offset);
			result.ok = true;
			break;
		}
	}
	return result;
}

bool DBManager::testFixidBelongCurDevice(uint16_t outfixid)
{
	bool result = false;
	int size = 0;
	int Pbegin = 0;
	auto& ret = UnchangeInfoMgr::instance().GetMapTable();
	for (auto& item : ret)
	{
		if (outfixid >= item.begin && outfixid <= item.end)
		{
			Pbegin = item.begin;

			switch (item.tableno)
			{
			case FROM_MAP_YX: {
				size = mapRMSystem[MAP_YX];
				break;
			}
			case FROM_MAP_YC: {
				size = mapRMSystem[MAP_YC];
				break;
			}
			case FROM_MAP_YT: {
				if (this->paramsize == 0)
					this->CalculateRuleParamSize();
				size = mapRMSystem[MAP_YT];
				break;
			}
			case FROM_MAP_YK: {
				size = mapRMSystem[MAP_YK];
				break;
			}
			case FROM_MAP_AU: {
				size = mapRMSystem[MAP_AU];
				break;
			}
			}

			break;
		}
	}
	return ((Pbegin <= outfixid) && (outfixid < Pbegin + (this->devno + 1) * size));
}

bool DBManager::isPasswordCorrect(const std::string& name, const std::string& password, UserError& flag)
{
	//TODO AES-加密解密
	bool result = false;
	flag = NO_USER;
	auto &ret = UnchangeInfoMgr::instance().GetUserTable();
	for (const auto &item : ret)
	{
		if (item.name == name) {
			flag = PW_UNCORRECT;
			if (item.password == password) {
				flag = PW_CORRECT;
				result = true;
				break;
			}
		}
	}
	return result;
}

UPOWER DBManager::GetUserPower(const std::string &name)
{
	UPOWER result = VISITOR;
	auto& ret = UnchangeInfoMgr::instance().GetUserTable();
	for (const auto& item : ret)
	{
		if (item.name == name) {
			result = item.power;
			break;
		}
	}
	return result;
}

std::string DBManager::GetRootKey()
{
	std::string result;
	auto& ret = UnchangeInfoMgr::instance().GetUserTable();
	for (const auto& item : ret)
	{
		if (item.name == "root") {
			result = item.word;
			break;
		}
	}
	return result;
}

bool DBManager::isUserExist(const std::string& name)
{
	bool result = false;
	auto& ret = UnchangeInfoMgr::instance().GetUserTable();
	for (const auto& item : ret)
	{
		if (item.name == name) {
			result = true;
			break;
		}
	}

	return result;
}

bool DBManager::isDevIDneedToMaster(MapFixno type, uint16_t devid)
{
	bool result = false;
	switch (type)
	{
	case MAP_YX: {
		auto &ret = UnchangeInfoMgr::instance().GetSOEIndex();
		auto ita = ret.find(devid);
		if (ita != ret.end())
			result = ita->second.use;
		break;
	}
	case MAP_YC: {
		auto& ret = UnchangeInfoMgr::instance().GetCOSIndex();
		auto ita = ret.find(devid);
		if (ita != ret.end())
			result = ita->second.use;
		break;
	}
	}
	return result;
}

void DBManager::TESTAPI()
{

}