#pragma execution_character_set("utf-8")

#include "dtucsprotocolwidget.h"

#include <QDateTime>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>

#include "dturpcclient.h"
#include "dtulog.h"
#include "dtustructs.h"

#include "dtucsprotocolanalysis.h"
#include "QCreateHelper.h"
#include "dtusystemconfig.h"
#include "dtutask.h"

using namespace DTU;
using namespace DTUCFG;

#define QSTR(s) QString::fromLocal8Bit(s)

dtucsprotocolwidget::dtucsprotocolwidget(QWidget *parent) : QWidget(parent),
	ui(new Ui::dtucsprotocolwidget)
{
	ui->setupUi(this);

	load_ui(ui->tableWidget_101);
	load_ui(ui->tableWidget_1012);
	load_ui(ui->tableWidget_104);

	connect(&dturpcclient::instance(), SIGNAL(update_CSLOG(QByteArray)), this, SLOT(update_info(QByteArray)));
	connect(&dturpcclient::instance(), SIGNAL(update_CSMSG(QByteArray)), this, SLOT(update_info(QByteArray)));
}

void dtucsprotocolwidget::init_widget(QTableWidget *widget)
{
	widget->clear();
	// 没有此行可能不会显示
	widget->setRowCount(1);
	// 隐藏第一行
	widget->setRowHidden(0, true);
	QStringList headerstr;
	headerstr << "方向" << "长度" << "时间" << "描述" << "数据" ;
	// 设置标题
	widget->setHorizontalHeaderLabels(headerstr);
}

void dtucsprotocolwidget::load_ui(QTableWidget *widget)
{
	widget->setEditTriggers(QAbstractItemView::NoEditTriggers);   //禁止修改
	widget->setSelectionMode(QAbstractItemView::SingleSelection); //设置为可以选中单个
	widget->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
	// 方向 长度 时间 数据 描述
	widget->setColumnCount(5);
	widget->horizontalHeader()->installEventFilter(this);

	widget->horizontalHeader()->setStretchLastSection(true);// 自动延展
	widget->verticalHeader()->setDefaultSectionSize(25);

	widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);// 设置列宽自适应
	widget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);// 设置列宽自适应
	widget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);// 设置列宽自适应
	widget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);// 设置列宽自适应
	// 初始化 清空 设置隐藏一行
	init_widget(widget);
}

void dtucsprotocolwidget::clear_101()
{
	init_widget(ui->tableWidget_101);
	CS101_index = 0;
}

void dtucsprotocolwidget::clear_1012()
{
	init_widget(ui->tableWidget_1012);
	CS1012_index = 0;
}

void dtucsprotocolwidget::clear_104()
{
	init_widget(ui->tableWidget_104);
	CS104_index = 0;
}

void dtucsprotocolwidget::AutoScroll_101()
{
	scroolflag_101 = scroolflag_101 ? false : true;
	if (scroolflag_101)
		ui->btn_roll_101->setText("滚动显示⏬");
	else
		ui->btn_roll_101->setText("滚动显示");
}

void dtucsprotocolwidget::AutoScroll_1012()
{
	scroolflag_1012 = scroolflag_1012 ? false : true;
	if (scroolflag_1012)
		ui->btn_roll_1012->setText("滚动显示⏬");
	else
		ui->btn_roll_1012->setText("滚动显示");
}

void dtucsprotocolwidget::AutoScroll_104()
{
	scroolflag_104 = scroolflag_104 ? false : true;
	if (scroolflag_104)
		ui->btn_roll_104->setText("滚动显示⏬");
	else
		ui->btn_roll_104->setText("滚动显示");
}

dtucsprotocolwidget::~dtucsprotocolwidget(){}

void dtucsprotocolwidget::update_info(QByteArray data)
{
	DTU::buffer result(data.data(),data.size());
	parase(result);
}

void dtucsprotocolwidget::parase(DTU::buffer &data)
{
	try
	{
		uint64_t msec = data.get(0, sizeof(uint64_t)).value<uint64_t>();// 毫秒时间戳
		uint8_t from = data.get(8, sizeof(uint8_t)).value<uint8_t>();// 数据规约来源
		uint8_t flag = data.get(9, sizeof(uint8_t)).value<uint8_t>();// 是报文还是数据
		bool sent = data.get(10, sizeof(bool)).value<bool>();// 收/发
		uint32_t size = data.get(11, sizeof(uint32_t)).value<uint32_t>();// 数据长度
		switch (flag)
		{
		case CSLOG: {
			update_log(msec, from, std::string(data.get(15, size).data(), size));
		}break;
		case CSMSG: {
			update_msg(msec, from, sent, (uint8_t*)(data.get(15, size).data()), size);
		}break;
		default:
			DTULOG(DTU_ERROR, "parase() 错误的数据来源");
			break;
		}
	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "parase() 解析发生错误%s",e.what());
	}
	
}

// 添加日志
void dtucsprotocolwidget::update_log(uint64_t msec, uint8_t protofrom, std::string log)
{
	switch (protofrom)
	{
	case 101:ui->textBrowser_101->insertPlainText(QString::fromStdString(log) + "\n"); break;
	case 102:ui->textBrowser_1012->insertPlainText(QString::fromStdString(log) + "\n"); break;
	case 104:ui->textBrowser_104->insertPlainText(QString::fromStdString(log) + "\n"); break;
	default:
		DTULOG(DTU_ERROR,"update_log() 错误的数据来源");
	}
}

void dtucsprotocolwidget::update_msg(uint64_t msec, uint8_t protofrom, bool sent, uint8_t *msg, uint32_t msgsize)
{
	QString dir;
	if (sent)
		dir = "[装置]→[主站]";
	else
		dir = "[主站]→[装置]";

	QDateTime datetime = QDateTime::fromMSecsSinceEpoch(msec);
	QString timestr = datetime.toString("yyyy-MM-dd hh:mm:ss zzz");

	QString data;
	for (int i = 0; i < msgsize; i++)
	{
		data = data + QString("%1 ").arg(msg[i],2,16, QLatin1Char('0')).toUpper();
	}
	data = data.left(data.size() - 1);

	Analysis ans;

	uint16_t ansno;
	if (dir == "[装置]→[主站]")
		ansno = 0;
	else
		ansno = 1;

	switch (protofrom)
	{
	case 101: {
		tableWidgetInsert(ui->tableWidget_101, scroolflag_101, CS101_index, dir,
			msgsize, timestr, data, ans.get_desc_by_Ti(msg[index_Ti_101], ansno) + " " + ans.get_desc_by_COT(msg[index_COT_101], ansno));
	}break;
	case 104: {
		tableWidgetInsert(ui->tableWidget_104, scroolflag_104, CS104_index, dir,
			msgsize, timestr, data, ans.get_desc_by_Ti(msg[index_Ti_104], ansno) + " " + ans.get_desc_by_COT(msg[index_COT_104], ansno));
	}break;
	case 102: {
		tableWidgetInsert(ui->tableWidget_1012, scroolflag_1012, CS1012_index, dir,
			msgsize, timestr, data, ans.get_desc_by_Ti(msg[index_Ti_101], ansno) + " " + ans.get_desc_by_COT(msg[index_COT_101], ansno));
	}break;
	default:
		DTULOG(DTU_ERROR, "update_msg() 错误的数据来源 [%d]", protofrom);
	}
}

QString dtucsprotocolwidget::analysisMessage(uint8_t flag, uint8_t *msg, uint32_t msgsize)
{
	QString ret;
	return ret;
}

void dtucsprotocolwidget::tableWidgetInsert(QTableWidget *widget,bool scroll,uint64_t &index, 
	QString dir, uint32_t size, QString time, QString data, QString desc)
{
	QColor color;
	if (dir == "[装置]→[主站]")
		color = QColor(255, 48, 48);
	else
		color = QColor(72,118,255);

	widget->insertRow(widget->rowCount() - 1);
	setOneWidget(widget, index, 0, dir, color);  // 方向
	setOneWidget(widget, index, 1, QString::number(size), color); // 长度
	setOneWidget(widget, index, 2, time, color); // 时间
	setOneWidget(widget, index, 3, desc, color); // 描述
	setOneWidget(widget, index, 4, data, color); // 数据

	if (scroll)
	{
		widget->scrollToBottom();
	}

	index++;
}

void dtucsprotocolwidget::setOneWidget(QTableWidget *widget,uint64_t index,int no,QString str, QColor color)
{
	QTableWidgetItem *table = nullptr;
	table = new QTableWidgetItem(str);// 创建item
	table->setTextColor(color);
	table->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);// 居中对齐
	widget->setItem(index, no, table);// 设置item
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void dtucsprotocolwidget::explain_101(int row, int column)
{
	QString data = ui->tableWidget_101->item(row, 4)->text();
	explain_csproto(101, data, ui->tableWidget_101->item(row, 2)->text(),"101",
		ui->tableWidget_101->item(row, 0)->text(),
		ui->tableWidget_101->item(row, 1)->text());
}

void dtucsprotocolwidget::explain_1012(int row, int column)
{
	QString data = ui->tableWidget_1012->item(row, 4)->text();
	explain_csproto(101, data, ui->tableWidget_1012->item(row, 2)->text(), "101",
		ui->tableWidget_1012->item(row, 0)->text(),
		ui->tableWidget_1012->item(row, 1)->text());
}

void dtucsprotocolwidget::explain_104(int row, int column)
{
	QString data = ui->tableWidget_104->item(row, 4)->text();
	explain_csproto(104, data, ui->tableWidget_104->item(row, 2)->text(), "104",
		ui->tableWidget_104->item(row, 0)->text(),
		ui->tableWidget_104->item(row, 1)->text());
}

void dtucsprotocolwidget::explain_csproto(int type, QString &text, QString time, QString proto, QString dir, QString size)
{
	if (explainWidget != nullptr) {
		delete explainWidget;
	}
	explainWidget = new dtucsprotocolexplain;
	explainWidget->setText(text,time,proto,dir,size);
	explainWidget->show();
}

void dtucsprotocolwidget::save_file_104()
{
	if (ui->tableWidget_104->rowCount() == 1) {
		QMessageBox::critical(this, "导出", "无报文无法导出", QMessageBox::Ok);
		return;
	}
	if (exportWidget) {
		delete exportWidget;
	}
	exportWidget = new dtuexportwidget(dtuexportwidget::EXPCSPROTO,
							"104规约报文", ui->tableWidget_104->rowCount() - 1, 104);
	exportWidget->setWidgetPointer(ui->tableWidget_104);
	exportWidget->show();
}

void dtucsprotocolwidget::save_file_101()
{
	if (ui->tableWidget_101->rowCount() == 1) {
		QMessageBox::critical(this, "导出", "无报文无法导出", QMessageBox::Ok);
		return;
	}
	if (exportWidget) {
		delete exportWidget;
	}
	exportWidget = new dtuexportwidget(dtuexportwidget::EXPCSPROTO, 
							"101规约报文", ui->tableWidget_101->rowCount() - 1, 101);
	exportWidget->setWidgetPointer(ui->tableWidget_101);
	exportWidget->show();
}

void dtucsprotocolwidget::save_file_1012()
{
	if (ui->tableWidget_1012->rowCount() == 1) {
		QMessageBox::critical(this, "导出", "无报文无法导出", QMessageBox::Ok);
		return;
	}
	if (exportWidget) {
		delete exportWidget;
	}
	exportWidget = new dtuexportwidget(dtuexportwidget::EXPCSPROTO,
		"101规约报文", ui->tableWidget_1012->rowCount() - 1, 101);
	exportWidget->setWidgetPointer(ui->tableWidget_1012);
	exportWidget->show();
}