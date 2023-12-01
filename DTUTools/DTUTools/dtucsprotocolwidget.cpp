#include "dtucsprotocolwidget.h"

#include <QDateTime>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>

#include "dturpcclient.h"
#include "dtulog.h"
#include "dtustructs.h"

#include "dtucsprotocolanalysis.h"
#include "create_control.h"
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
	load_ui(ui->tableWidget_104);
	load_ui();

	connect(&dturpcclient::instance(), SIGNAL(update_CSLOG(QByteArray)), this, SLOT(update_info(QByteArray)));
	connect(&dturpcclient::instance(), SIGNAL(update_CSMSG(QByteArray)), this, SLOT(update_info(QByteArray)));
	connect(ui->tabWidget,SIGNAL(tabBarClicked(int)), this, SLOT(updatecfg(int)));
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
	QString timestr = datetime.toString("yyyy-mm-dd hh:mm:ss zzz");

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
	default:
		DTULOG(DTU_ERROR, "update_msg() 错误的数据来源");
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

void dtucsprotocolwidget::load_ui()
{
	QStringList Measure;
	Measure << "归一化值" << "带CP56时标的归一化值" << "带CP24时标的归一化值"
		<< "标度化值" << "带CP56时标的标度化值" << "带CP24时标的标度化值"
		<< "短浮点数" << "带CP56时标的短浮点数" << "带CP24时标的短浮点数";

	QStringList Telegram;
	Telegram << "单点遥信" << "带CP56时标的单点遥信" << "带CP24时标的单点遥信"
		<< "双点遥信" << "带CP56时标的双点遥信" << "带CP24时标的双点遥信";
	load_widget(ui->tableWidget104, 9);
	load_widget(ui->tableWidget101, 18);

	// 104协议
	addItemToTableWidget(ui->tableWidget104, 0, 0, createStrLabel("规约是否使用"));
	addItemToTableWidget(ui->tableWidget104, 0, 1, createSwitch("启用1/停用0"));
	addItemToTableWidget(ui->tableWidget104, 1, 0, createStrLabel("数据是否使用压缩格式"));
	addItemToTableWidget(ui->tableWidget104, 1, 1, createSwitch("使用1/不使用0",true));
	addItemToTableWidget(ui->tableWidget104, 2, 0, createStrLabel("IP地址"));
	addItemToTableWidget(ui->tableWidget104, 2, 1, createIPWidget());
	addItemToTableWidget(ui->tableWidget104, 3, 0, createStrLabel("端口号"));
	addItemToTableWidget(ui->tableWidget104, 3, 1, createSpinBox(0,65535,1,2404));
	addItemToTableWidget(ui->tableWidget104, 4, 0, createStrLabel("遥测值类型"));
	addItemToTableWidget(ui->tableWidget104, 4, 1, createComBox(Measure));
	addItemToTableWidget(ui->tableWidget104, 5, 0, createStrLabel("遥信值类型"));
	addItemToTableWidget(ui->tableWidget104, 5, 1, createComBox(Telegram));
	addItemToTableWidget(ui->tableWidget104, 6, 0, createStrLabel("传送原因COT长度"));
	addItemToTableWidget(ui->tableWidget104, 6, 1, createSpinBox(1, 2, 1, 2));
	addItemToTableWidget(ui->tableWidget104, 7, 0, createStrLabel("公共地址CA长度"));
	addItemToTableWidget(ui->tableWidget104, 7, 1, createSpinBox(1, 2, 1, 2));
	addItemToTableWidget(ui->tableWidget104, 8, 0, createStrLabel("信息体地址IOA长度"));
	addItemToTableWidget(ui->tableWidget104, 8, 1, createSpinBox(1, 3, 1, 3));

	// 101协议
	addItemToTableWidget(ui->tableWidget101, 0, 0, createStrLabel("规约是否使用"));
	addItemToTableWidget(ui->tableWidget101, 0, 1, createSwitch("启用1/停用0"));

	addItemToTableWidget(ui->tableWidget101, 1, 0, createStrLabel("数据是否使用压缩格式"));
	addItemToTableWidget(ui->tableWidget101, 1, 1, createSwitch("使用1/不使用0", true));

	addItemToTableWidget(ui->tableWidget101, 2, 0, createStrLabel("规约模式"));
	QStringList mode;
	mode << "平衡模式传输" << "非平衡模式传输";
	addItemToTableWidget(ui->tableWidget101, 2, 1, createComBox(mode));
	addItemToTableWidget(ui->tableWidget101, 3, 0, createStrLabel("其他单元ASDU地址"));
	addItemToTableWidget(ui->tableWidget101, 3, 1, createSpinBox(0, 255, 1, 1));
	addItemToTableWidget(ui->tableWidget101, 4, 0, createStrLabel("串口"));
	QStringList serial;
	serial << "ttyS1" << "ttyS2" << "ttyS3" << "ttyS4" << "ttyS5" << "ttyS6";
	addItemToTableWidget(ui->tableWidget101, 4, 1, createComBox(serial));
	QStringList baudlist;
	baudlist << "115200" << "9600" << "4800" << "2400";
	addItemToTableWidget(ui->tableWidget101, 5, 0, createStrLabel("波特率"));
	addItemToTableWidget(ui->tableWidget101, 5, 1, createComBox(baudlist));
	addItemToTableWidget(ui->tableWidget101, 6, 0, createStrLabel("数据位"));
	addItemToTableWidget(ui->tableWidget101, 6, 1, createSpinBox(5, 8, 1, 8));
	addItemToTableWidget(ui->tableWidget101, 7, 0, createStrLabel("校验方式"));
	QStringList pairty;
	pairty << "无校验" << "偶校验" << "奇校验";
	addItemToTableWidget(ui->tableWidget101, 7, 1, createComBox(pairty));
	addItemToTableWidget(ui->tableWidget101, 8, 0, createStrLabel("停止位"));
	addItemToTableWidget(ui->tableWidget101, 8, 1, createSpinBox(1, 2, 1, 1));
	addItemToTableWidget(ui->tableWidget101, 9, 0, createStrLabel("遥测值类型"));
	addItemToTableWidget(ui->tableWidget101, 9, 1, createComBox(Measure));
	addItemToTableWidget(ui->tableWidget101, 10, 0, createStrLabel("遥信值类型"));
	addItemToTableWidget(ui->tableWidget101, 10, 1, createComBox(Telegram));
	addItemToTableWidget(ui->tableWidget101, 11, 0, createStrLabel("传送原因COT长度"));
	addItemToTableWidget(ui->tableWidget101, 11, 1, createSpinBox(1, 2, 1, 2));
	addItemToTableWidget(ui->tableWidget101, 12, 0, createStrLabel("公共地址CA长度"));
	addItemToTableWidget(ui->tableWidget101, 12, 1, createSpinBox(1, 2, 1, 2));
	addItemToTableWidget(ui->tableWidget101, 13, 0, createStrLabel("信息体地址IOA长度"));
	addItemToTableWidget(ui->tableWidget101, 13, 1, createSpinBox(1, 3, 1, 3));
	addItemToTableWidget(ui->tableWidget101, 14, 0, createStrLabel("链路层地址长度"));
	addItemToTableWidget(ui->tableWidget101, 14, 1, createSpinBox(1, 2, 1, 1));
	addItemToTableWidget(ui->tableWidget101, 15, 0, createStrLabel("使用单字符回复"));
	addItemToTableWidget(ui->tableWidget101, 15, 1, createSwitch("启用1/停用0",true));
	addItemToTableWidget(ui->tableWidget101, 16, 0, createStrLabel("链路层确认超时(毫秒)"));
	addItemToTableWidget(ui->tableWidget101, 16, 1, createSpinBox(1, 100000, 1, 500));
	addItemToTableWidget(ui->tableWidget101, 17, 0, createStrLabel("未收到ACK时重复消息传输超时(毫秒)"));
	addItemToTableWidget(ui->tableWidget101, 17, 1, createSpinBox(1, 100000, 1, 1000));
}

void dtucsprotocolwidget::load_widget(QTableWidget *widget,int row)
{
	// 设置行数
	widget->setRowCount(row);
	// 设置列数
	widget->setColumnCount(2);
	// ui修改
	// 自动延展
	widget->horizontalHeader()->setStretchLastSection(true);
	//设置为不可修改
	widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	// 隐藏水平表头
	widget->horizontalHeader()->setVisible(false);
	// 隐藏垂直表头
	widget->verticalHeader()->setVisible(false);
	// 先自适应宽度
	widget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	// 然后设置要根据内容使用宽度的列
	widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	// 不可选中
	widget->setSelectionMode(QAbstractItemView::NoSelection);
}

void dtucsprotocolwidget::addItemToTableWidget(QTableWidget *ptable, int row, int column, QWidget *widget)
{
	ptable->setCellWidget(row, column, widget);
}

void dtucsprotocolwidget::read_config()
{
	DTU::buffer result;
	if (DTU_SUCCESS == execute_query_data(DTU_GET_PROTO_CONFIG, result))
	{
		DSYSCFG::instance().csunpack(result);
		ui->spinBox_asdu->setValue(DSYSCFG::instance().ASDU());


		// 设置104配置
		GetItem<QCheckBox>(ui->tableWidget104, 0)->setChecked(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.use);
		GetItem<QCheckBox>(ui->tableWidget104, 1)->setChecked(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.EXParam.isSequence);
		GetItem<IPAddress>(ui->tableWidget104, 2)->setIP(
			QString::fromStdString(DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.ip));
		GetItem<SDLPCTSpinBox>(ui->tableWidget104, 3)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.eth.port);
		GetItem<QComboBox>(ui->tableWidget104, 4)->setCurrentIndex(static_cast<int>(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType));
		GetItem<QComboBox>(ui->tableWidget104, 5)->setCurrentIndex(static_cast<int>(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.VType.TelegramValueType));
		GetItem<SDLPCTSpinBox>(ui->tableWidget104, 6)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofCOT);
		GetItem<SDLPCTSpinBox>(ui->tableWidget104, 7)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofCA);
		GetItem<SDLPCTSpinBox>(ui->tableWidget104, 8)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA);


		// 设置101配置
		GetItem<QCheckBox>(ui->tableWidget101, 0)->setChecked(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.use);
		GetItem<QCheckBox>(ui->tableWidget101, 1)->setChecked(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.EXParam.isSequence);
		GetItem<QComboBox>(ui->tableWidget101, 2)->setCurrentIndex(static_cast<int>(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.mode));
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 3)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.otheraddr);
		GetItem<QComboBox>(ui->tableWidget101, 4)->setCurrentText(QString::fromStdString(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.name));
		GetItem<QComboBox>(ui->tableWidget101, 5)->setCurrentText(QString::number(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.baudrate));
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 6)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.databits);
		GetItem<QComboBox>(ui->tableWidget101, 7)->setCurrentIndex(TransParityToindex(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.pairty));
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 8)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.serial.stopbits);
		GetItem<QComboBox>(ui->tableWidget101, 9)->setCurrentIndex(static_cast<int>(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType));
		GetItem<QComboBox>(ui->tableWidget101, 10)->setCurrentIndex(static_cast<int>(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.VType.TelegramValueType));
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 11)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofCA);
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 12)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofCOT);
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 13)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA);
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 14)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.LinkAddrLength);
		GetItem<QCheckBox>(ui->tableWidget101, 15)->setChecked(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.SingalCharACK);
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 16)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForACK);
		GetItem<SDLPCTSpinBox>(ui->tableWidget101, 17)->setValue(
			DSYSCFG::instance().Get_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForRepeat);
	}
	else {
		QtDTULOG(DTU_ERROR,"获取规约配置失败");
		return;
	}
}

void dtucsprotocolwidget::save_config()
{
	if (execute_test_arm_connect())
	{
		DSYSCFG::instance().ASDU(ui->spinBox_asdu->value());
		// 设置104配置
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.use =
			GetItem<QCheckBox>(ui->tableWidget104, 0)->checkState();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.EXParam.isSequence =
			GetItem<QCheckBox>(ui->tableWidget104, 1)->checkState();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.eth.ip =
			GetItem<IPAddress>(ui->tableWidget104, 2)->getIP().toStdString();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.eth.port =
			GetItem<SDLPCTSpinBox>(ui->tableWidget104, 3)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.VType.MeasuredValueType =
			static_cast<MEASURED_TYPE>(GetItem<QComboBox>(ui->tableWidget104, 4)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.VType.TelegramValueType =
			static_cast<TELEGRAM_TYPE>(GetItem<QComboBox>(ui->tableWidget104, 5)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.ALParam.sizeofCOT =
			GetItem<SDLPCTSpinBox>(ui->tableWidget104, 6)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.ALParam.sizeofCA =
			GetItem<SDLPCTSpinBox>(ui->tableWidget104, 7)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS104.ALParam.sizeofIOA =
			GetItem<SDLPCTSpinBox>(ui->tableWidget104, 8)->value();

		// 配置101配置
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.use =
			GetItem<QCheckBox>(ui->tableWidget101, 0)->checkState();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.EXParam.isSequence =
			GetItem<QCheckBox>(ui->tableWidget101, 1)->checkState();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.mode =
			static_cast<LinkLayerMode>(GetItem<QComboBox>(ui->tableWidget101, 2)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.otheraddr =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 3)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.serial.name =
			GetItem<QComboBox>(ui->tableWidget101, 4)->currentText().toStdString();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.serial.baudrate =
			GetItem<QComboBox>(ui->tableWidget101, 5)->currentText().toInt();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.serial.databits =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 6)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.serial.pairty =
			TransIndexToParity(GetItem<QComboBox>(ui->tableWidget101, 7)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.serial.stopbits =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 8)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.VType.MeasuredValueType =
			static_cast<MEASURED_TYPE>(GetItem<QComboBox>(ui->tableWidget101, 9)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.VType.TelegramValueType =
			static_cast<TELEGRAM_TYPE>(GetItem<QComboBox>(ui->tableWidget101, 10)->currentIndex());
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.ALParam.sizeofCA =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 11)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.ALParam.sizeofCOT =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 12)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.ALParam.sizeofIOA =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 13)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.LLParam.LinkAddrLength =
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 14)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.LLParam.SingalCharACK =
			GetItem<QCheckBox>(ui->tableWidget101, 15)->checkState();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForACK = 
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 16)->value();
		DSYSCFG::instance().Modify_IEC60870_Slave_CFG().CS101.LLParam.TimeoutForRepeat = 
			GetItem<SDLPCTSpinBox>(ui->tableWidget101, 17)->value();

		DTU::buffer result = DSYSCFG::instance().cspack();
		// 获取文件路径
		std::string execPath;
		if (DTU_FAILED(execute_get_filepath(execPath)))
		{
			DTULOG(DTU_ERROR, "save_config() 获取路径失败");
			return;
		}
		execPath = execPath + "/update/config/csprotocol.json";
		// 传送文件
		if (DTU_SUCCESS != execute_set_file(execPath, result))
		{
			DTULOG(DTU_ERROR, "save_config() 获取失败");
			return;
		}
		// 执行升级
		uint16_t tag = 0x1000;
		execute_updateprogram(tag);
	}
	else
	{
		QtDTULOG(DTU_WARN,"ARM后台未连接");
	}
}

void dtucsprotocolwidget::default_config()
{
	DSYSCFG::instance().csrecover();

	DTU::buffer result = DSYSCFG::instance().cspack();
	// 获取文件路径
	std::string execPath;
	if (DTU_FAILED(execute_get_filepath(execPath)))
	{
		DTULOG(DTU_ERROR, "save_config() 获取路径失败");
		return;
	}
	execPath = execPath + "/update/config/csprotocol.json";
	// 传送文件
	if (DTU_SUCCESS != execute_set_file(execPath, result))
	{
		DTULOG(DTU_ERROR, "save_config() 获取失败");
		return;
	}
	// 执行升级
	uint16_t tag = 0x1000;
	execute_updateprogram(tag);
	read_config();
}

void dtucsprotocolwidget::updatecfg(int index)
{
	if (index == 2)
	{
		if (execute_test_arm_connect())
		{
			read_config();
		}
	}
}

int dtucsprotocolwidget::TransParityToindex(char parity)
{
	int ret = 0;
	switch (parity)
	{
	case 'N': ret = 0; break;
	case 'E': ret = 1; break;
	case 'O': ret = 2; break;
	}
	return ret;
}

char dtucsprotocolwidget::TransIndexToParity(int index)
{
	char ret = 'N';
	switch (index)
	{
	case 0: ret = 'N'; break;
	case 1: ret = 'E'; break;
	case 2: ret = 'O'; break;
	}
	return ret;
}

void dtucsprotocolwidget::explain_101(int row, int column)
{
	QString data = ui->tableWidget_101->item(row, 4)->text();
	explain_csproto(101, data, ui->tableWidget_101->item(row, 2)->text(),"101",
		ui->tableWidget_101->item(row, 0)->text(),
		ui->tableWidget_101->item(row, 1)->text());
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
	printf("row %d\n", ui->tableWidget_104->rowCount());
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
	printf("row %d\n",ui->tableWidget_101->rowCount());
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