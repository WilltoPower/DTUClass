#include "dtumainmenu.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <bitset>
#include "create_control.h"

#include "dtulog.h"
#include "dtutask.h"
#include "dturpcclient.h"

#include "dtureportwidget.h"
#include "dturulefilewidget.h"
#include "dtuparamwidget.h"
#include "dtuinformationwidget.h"
#include "dturmcwidget.h"
#include "dtufixmodify.h"

using namespace DTU;

#define WIDGET_INFO 0 // 信息查看界面
#define WIDGET_PARA 1 // 定值界面
#define WIDGET_REPT 2 // 报告界面
#define WIDGET_HIS  3 // 历史记录界面
#define WIDGET_RMCT 4 // 遥控界面
#define WIDGET_SYX  5 // 软遥信信息查看


dtumainmenu::dtumainmenu(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtumainmenuClass())
{
	ui->setupUi(this);
	load_ui();
}

dtumainmenu::~dtumainmenu()
{
	delete ui;
}

void dtumainmenu::load_ui()
{
	//加载index信息
	infoindex = DBManager::instance().GetInfomationTable();
	Paramindex = DBManager::instance().GetParamInfo();
	Reportindex = DBManager::instance().GetReportInfo();
	widgetlayout = new QStackedLayout;
	//设置布局模式
	widgetlayout->setStackingMode(QStackedLayout::StackOne);
	// 设置布局器
	ui->widget->setLayout(widgetlayout);
	// TreeWidget槽函数
	connect(ui->treeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(treeWidgetDoubleClick(QModelIndex)));
	// 以下加载各部分UI(UI加载不可修改顺序)
	load_info_index_ui();	// 加载左侧info索引UI
	load_param_index_ui();	// 加载左侧Param索引UI
	load_report_index_ui();	// 加载左侧报告索引UI
	load_rule_ui();			// 加载规约文件
	load_rm_ui();			// 加载左侧遥控UI
	load_modifyfixid_ui();	// 加载点表修改查看UI
	
	// 显示硬件遥信
	dturpcclient::instance().setInformationID(InfomHardRemoteSignal);

	ui->treeWidget->expandAll();

	widgetlayout->setCurrentIndex(4);
}

void dtumainmenu::treeWidgetDoubleClick(QModelIndex index)
{
	int prow = index.parent().row();
	// 父控件索引小于0说明正在选择子索引,不做动作,直接返回
	if (prow < 0)
		return;
	switch (prow)
	{
		// 信息查看部分
	case WIDGET_INFO: {
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
		// 设置信息查看
		dturpcclient::instance().setInformationID(index.row());
	}; break;
		// 选择的是定值部分
	case WIDGET_PARA: {
		// 关闭信息查看
		dturpcclient::instance().setInformationID(0, false);
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
		((dtuparam*)(widgetlayout->currentWidget()))->read_param();
	} break;
		// 选择的是报告部分
	case WIDGET_REPT: {
		// 关闭信息查看
		dturpcclient::instance().setInformationID(0, false);
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
		((dtureport*)(widgetlayout->currentWidget()))->updatereport();
	}break;
		// 选择的是规约历史文件
	case WIDGET_HIS: {
		// 关闭信息查看
		dturpcclient::instance().setInformationID(0, false);
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
		((dturulefilewidget*)(widgetlayout->widget(CalculateAbsoluteIndex(prow, index.row()))))->load_first();
	}break;
		// 选择的是遥控
	case WIDGET_RMCT: {
		// 关闭信息查看
		dturpcclient::instance().setInformationID(0, false);
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
	}; break;
	case WIDGET_SYX: {
		// 关闭信息查看
		dturpcclient::instance().setInformationID(0, false);
		widgetlayout->setCurrentIndex(CalculateAbsoluteIndex(prow, index.row()));
	}break;
	}
}

int dtumainmenu::CalculateAbsoluteIndex(int index, int row)
{
	int ret = 0;
	switch (index)
	{
	case WIDGET_SYX: ret = ret + 1;
	case WIDGET_RMCT: ret = ret + 1;
	case WIDGET_HIS: ret = ret + Reportindex.size() - 1;// 减一是去除了动作简报的内容
	case WIDGET_REPT: ret = ret + Paramindex.size();
	case WIDGET_PARA: ret = ret + infoindex.size();
	case WIDGET_INFO: ret = ret + row;
	}
	return ret;
}

/************************************************* 信息查看 **************************************************/
void dtumainmenu::load_info_index_ui()
{
	GroupInfo = new QTreeWidgetItem(ui->treeWidget);
	GroupInfo->setText(0, "信息查看");
	for (const auto &item : infoindex)
	{
		QTreeWidgetItem *newgroup = new QTreeWidgetItem(GroupInfo);
		newgroup->setText(0, QString::fromStdString(item.second.desc));
		load_one_info_ui(item.second.iid);
	}
}

void dtumainmenu::load_one_info_ui(uint16_t infoID)
{
	dtuinformation *itable = new dtuinformation;
	itable->setCurWidgetInfoID(infoID, infoindex);
	widgetlayout->addWidget(itable);

	// 连接槽函数
	connect(&dturpcclient::instance(), SIGNAL(update_information(unsigned short, QByteArray)),
		itable, SLOT(updateinfo(unsigned short, QByteArray)), Qt::QueuedConnection);
}

/************************************************* 定值部分 **************************************************/
void dtumainmenu::load_param_index_ui()
{
	GroupParam = new QTreeWidgetItem(ui->treeWidget);
	GroupParam->setText(0, "定值");
	for (const auto &item : Paramindex)
	{
		QTreeWidgetItem *newgroup = new QTreeWidgetItem(GroupParam);
		newgroup->setText(0, QString::fromStdString(item.second.desc));
		load_one_param_ui(item.first);
	}
}

void dtumainmenu::load_one_param_ui(uint16_t paramid)
{
	dtuparam *ptable = new dtuparam;
	ptable->SetParamID(paramid);
	widgetlayout->addWidget(ptable);
}

/************************************************* 报告部分 **************************************************/
void dtumainmenu::load_report_index_ui()
{
	GroupReport = new QTreeWidgetItem(ui->treeWidget);
	GroupReport->setText(0, "报告");
	for (const auto &item : Reportindex)
	{
		if (item.first == ReportProSimple)
			continue;
		QTreeWidgetItem *newgroup = new QTreeWidgetItem(GroupReport);
		newgroup->setText(0, QString::fromStdString(item.second.desc));
		load_one_report_ui(item.second.reportid);
	}
}

void dtumainmenu::load_one_report_ui(uint16_t reportid)
{
	// 动作简报不显示
	if (reportid == ReportProSimple)
		return;
	dtureport *rtable = new dtureport;
	// 设置ReportID
	rtable->setCurReportID(reportid);
	widgetlayout->addWidget(rtable);
}

/************************************************ 规约历史文件 *************************************************/
void dtumainmenu::load_rule_ui()
{
	GroupRule = new QTreeWidgetItem(ui->treeWidget);
	GroupRule->setText(0, "规约历史文件");
	QTreeWidgetItem *newgroup = new QTreeWidgetItem(GroupRule);

	newgroup->setText(0, "规约历史文件");

	dturulefilewidget *ruletable = new dturulefilewidget;
	widgetlayout->addWidget(ruletable);
}

/************************************************* 遥控部分 **************************************************/
void dtumainmenu::load_rm_ui()
{
	GroupRmctrl = new QTreeWidgetItem(ui->treeWidget);
	GroupRmctrl->setText(0, "远程遥控");

	QTreeWidgetItem *newgroup = new QTreeWidgetItem(GroupRmctrl);
	newgroup->setText(0, "遥控");

	dturmcwidget *rmctable = new dturmcwidget;

	widgetlayout->addWidget(rmctable);
}


/************************************************ 软遥信部分 **************************************************/
void dtumainmenu::load_modifyfixid_ui()
{
	
	Groupsyx = new QTreeWidgetItem(ui->treeWidget);
	Groupsyx->setText(0, "点表映射");
	QTreeWidgetItem *newgroup = new QTreeWidgetItem(Groupsyx);

	newgroup->setText(0, "点表修改");

	dtufixmodify *fixmtable = new dtufixmodify;

	widgetlayout->addWidget(fixmtable);
}