#pragma execution_character_set("utf-8")

#include "dtuinfomationdialog.h"
#include "ui_dtuinfomationdialog.h"

#include <QTableWidget>
#include <QLabel>

#include <bitset>

#include "QCreateHelper.h"
#include "dtulog.h"
#include "dturpcclient.h"

using namespace DTU;

dtuinfomationDialog::dtuinfomationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuinfomationDialog)
{
    ui->setupUi(this);
	this->load_ui();
	connect(&dturpcclient::instance(), SIGNAL(update_information(unsigned short, QByteArray)),
		this, SLOT(updateinfo(unsigned short, QByteArray)), Qt::QueuedConnection);
}

dtuinfomationDialog::~dtuinfomationDialog()
{
    delete ui;
}

void dtuinfomationDialog::load_ui()
{
	this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
}

void dtuinfomationDialog::setCurIndex(int no)
{
	ui->stackedWidget->setCurrentIndex(no);
	curPage = no;
}

bool dtuinfomationDialog::checkInfoDateSize(uint16_t infoID, int size)
{
	return (infoindex[infoID].size == size) ? true : false;
}

QTableWidget *dtuinfomationDialog::createTabWidget(QTableWidget *itable, int row, QStringList header)
{
	// 设置行数
	itable->setRowCount(row);
	// 设置列数
	itable->setColumnCount(header.size());
	// 设置表头
	itable->setHorizontalHeaderLabels(header);

	// ui修改
	itable->horizontalHeader()->setStretchLastSection(true);// 自动延展
	itable->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置为不可修改
	//ptable->horizontalHeader()->setVisible(false);// 隐藏水平表头
	itable->verticalHeader()->setVisible(false);    // 隐藏垂直表头

	itable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//先自适应宽度
	itable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//然后设置要根据内容使用宽度的列
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	return itable;
}

QTableWidget *dtuinfomationDialog::createTabWidget(QTableWidget *widget, int row, int column)
{
	// 设置行数
	widget->setRowCount(row);
	// 设置列数
	widget->setColumnCount(column);

	widget->horizontalHeader()->setVisible(false);				// 隐藏水平表头
	widget->verticalHeader()->setVisible(false);				// 隐藏垂直表头
	widget->setEditTriggers(QAbstractItemView::NoEditTriggers);	// 设置不可编辑
	widget->setSelectionMode(QAbstractItemView::NoSelection);	// 设置不可选中

	return widget;
}

void dtuinfomationDialog::load_one_widget(QTableWidget *widget, uint16_t infoid)
{
	// 加载标题
	QStringList header;
	header << "序号" << "点表值" << "描述" << "值";

	auto iinfo = infoindex[infoid];
	QTableWidget *itable = createTabWidget(widget, iinfo.info.size(), header);
	int index = 1;
	for (const auto &item : iinfo.info)
	{
		itable->setCellWidget(index - 1, 0, QCreateHelper::createStrLabel(QString::number(index)));//序号
		itable->setCellWidget(index - 1, 1, QCreateHelper::createStrLabel("0x" + QString("%1").arg(item.second.fixid, 2, 16, QLatin1Char('0')).toUpper()));// 点表
		itable->setCellWidget(index - 1, 2, QCreateHelper::createStrLabel(QString::fromStdString(item.second.desc)));// 描述
		itable->setCellWidget(index - 1, 3, QCreateHelper::createWidget(item.second.qtctrl, {
			item.second.desc,0,0,item.second.defaul,1,std::string("")
			}, false));// 值
		index++;
	}
}

void dtuinfomationDialog::init()
{
	// 加载界面窗口(需要预先加载数据库)
	infoindex = DBManager::instance().GetInfomationTable();
	// 加载硬件遥信界面
	auto iinfo = infoindex[InfomHardRemoteSignal];
	int hx_column = iinfo.info.size() / 8;
	if ((iinfo.info.size() % 8) > 0) {
		hx_column++;
	}
	hx_column *= 2;

	QTableWidget *itable = createTabWidget(ui->tableWidget1, 8, hx_column);
	int rowCount = 0;
	int columnCount = 1;
	for (const auto &item : iinfo.info)
	{
		auto oneLabel = new QTableWidgetItem(QString::fromStdString(item.second.desc));
		oneLabel->setTextAlignment(Qt::AlignCenter);
		itable->setItem(rowCount, columnCount - 1, oneLabel);// 描述
		itable->setCellWidget(rowCount, columnCount, QCreateHelper::createWidget(item.second.qtctrl, {
			item.second.desc,0,0,item.second.defaul,1,std::string("")
			}, false));// 值
		rowCount++;
		if (rowCount == 8) {
			// 一列结束
			rowCount = 0;
			columnCount += 2;
		}
	}
	// 窗口尺寸自适应
	for (int i = 0; i < hx_column; i++) {
		itable->resizeColumnToContents(i);
	}
	this->load_one_widget(ui->tableWidget2, InfomTelemetry);	//遥测值
	this->load_one_widget(ui->tableWidget3, InfomSelfCheck);	// 自检状态
	this->load_one_widget(ui->tableWidget4, InfomStatusInfo);	// 状态信息

	// 设置适中窗口
	int maxWith = ui->tableWidget1->columnCount();
	int With = 0;
	for (int i = 0; i < maxWith; i++) {
		With = ui->tableWidget1->columnWidth(i) + With;
	}
	ui->tableWidget1->setMaximumWidth(With + 2);
	ui->tableWidget1->setMinimumWidth(With + 2);

	int maxHeight = ui->tableWidget1->rowCount();
	int Height = 0;
	for (int i = 0; i < maxHeight; i++) {
		Height = ui->tableWidget1->rowHeight(i) + Height;
	}
	ui->tableWidget1->setMaximumHeight(Height + 2);
	ui->tableWidget1->setMinimumHeight(Height + 2);
}

void dtuinfomationDialog::updateinfo(unsigned short infoID, QByteArray data)
{
	//if (infoID != this->infoID)
	//	return;
	// 检查长度 长度错误直接返回
	if (!checkInfoDateSize(infoID, data.size()))
		return;
	DTU::buffer buff(data.data(), data.size());
	switch (infoID)
	{
		// 硬件遥信
	case 0: {
		uint32_t hyx = buff.value<uint32_t>();
		std::bitset<32> hyxbits(hyx);
		int row = 0;
		int column = 1;
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!hyxbits[item.second.offset])
				str = "0";
			QCreateHelper::setWidgetValue(item.second.qtctrl, str.c_str(), str.size(),
				ui->tableWidget1->cellWidget(row, column));
			row++;
			if (row == 8) {
				row = 0;
				column += 2;
			}
		}
	}break;
		// 遥测值
	case 1: {
		int index = 0;
		for (const auto &item : infoindex[infoID].info)
		{
			float value = buff.get(item.second.offset, item.second.size).value<float>();
			std::string svalue = QString::number(value, 'f', 3).toStdString();
			QCreateHelper::setWidgetValue(item.second.qtctrl, svalue.c_str(), svalue.size(),
				ui->tableWidget2->cellWidget(index, 3));
			index++;
		}
	}break;
		// 自检状态
	case 2: {
		std::string bin_str;
		uint8_t bitNum = 0;
		for (int i = 0; i < buff.size(); i++)
		{
			bitNum = buff.get(i, sizeof(bitNum)).value<uint8_t>();
			for (int j = 0; j < 8; j++)
			{
				if ((0x01 << j) & bitNum)
					bin_str = "1" + bin_str;
				else
					bin_str = "0" + bin_str;
			}
		}
		// 这里大于自检信息的位数即可
		std::bitset<272> selfcheckbits(bin_str);
		int widgetIndex = 0;
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!selfcheckbits[item.second.offset])
				str = "0";
			QCreateHelper::setWidgetValue(item.second.qtctrl, (char*)str.c_str(), str.size(),
				ui->tableWidget3->cellWidget(widgetIndex, 3));
			widgetIndex++;
		}
	}break;
		// 状态信息
	case 3: {
		uint32_t hyx = buff.value<uint32_t>();
		std::bitset<32> statebits(hyx);
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!statebits[item.second.offset])
				str = "0";
			QCreateHelper::setWidgetValue(item.second.qtctrl, str.c_str(), str.size(),
				ui->tableWidget4->cellWidget(item.second.offset, 3));
		}
	}break;
	default: {
		DTULOG(DTU_ERROR, "未知的信息查看编号[%04d]", infoID);
	}
	}
}

void dtuinfomationDialog::get_disk_usage()
{
	ui->widget->get_disk_usage();
}

void dtuinfomationDialog::setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2)
{
	ui->widget->setWidget(widget1 ,widget2);
}