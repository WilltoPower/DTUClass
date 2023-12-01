#pragma execution_character_set("utf-8")

#include "dtureportwidget.h"
#include "dtutask.h"
#include "dtulog.h"

#include <QMessageBox>
#include <fstream>
#include <filesystem>
#include <atlconv.h>
#include <QProcess>

#include "dtuconfigure.h"
#include "dtustorage.h"

using namespace DTU;

dtureport::dtureport(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
}

dtureport::~dtureport() {}

void dtureport::load_ui()
{
	if (reportID == 0xFF)
		return;

	QStringList header = load_header();

	// 设置列数
	ui.tableWidget->setColumnCount(header.size());
	// 设置表头
	ui.tableWidget->setHorizontalHeaderLabels(header);

	// ui修改
	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);// 自动延展
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置为不可修改
	//ptable->horizontalHeader()->setVisible(false);// 隐藏水平表头
	ui.tableWidget->verticalHeader()->setVisible(false);    // 隐藏垂直表头
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//先自适应宽度
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//然后设置要根据内容使用宽度的列
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// 设置整行选中
	ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	// 单次只能选中一个
	ui.tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

QStringList dtureport::load_header()
{
	auto ret = DBManager::instance().GetReportInfoByID(reportID);
	QStringList header;

	header << "序号";

	switch (reportID)
	{
	case ReportPro: {
		header << "时间" << "动作文件";
	}; break;
	case ReportOPT: {
		header << "时间"  << "操作" << "操作源";
	}; break;
	case ReportWAR: {
		header << "时间" << "告警信息" << "状态变化";
	}; break;
	case ReportSOE: {
		header << "时间" << "SOE类型" << "SOE信息" << "状态变化";
	}; break;
	default: {
		for (auto &item : ret.rinfo)
		{
			header << QString::fromStdString(item.second.desc);
		}
	}; break;
	}
	return header;
}

void dtureport::load_connect()
{
	switch (reportID)
	{
		// 保护动作报告
	case ReportPro:
		// 保护录波档案
	case ReportTransRcd:
		// 业务录波档案
	case ReportWorkRcd: {
		connect(ui.tableWidget,SIGNAL(cellDoubleClicked(int, int)),this,SLOT(LoadReportFile(int,int)));
	}; break;
	default:
		break;
	}
}

void addOneItemToTableWidget(QTableWidget *table,int index,int no,QString desc)
{
	table->setItem(index, no, new QTableWidgetItem(desc));
	table->item(index, no)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void dtureport::ReportParseSOE(QTableWidget *widget, DTU::buffer &data, int index, int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	// 
	uint16_t type = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t fix = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	uint16_t statuschange = data.get(14, sizeof(uint16_t)).value<uint16_t>();
	QString state = QString("%1->%2").arg(LOBYTE(statuschange)).arg(HIBYTE(statuschange));
	std::string soename;

	std::string SOEType;

	if (type == 0x0001) {
		// 硬遥信
		SOEType = "硬件遥信(1)";
	}
	else if (type == 0x0002) {
		// 软遥信
		SOEType = "软件遥信(2)";
	}

	if (fix == 0) {
		soename = "SOE值为0 TYPE=" + std::to_string(type);
	}
	else {
		auto ret = DTU::DBManager::instance().GetSOEIndex();
		auto ita = ret.find(fix);
		if (ita != ret.end()) {
			soename = ita->second.desc;
		}
		else {
			soename = "未知SOE=" + std::to_string(fix) + " TYPE=" + std::to_string(type);
		}
	}

	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, QString::fromStdString(SOEType));
	addOneItemToTableWidget(widget, index, 3, QString::fromStdString(soename));
	addOneItemToTableWidget(widget, index, 4, state);
}

void dtureport::ReportParseOPT(QTableWidget *widget, DTU::buffer &data, int index, int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	//
	uint16_t optsrc = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t optobj = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	
	static std::map<uint16_t, QString> srcmap = {
		{1, "本地"},{2, "远方"}
	};
	static std::map<uint16_t, QString> objmap = {
		{1, "遥控分闸"},{2, "遥控合闸"},
		{3, "活化启动"},{4, "活化退出"},
		{5, "信号复归"},{6, "公共定值"},
		{7, "保护定值"},{8, "保护控制字"},
		{9, "就地FA定值"},{10, "就地FA控制字"}
	};
	QString strsrc;
	auto srcita = srcmap.find(optsrc);
	if (srcita == srcmap.end()) {
		strsrc = "未知操作源";
	}
	else {
		strsrc = srcita->second;
	}
	QString strobj;
	auto objita = objmap.find(optobj);
	if (objita == objmap.end()) {
		strobj = "未知操作";
	}
	else {
		strobj = objita->second;
	}
	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, strobj);
	addOneItemToTableWidget(widget, index, 3, strsrc);
}

void dtureport::ReportParseWar(QTableWidget *widget, DTU::buffer &data, int index, int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	uint16_t warnno = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t statuschange = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	QString state = QString("%1->%2").arg(LOBYTE(statuschange)).arg(HIBYTE(statuschange));

	QString content;

	switch (warnno)
	{
	case 1:content = "PT断线"; break;
	case 2:content = "控制回路断线"; break;
	case 3:content = "线路断线"; break;
	case 4:content = "保护功能投入冲突告警"; break;
	default:
		content = "未知告警";
		break;
	}
	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, content);
	addOneItemToTableWidget(widget, index, 3, state);
}

void dtureport::ReportParseRMC(QTableWidget *widget, DTU::buffer &data, int index, int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	std::string strtime = timestamp_to_date(seconds, micosec);
	uint16_t fixid = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	uint16_t opera = data.get(14, sizeof(uint16_t)).value<uint16_t>();
	uint16_t co = data.get(16, sizeof(uint16_t)).value<uint16_t>();
	QString operateStr;
	switch (opera)
	{
	case RC_CMD_PRE: operateStr = "预置"; break;
	case RC_CMD_EXE: operateStr = "执行"; break;
	case RC_CMD_CAN: operateStr = "取消"; break;
	default:
		operateStr = "未知操作(" + QString::number(opera) + ")";
	}
	QString coStr;
	switch (co)
	{
	case 0: coStr = "成功"; break;
	case 1: coStr = "失败"; break;
	default:
		coStr = "未知(" + QString::number(co) + ")";
	}

	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, "0x" + QString("%1").arg(fixid, 2, 16, QLatin1Char('0')).toUpper());
	addOneItemToTableWidget(widget, index, 3, operateStr);
	addOneItemToTableWidget(widget, index, 4, coStr);
}

void dtureport::ReportParsePro(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms)
{
	uint64_t time = (uint64_t)s * 1000000 + (uint64_t)ms * 1000;
	std::string strtime = create_time_from_mirco(time);
	std::string fileName(data.const_data(),data.size());

	if (fileName.empty()){
		return;
	}

	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, QString::fromStdString(fileName));
}

void dtureport::ReportParseTranRcd(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms)
{
	uint64_t totlemircoseconds = (uint64_t)s * 1000000 + (uint64_t)ms * 1000;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	std::string fileName(data.const_data(), data.size());
	if (fileName.empty()) {
		return;
	}
	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, QString::fromStdString(fileName));
}

void dtureport::ReportParseWorkRcd(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms)
{
	uint64_t totlemircoseconds = (uint64_t)s * 1000000 + (uint64_t)ms * 1000;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	std::string fileName(data.const_data(),data.size());
	if (fileName.empty()) {
		return;
	}
	addOneItemToTableWidget(widget, index, 0, QString("%1").arg(no));
	addOneItemToTableWidget(widget, index, 1, QString::fromStdString(strtime));
	addOneItemToTableWidget(widget, index, 2, QString::fromStdString(fileName));
}

void dtureport::LoadReportFile(int row, int clcolumn)
{
	// 获取文件名
	QString fullPath;
	QString fileName = ui.tableWidget->item(row, 2)->text();
	bool ret = false;
	switch (reportID)
	{
	// 保护动作报告
	case ReportPro: {
		static std::vector<std::string> suffix = { "" };
		ret = get_file_from_arm("/protect/protect/", "\\protect\\protect\\", fileName, suffix, fullPath);
		if (ret) {
			DTU::buffer protact;
			// 打开文件
			QFile ft(fullPath.toStdString().c_str());
			if (ft.open(QIODevice::ReadOnly)) {
				auto filedata = ft.readAll();
				protact.append(filedata.data(), filedata.size());
				ft.close();
			}
			else {
				DTULOG(DTU_ERROR, (char*)"无法打开文件:%s", fileName.toStdString().c_str());
			}
			if (protact.size() > 0) {
				_protdlg.parse(protact);
				_protdlg.exec();
			}
		}
		else {
			DTULOG(DTU_ERROR, "无法获取保护录波文件%s", fileName.toStdString().c_str());
		}
	}; break;
	// 保护录波档案
	case ReportTransRcd: {
		static std::vector<std::string> suffix = { ".cfg",".dat" };
		ret = get_file_from_arm("/protect/comtrade/", "\\protect\\comtrade\\", fileName, suffix, fullPath);
		// 获取原始数据文件
		get_file_from_arm("/protect/factory/", "\\protect\\factory\\", fileName);
		if (ret) {
			open_comtrade_file(fullPath);
		}
		else {
			DTULOG(DTU_ERROR, "无法获取保护录波文件%s", fileName.toStdString().c_str());
		}
	}; break;
	// 业务录波档案
	case ReportWorkRcd: {
		static std::vector<std::string> suffix = { ".cfg",".dat" };
		ret = get_file_from_arm("/COMTRADE/", "\\COMTRADE\\", fileName, suffix, fullPath);
		// 获取原始数据文件
		get_file_from_arm("/FACTORY/", "\\FACTORY\\", fileName);
		if (ret) {
			open_comtrade_file(fullPath);
		}
		else {
			DTULOG(DTU_ERROR,"无法获取业务录波文件%s", fileName.toStdString().c_str());
		}
	}; break;
	default:
		break;
	}

	
}

void dtureport::setCurReportID(uint16_t reportid)
{
	reportID = reportid;
	CurInfo = DBManager::instance().GetReportInfo();
	load_ui();
	load_connect();
}

void dtureport::addOneUIitem(QTableWidget *widget, int index ,int no , uint16_t reportid, ReportOneBuffer data)
{
	switch (reportid)
	{
	case ReportSOE: {
		ReportParseSOE(widget, std::get<2>(data), index, no);
	}; break;
	case ReportOPT: {
		ReportParseOPT(widget, std::get<2>(data), index, no);
	}; break;
	case ReportWAR: {
		ReportParseWar(widget, std::get<2>(data), index, no);
	}; break;
	case ReportRMC: {
		ReportParseRMC(widget, std::get<2>(data), index, no);
	}; break;
	case ReportPro: {
		ReportParsePro(widget, std::get<2>(data), index, no, std::get<0>(data), std::get<1>(data));
	}; break;
	case ReportTransRcd: {
		ReportParseTranRcd(widget, std::get<2>(data), index, no, std::get<0>(data), std::get<1>(data));
	}; break;
	case ReportWorkRcd:{
		ReportParseWorkRcd(widget, std::get<2>(data), index, no, std::get<0>(data), std::get<1>(data));
	}; break;
	default:
		break;
	}
}

void dtureport::updatereport()
{
	uint32_t reportno = 0;

	bool isOnline = execute_test_arm_connect();

	if (!isOnline) {
		// 从本地获取
		DTULOG(DTU_INFO, "后台程序未连接,从本地读取");
		reportno = DTU::DSTORE::instance().get_cur_report_no(reportID);
	}
	else {
		if (DTU_SUCCESS != execute_get_reportno(reportID, reportno)) {
			DTULOG(DTU_ERROR, "读取报告失败");
			return;
		}
	}

	ui.tableWidget->clear();
	ui.tableWidget->setHorizontalHeaderLabels(load_header());

	if (reportno == 0) {
		ui.spin_page->setValue(1);
		ui.spin_page->setMaximum(1);
		return;
	}
	// 设置页码
	int mod = reportno % 50;
	if (mod == 0) {
		ui.spin_page->setMaximum(reportno / 50);
		ui.spin_page->setValue(reportno / 50);
		mod = 50;
	}
	else {
		ui.spin_page->setMaximum(reportno / 50 + 1);
		ui.spin_page->setValue(reportno / 50 + 1);
	}

	DTU::ReportBufferAttr result;
	if (!isOnline) {
		DTU::DSTORE::instance().get_report_range(reportID, reportno - mod + 1, reportno, result);
	}
	else {
		if (DTU_SUCCESS != execute_get_report(reportID, reportno - mod + 1, reportno, result)) {
			return;
		}
	}

	ui.tableWidget->setRowCount(result.size());

	int index = 0;
	for (auto &item : result)
	{
		addOneUIitem(ui.tableWidget, index, item.first , reportID, item.second);
		index++;
	}
}

void dtureport::dtureport::clearreport()
{
	if (!execute_test_arm_connect()) {
		DTULOG(DTU_ERROR,"后台程序未连接");
		return;
	}
	execute_clear_report(reportID);
	ui.tableWidget->clearContents();
	ui.tableWidget->setRowCount(0);
	ui.spin_page->setValue(1);
	ui.spin_page->setMaximum(1);
}

bool dtureport::get_file_from_arm(std::string armPath, std::string localPath, QString fileName, std::vector<std::string> suffix, QString &FullPath)
{
	if (suffix.empty())
		return false;

	// 查找文件是否存在
	std::string file = get_exec_dir() + localPath + fileName.toStdString() + suffix[0];
	
	FullPath = QString::fromStdString(file);

	if (execute_test_arm_connect())
	{
		// ARM已经连接无论是否存在文件都刷新 因为可能产生同名文件无法进行覆盖
		for (const auto& item : suffix)
		{
			std::string fullName = fileName.toStdString() + item;
			DTU::buffer result;
			if (DTU_SUCCESS != execute_get_file(armPath + fullName, result))
			{
				QMessageBox::information(this, "错误", "无法获取文件", QMessageBox::Ok);
				return false;
			}
			// 保存文件
			fullName = get_exec_dir() + localPath + fullName;
			std::ofstream ofs;
			ofs.open(fullName, std::ios::binary | std::ios::app);
			ofs.write(result.const_data(), result.size());
			ofs.close();
		}
		return true;
	}
	else
	{
		// 未连接ARM检测文件是否存在 存在则直接打开
		if (std::filesystem::exists(file)) {
			return true;
		}
		return false;
	}
}

bool dtureport::get_file_from_arm(std::string armPath, std::string localPath, QString fileName)
{
	// 查找文件是否存在
	std::string file = get_exec_dir() + localPath + fileName.toStdString();

	if (execute_test_arm_connect())
	{
		// ARM已经连接无论是否存在文件都刷新 因为可能产生同名文件无法进行覆盖
		std::string fullName = fileName.toStdString();
		DTU::buffer result;
		if (DTU_SUCCESS != execute_get_file(armPath + fullName, result)) {
			QMessageBox::information(this, "错误", "无法获取文件", QMessageBox::Ok);
			return false;
		}
		// 保存文件
		fullName = get_exec_dir() + localPath + fullName;
		save_file(fullName, result);
		return true;
	}
	else
	{
		// 未连接ARM检测文件是否存在 存在则直接打开
		if (std::filesystem::exists(file)) {
			return true;
		}
		return false;
	}
}

void dtureport::open_comtrade_file(QString fileName)
{
#if 0
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	PROCESS_INFORMATION pi = {0};
	std::string fdranalyse = dtutoolcfg::instance().GetAnalyzeToolPath();

	std::string cmdLine = "start " + fileName.toStdString();

	//用于部分入参CString向LPCWSTR转换
	USES_CONVERSION;

	if (!CreateProcess((LPCWSTR)(fdranalyse.c_str()),
		(LPWSTR)(cmdLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		DTULOG(DTU_ERROR, "无法启动离线分析工具[0x%04X]", GetLastError());
		return;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#else

	QProcess myProcess(this);
	QString fdranalyse = QString::fromStdString(dtutoolcfg::instance().GetAnalyzeToolPath());
	QStringList arguments;

	QString cmdLine = fileName;

	arguments << "start" << cmdLine;//传递到exe的参数

	if (!myProcess.startDetached(fdranalyse, arguments)) {
		DTULOG(DTU_ERROR, "QProcess() 无法启动离线分析工具");
		return;
	}

#endif
}

std::string dtureport::timestamp_to_date(time_t tt, uint32_t ms)
{
	struct tm *t = localtime(&tt);
	char dateBuf[128];
	snprintf(dateBuf, sizeof(dateBuf), "%04d年%02d月%02d日 %02d:%02d:%02d %03dms", t->tm_year + 1900,
		t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, ms);
	std::string date(dateBuf);
	return date;
}

void dtureport::page_changed(int pageno)
{
	bool isOnline = execute_test_arm_connect();

	ui.tableWidget->clear();
	ui.tableWidget->setHorizontalHeaderLabels(load_header());

	DTU::ReportBufferAttr result;

	if (!isOnline) {
		DTU::DSTORE::instance().get_report_range(reportID, 50 * (pageno - 1) + 1, 50 * pageno, result);
	}
	else {
		if (DTU_SUCCESS != execute_get_report(reportID, 50 * (pageno - 1) + 1, 50 * pageno, result)) {
			return;
		}
	}



	ui.tableWidget->setRowCount(result.size());

	int index = 0;
	for (auto &item : result)
	{
		addOneUIitem(ui.tableWidget, index, item.first, reportID, item.second);
		index++;
	}
}

uint32_t dtureport::groupsize()
{
	// 动作简报不需要显示所以减一
	return CurInfo.size() - 1;
}

void dtureport::packdata()
{
	if (!execute_test_arm_connect()) {
		DTULOG(DTU_ERROR, "后台程序未连接");
		return;
	}

	uint32_t reportno = 0;
	if (DTU_SUCCESS != execute_get_reportno(reportID, reportno)) {
		return;
	}
	
	if (reportno > 0) {
		if (exportwidget != nullptr) {
			delete exportwidget;
		}
		if (reportID == ReportPro || reportID == ReportTransRcd || reportID == ReportWorkRcd) {
			exportwidget = new dtuexportwidget(dtuexportwidget::EXPREPORTFILE, QString::fromStdString(
				DBManager::instance().GetReportInfoByID(reportID).desc), reportno, reportID);
		}
		else {
			exportwidget = new dtuexportwidget(dtuexportwidget::EXPREPORT, QString::fromStdString(
				DBManager::instance().GetReportInfoByID(reportID).desc), reportno, reportID);
		}

		exportwidget->show();
	}
	else {
		QMessageBox::information(this, "警告", "报告内容为空无法导出", QMessageBox::Ok);
	}
}