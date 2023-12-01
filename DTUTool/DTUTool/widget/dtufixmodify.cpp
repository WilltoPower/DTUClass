#pragma execution_character_set("utf-8")

#include "dtufixmodify.h"

#include <QLabel>
#include <QScrollBar>

#include "dtulog.h"
#include "QCreateHelper.h"
#include "dtutask.h"
#include "dtuonefixmodify.h"

using namespace DTU;

dtufixmodify::dtufixmodify(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtufixmodifyClass())
{
	ui->setupUi(this);
	//load_ui();
}

dtufixmodify::~dtufixmodify()
{
	delete ui;
}

void dtufixmodify::load_ui()
{
	// 创建布局器
	widgetlayout = new QStackedLayout;
	widgetlayout->setStackingMode(QStackedLayout::StackOne);
	// 设置布局器
	ui->widget->setLayout(widgetlayout);

	// 1.创建遥信索引表
	auto rmtable = DBManager::instance().GetSOEIndex();
	QStringList header;
	header << "序号" << "映射点表值" << "分组" << "描述";
	
	QTableWidget *tableSOE = nullptr;

	createTabWidget(tableSOE,rmtable.size(), header);
	// 默认窗口
	CurWidget = tableSOE;

	int index = 0;
	for (const auto &item : rmtable)
	{
		// 序号
		tableSOE->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
		tableSOE->item(index, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 映射点表
		tableSOE->setItem(index, 1, new QTableWidgetItem("0x" + QString("%1").arg(
			DBManager::instance().FixidMapIntoout(MAP_YX, item.first), 4, 16, QLatin1Char('0')).toUpper()));
		tableSOE->item(index, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 分组
		tableSOE->setItem(index, 2, new QTableWidgetItem(QString::fromStdString(item.second.adddesc)));
		tableSOE->item(index, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 描述
		tableSOE->setItem(index, 3, new QTableWidgetItem(QString::fromStdString(item.second.desc)));
		tableSOE->item(index, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		
		index++;
	}
	connect(tableSOE, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(DoubleClickedItem(int, int)));
	widgetlayout->addWidget(tableSOE);
	// 2.创建遥测点表
	auto rctable = DBManager::instance().GetCOSIndex();
	header.clear();
	header << "序号" << "映射点表值" << "分组" << "描述";

	QTableWidget *tableCOS = nullptr;

	createTabWidget(tableCOS, rctable.size(), header);
	
	index = 0;
	for (const auto &item : rctable)
	{
		// 序号
		tableCOS->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
		tableCOS->item(index, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 映射点表
		tableCOS->setItem(index, 1, new QTableWidgetItem("0x" + QString("%1").arg(
			DBManager::instance().FixidMapIntoout(MAP_YC ,item.first), 4, 16, QLatin1Char('0')).toUpper()));
		tableCOS->item(index, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 分组
		tableCOS->setItem(index, 2, new QTableWidgetItem("遥测量"));
		tableCOS->item(index, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 描述
		tableCOS->setItem(index, 3, new QTableWidgetItem(QString::fromStdString(item.second.desc)));
		tableCOS->item(index, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		index++;
	}
	connect(tableCOS, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(DoubleClickedItem(int, int)));
	widgetlayout->addWidget(tableCOS);
	// 3.创建遥控点表
	auto rmctable = DBManager::instance().GetRMCIndex();
	
	header.clear();
	header << "序号" << "映射点表值" << "分组" << "描述";

	QTableWidget *tableRMC = nullptr;

	createTabWidget(tableRMC, rmctable.size(), header);
	
	index = 0;
	for (const auto &item : rmctable)
	{
		// 序号
		tableRMC->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
		tableRMC->item(index, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 映射点表
		tableRMC->setItem(index, 1, new QTableWidgetItem("0x" + QString("%1").arg(
			DBManager::instance().FixidMapIntoout(MAP_YK, item.second.addr), 4, 16, QLatin1Char('0')).toUpper()));
		tableRMC->item(index, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 分组
		tableRMC->setItem(index, 2, new QTableWidgetItem("遥控"));
		tableRMC->item(index, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 描述
		tableRMC->setItem(index, 3, new QTableWidgetItem(QString::fromStdString(item.second.desc)));
		tableRMC->item(index, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		index++;
	}
	connect(tableRMC, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(DoubleClickedItem(int, int)));
	widgetlayout->addWidget(tableRMC);
	// 4.创建所有定值表
	auto rptable = DBManager::instance().GetParamInfo();

	header.clear();
	header << "序号" << "映射点表值" << "分组" << "描述";

	QTableWidget *tableRP = nullptr;

	int size = 0;

	for (auto &item : rptable)
	{
		if (item.second.pid == ParamCommunication || item.second.pid == ParamAutomation ||
			item.second.pid == ParamDevice || item.second.pid == ParamAdjust) {
			continue;
		}
		size = size + item.second.info.size();
	}

	createTabWidget(tableRP, size, header);

	index = 0;
	for (const auto &item : rptable)
	{
		if (item.second.pid == ParamCommunication || item.second.pid == ParamAutomation ||
			item.second.pid == ParamDevice || item.second.pid == ParamAdjust) {
			continue;
		}
		for (const auto &item1 : item.second.info)
		{
			// 序号
			tableRP->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
			tableRP->item(index, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			// 映射点表
			tableRP->setItem(index, 1, new QTableWidgetItem("0x" + QString("%1").arg(
				DBManager::instance().FixidMapIntoout(MAP_YT, item1.second.fixid), 4, 16, QLatin1Char('0')).toUpper()));
			tableRP->item(index, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			// 分组
			tableRP->setItem(index, 2, new QTableWidgetItem(QString::fromStdString(item.second.desc)));
			tableRP->item(index, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			// 描述
			tableRP->setItem(index, 3, new QTableWidgetItem(QString::fromStdString(item1.second.desc)));
			tableRP->item(index, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			index++;
		}
	}
	connect(tableRP, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(DoubleClickedItem(int, int)));
	widgetlayout->addWidget(tableRP);
	// 5.创建自动化参数信息
	auto &autable = DBManager::instance().GetParamInfoByID(ParamAutomation);
	header.clear();
	header << "序号" << "映射点表值" << "分组" << "描述";

	QTableWidget *tableAuto = nullptr;
	
	createTabWidget(tableAuto, autable.info.size(), header);

	index = 0;
	for (const auto &item : autable.info)
	{
		// 序号
		tableAuto->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
		tableAuto->item(index, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 映射点表
		tableAuto->setItem(index, 1, new QTableWidgetItem("0x" + QString("%1").arg(
			DBManager::instance().FixidMapIntoout(MAP_AU, item.second.fixid), 4, 16, QLatin1Char('0')).toUpper()));
		tableAuto->item(index, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 分组
		tableAuto->setItem(index, 2, new QTableWidgetItem(QString::fromStdString(autable.desc)));
		tableAuto->item(index, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		// 描述
		tableAuto->setItem(index, 3, new QTableWidgetItem(QString::fromStdString(item.second.desc)));
		tableAuto->item(index, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		index++;
	}
	connect(tableAuto, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(DoubleClickedItem(int, int)));
	widgetlayout->addWidget(tableAuto);
}

// 创建TableWidget控件
void dtufixmodify::createTabWidget(QTableWidget *&ptable, int row, QStringList header)
{
	if (ptable == nullptr) {
		ptable = new QTableWidget;
	}
	// 设置行数
	ptable->setRowCount(row);
	// 设置列数
	ptable->setColumnCount(header.size());
	// 设置表头
	ptable->setHorizontalHeaderLabels(header);

	// ui修改
	// 自动延展
	ptable->horizontalHeader()->setStretchLastSection(true);
	//设置为不可修改
	ptable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	// 隐藏水平表头
	//ptable->horizontalHeader()->setVisible(false);
	// 隐藏垂直表头
	ptable->verticalHeader()->setVisible(false);
	// 先自适应宽度
	ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	// 然后设置要根据内容使用宽度的列
	ptable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ptable->setSelectionBehavior(QAbstractItemView::SelectRows);
	ptable->setSelectionMode(QAbstractItemView::SingleSelection);
}

void dtufixmodify::serach()
{
	indexvec.clear();
	if (CurWidget != nullptr) {
		if (!ui->lineEdit->text().isEmpty()) {
			std::string finder = ui->lineEdit->text().toStdString();
			for (int i = 0; i < CurWidget->rowCount(); i++)
			{
				if (CurWidget->item(i, 3)->text().contains(ui->lineEdit->text())) {
					indexvec.emplace_back(i);
				}
			}
			if (indexvec.size() > 0) {
				CurWidget->verticalScrollBar()->setSliderPosition(indexvec[0]);
				QString text = "结果共" + QString::number(indexvec.size()) + "项";
				ui->label_result->setText(text);
				ui->label_page->setText("当前第1项");
				CurNo = 1;
			}
			else {
				ui->label_result->setText("结果共0项");
				ui->label_page->setText("当前第0项");
			}
		}
	}
}

void dtufixmodify::front()
{
	if (CurWidget != nullptr) {
		if (indexvec.size() > 0) {
			if (CurNo - 1 < 0) {
				CurNo = indexvec.size() - 1;
			}
			else {
				CurNo--;
			}
			CurWidget->verticalScrollBar()->setSliderPosition(indexvec[CurNo]);
			QString text = "当前第" + QString::number(CurNo + 1) + "项";
			ui->label_page->setText(text);
		}
	}
}

void dtufixmodify::next()
{
	if (CurWidget != nullptr) {
		if (indexvec.size() > 0) {
			if (CurNo + 1 >= indexvec.size()) {
				CurNo = 0;
			}
			else {
				CurNo++;
			}
			CurWidget->verticalScrollBar()->setSliderPosition(indexvec[CurNo]);
			QString text = "当前第" + QString::number(CurNo + 1) + "项";
			ui->label_page->setText(text);
		}
	}
}

void dtufixmodify::indexchange(int index)
{
	indexvec.clear();
	CurNo = -1;
	widgetlayout->setCurrentIndex(index);
	CurWidget = ((QTableWidget*)(widgetlayout->currentWidget()));
}

void dtufixmodify::DoubleClickedItem(int row, int column)
{
	if (CurWidget != nullptr) {
		bool ok;
		static dtuonefixmodify *widget;
		if (widget == nullptr) {
			widget = new dtuonefixmodify(
				ui->combox_index->currentIndex(),
				CurWidget->item(row, 1)->text().toInt(&ok, 16),
				CurWidget->item(row, 3)->text(),
				CurWidget->item(row, 2)->text(),
				CurWidget->item(row, 1));
		}
		else {
			delete widget;
			widget = new dtuonefixmodify(
				ui->combox_index->currentIndex(),
				CurWidget->item(row, 1)->text().toInt(&ok, 16),
				CurWidget->item(row, 3)->text(),
				CurWidget->item(row, 2)->text(),
				CurWidget->item(row, 1));
		}
		widget->show();
	}
}