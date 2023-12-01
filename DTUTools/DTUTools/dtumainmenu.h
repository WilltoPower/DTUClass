#pragma once

#include <QWidget>
#include "ui_dtumainmenu.h"


#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QStackedLayout>

#include "dtudbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtumainmenuClass; };
QT_END_NAMESPACE

using namespace DTU;

class dtumainmenu : public QWidget
{
	Q_OBJECT

public:
	dtumainmenu(QWidget *parent = nullptr);
	~dtumainmenu();

private:
	Ui::dtumainmenuClass *ui;

private:
	void load_ui();
private:
	QStackedLayout *widgetlayout = nullptr;
private:
	int CalculateAbsoluteIndex(int index, int row);
private slots:
	void treeWidgetDoubleClick(QModelIndex index);
////////////////////////////信息查看部分
public:
	void load_info_index_ui();
private:
	QTreeWidgetItem* GroupInfo = nullptr;
	DTU::dtuInfoTable infoindex;
private:
	void load_one_info_ui(uint16_t infoID);
	
	////////////////////////////定值部分
public:
	void load_param_index_ui();
	void load_one_param_ui(uint16_t paramid);
private:
	DTU::dtuParamIndexTable Paramindex;
	QTreeWidgetItem* GroupParam = nullptr;
	////////////////////////////报告部分
private:
	void load_report_index_ui();
	void load_one_report_ui(uint16_t reportid);
private:
	uint16_t reportID = 0;
	DTU::dtuReportIndexTable Reportindex;
	QTreeWidgetItem* GroupReport = nullptr;
	////////////////////////////规约历史部分
public:
	void load_rule_ui();
	QTreeWidgetItem* GroupRule = nullptr;
	////////////////////////////遥控部分
public:
	void load_rm_ui();
private:
	QTreeWidgetItem* GroupRmctrl = nullptr;

	////////////////////////////软遥信部分
public:
	void load_modifyfixid_ui();
private:
	QTreeWidgetItem* Groupsyx = nullptr;
};
