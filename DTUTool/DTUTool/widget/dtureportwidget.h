#pragma once

#include <QWidget>
#include <QTableWidget>
#include "ui_dtureportwidget.h"

#include "dtubuffer.h"
#include "dtudbmanager.h"
#include "dtuprotactdlg.h"
#include "dtuexportwidget.h"

using namespace DTU;

class dtureport : public QWidget
{
	Q_OBJECT

public:
	dtureport(QWidget *parent = nullptr);
	~dtureport();

private:
	Ui::dtureportClass ui;

private:
	void load_ui();
	// 加载表头
	QStringList load_header();
	// 加载信号槽
	void load_connect();
	// 解析SOE报告并添加到控件
	void ReportParseSOE(QTableWidget *widget, DTU::buffer &data, int index, int no);
	// 解析操作报告并添加到控件
	void ReportParseOPT(QTableWidget *widget, DTU::buffer &data, int index, int no);
	// 解析告警报告并添加到控件
	void ReportParseWar(QTableWidget *widget, DTU::buffer &data, int index, int no);
	// 解析遥控记录报告
	void ReportParseRMC(QTableWidget *widget, DTU::buffer &data, int index, int no);
	// 解析动作报告文件并添加到控件
	void ReportParsePro(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms);
	// 解析保护录波文件并添加到控件
	void ReportParseTranRcd(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms);
	// 解析业务录波文件并添加到控件
	void ReportParseWorkRcd(QTableWidget *widget, DTU::buffer &data, int index, int no, uint32_t s, uint32_t ms);

public:
	void setCurReportID(uint16_t reportid);
	void addReport();

public slots:
	// 更新(会直接跳转到最新的一条)
	void updatereport();

private slots:
	// 清空
	void clearreport();
	// 加载报告文件
	void LoadReportFile(int row, int clcolumn);
	// 翻页函数
	void page_changed(int pageno);
	// 组大小
	uint32_t groupsize();
	// 导出功能
	void packdata();

private:
	// 导出窗口指针
	dtuexportwidget *exportwidget = nullptr;
	uint16_t reportID = 0xFF;
	QTableWidget *widget = nullptr;
	dtuprotactdlg _protdlg;
	// 添加一条报告
	void addOneUIitem(QTableWidget *widget, int index, int no, uint16_t reportid, ReportOneBuffer data);
	// 从ARM获取文件
	bool get_file_from_arm(std::string armPath, std::string localPath, QString fileName, std::vector<std::string> suffix, QString &FullPath);
	// 从ARM获取文件
	bool get_file_from_arm(std::string armPath, std::string localPath, QString fileName);
	// 打开comtrade文件
	void open_comtrade_file(QString fileName);
	// 遥控记录时间戳转换
	std::string timestamp_to_date(time_t tt, uint32_t ms);
	// 报告信息
	DTU::dtuReportIndexTable CurInfo;
};
