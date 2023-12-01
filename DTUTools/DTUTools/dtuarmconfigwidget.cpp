#include "dtuarmconfigwidget.h"
#include "ui_dtuarmconfigwidget.h"

#include <fstream>
#include <dtulog.h>
#include <dtutask.h>
#include <dtucmdcode.h>

// 取反
#define INVERT(st) st?false:true;

dtuarmconfigwidget::dtuarmconfigwidget(QWidget *parent)
	: QWidget(parent)
{
	ui = new Ui::dtuarmconfigwidget();
	ui->setupUi(this);

	load_ui();
	if (execute_test_arm_connect())
	{
		SetAll();
	}
	load_connect();
}

dtuarmconfigwidget::~dtuarmconfigwidget()
{
	delete ui;
}

void dtuarmconfigwidget::load_ui()
{
	// 控件List
	tty_ui_list << ui->lcd_tty << ui->line_tty << ui->interval_101_tty;
	baud_ui_list  << ui->lcd_baud << ui->line_baud << ui->interval_101_baud;

	protocol << "RPC" << "CS101" << "CS104";// 装置间通信协议
	connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(interval_double_click(QTreeWidgetItem*, int)));
}

void dtuarmconfigwidget::load_connect()
{
	connect(ui->interval_check, SIGNAL(stateChanged(int)), this, SLOT(GetCurBay()));
	connect(ui->interval_addr, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_mode, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_pri_ip, SIGNAL(IPChanged(QString)), this, SLOT(GetCurBay()));
	connect(ui->interval_pri_port, SIGNAL(valueChanged(int)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_mode, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_tty, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_baud, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_data, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_parity, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_101_stop, SIGNAL(currentTextChanged(const QString&)), this, SLOT(GetCurBay()));
	connect(ui->interval_104_ip, SIGNAL(IPChanged(QString)), this, SLOT(GetCurBay()));
	connect(ui->interval_104_port, SIGNAL(valueChanged(int)), this, SLOT(GetCurBay()));
}

void dtuarmconfigwidget::interval_double_click(QTreeWidgetItem *item, int column)
{
	int index = ui->treeWidget->indexOfTopLevelItem(item);
	SetBay(index);
}

/******************************* 设置界面 *******************************************/
QStringList dtuarmconfigwidget::notInclude(QStringList src, QStringList dest)
{
	for (auto &item : dest)
	{
		src.removeOne(item);
	}
	return src;
}

void dtuarmconfigwidget::SetAll()
{
	try
	{
	DTU::buffer result;
	if (DTU_SUCCESS != execute_query_data(DTU_GET_SYS_CONFIG, result))
	{
		DTULOG(DTU_ERROR,"dtuarmconfigwidget::SetAll() 获取失败");
		return;
	}

	if (result.size() == 0)
	{
		DTULOG(DTU_ERROR, "dtuarmconfigwidget::SetAll() 长度为0");
		return;
	}

	Json::Value jroot;
	std::istringstream jsstream(std::string(result.data(), result.size()));

	Json::CharReaderBuilder readerbuilder;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(readerbuilder, jsstream, &jroot, &errs)) {
		DTULOG(DTU_ERROR, (char*)"解析Json失败%s ", errs.c_str());
		return;
	}

	SetASDU(jroot["unittype"].asInt(), jroot["asduaddr"].asInt(), jroot["ortheraddr"].asInt(),
		jroot["filepath"].asString(), jroot["bay"]["localrpcport"].asInt());

	SetTime(jroot["autotime"]["used"].asBool(), jroot["autotime"]["interval"].asInt());

	SetLine(jroot["pic100"]["used"].asBool(), jroot["pic100"]["serial"].asString().erase(0,5), jroot["pic100"]["baudrate"].asInt());

	SetLCD(jroot["mcu"]["serial"].asString().erase(0,5), jroot["mcu"]["baudrate"].asInt());

	SetCommonunit(jroot["bay"]["commonunitrpcip"].asString(), jroot["bay"]["commonunitrpcport"].asInt());

	Json::Value csproto = jroot["protocol"];
	Json::Value cs101 = csproto["CS101"];
	Json::Value cs104 = csproto["CS104"];
	Json::Value cscom = csproto["AppLayerParameters"];
	
	interval_info = jroot["bay"]["remote"];
	SetBay(0);

	}
	catch (std::exception& e)
	{
		DTULOG(DTU_ERROR, "SetAll() 发生未知错误 %s", e.what());
		return;
	}
}

void dtuarmconfigwidget::GetAll()
{
	try
	{
		Json::Value root;

		GetASDU(root);
		GetTime(root);
		GetLine(root);
		GetLCD(root);
		GetAllBay(root);
		GetCommonunit(root);

		Json::StyledWriter  swriter;

		std::string str = swriter.write(root);

		std::stringstream output(str);
		DTU::buffer result(str.c_str(),str.size());

		// 获取文件路径
		std::string execPath;
		if (DTU_FAILED(execute_get_filepath(execPath)))
		{
			DTULOG(DTU_ERROR,"GetAll() 获取路径失败");
			return;
		}
		execPath = execPath + "/update/config/syscfg.json";
		// 传送文件
		if (DTU_SUCCESS != execute_set_file(execPath, result))
		{
			DTULOG(DTU_ERROR, "dtuarmconfigwidget::SetAll() 获取失败");
			return;
		}
		// 执行升级
		uint16_t tag = 0x1000;
		execute_updateprogram(tag);
	}
	catch (std::exception &e)
	{
		DTULOG(DTU_ERROR, "GetAll() 发生未知错误 %s", e.what());
		return;
	}
}

void dtuarmconfigwidget::SetASDU(int mode, int addr, int other, std::string path, int port)
{
	if(mode > 1) {
		ui->asdu_mode->setCurrentIndex(mode-1);// 0公共单元 1间隔单元
	}
	ui->asdu_port->setValue(port);

	ASDU_select();
}

void dtuarmconfigwidget::SetCommonunit(std::string ip,int port)
{
	ui->com_ip->setIP(QString::fromStdString(ip));
	ui->com_port->setValue(port);
}

void dtuarmconfigwidget::SetTime(bool check, int time)
{
	ui->time_check->setChecked(check);
	ui->time_delay->setValue(time);

	Time_check();
}

void dtuarmconfigwidget::SetLine(bool check, std::string tty, int baud)
{
	ui->line_check->setChecked(check);
	ui->line_tty->setCurrentText(QString::fromStdString(tty));
	ui->line_baud->setCurrentText(QString::number(baud));
	Line_check();
}

void dtuarmconfigwidget::SetLCD(std::string tty, int baud)
{
	ui->lcd_tty->setCurrentText(QString::fromStdString(tty));
	ui->lcd_baud->setCurrentText(QString::number(baud));
}

void dtuarmconfigwidget::SetBay(int index)
{
	if (interval_info.empty())
		return;

	curBayNo = index;

	Json::Value bay = interval_info[index];
	ui->interval_check->setChecked(bay["used"].asBool());
	ui->interval_addr->setCurrentIndex(bay["asdu"].asInt() - 1);
	ui->interval_mode->setCurrentIndex(protocol.indexOf(QString::fromStdString(bay["protocol"].asString())));

	ui->interval_pri_ip->setIP(QString::fromStdString(bay["RPC"]["ip"].asString()));
	ui->interval_pri_port->setValue(bay["RPC"]["port"].asInt());

	ui->interval_101_mode->setCurrentIndex(bay["CS101"]["mode"].asInt());
	ui->interval_101_tty->setCurrentText(QString::fromStdString(bay["CS101"]["serial"].asString().erase(0,5)));
	ui->interval_101_baud->setCurrentText(QString::number(bay["CS101"]["baudrate"].asInt()));
	ui->interval_101_data->setCurrentIndex(8 - bay["CS101"]["databits"].asInt());
	ui->interval_101_parity->setCurrentIndex(bay["CS101"]["parity"].asInt());
	ui->interval_101_stop->setCurrentIndex(bay["CS101"]["stopbits"].asInt() - 1);

	ui->interval_104_ip->setIP(QString::fromStdString(bay["CS104"]["ip"].asString()));
	ui->interval_104_port->setValue(bay["CS104"]["port"].asInt());

}

void dtuarmconfigwidget::SetInterval(bool check, int addr, std::string mode)
{
	ui->interval_check->setChecked(check);
	ui->interval_addr->setCurrentIndex(addr);
	ui->interval_mode->setCurrentIndex(protocol.indexOf(QString::fromStdString(mode)));
}

void dtuarmconfigwidget::GetASDU(Json::Value &root)
{
	root["unittype"] = ui->asdu_mode->currentIndex();
	root["bay"]["localrpcport"] = ui->asdu_port->value();
}

void dtuarmconfigwidget::GetTime(Json::Value &root)
{
	root["autotime"]["used"] = ui->time_check->isChecked();
	root["autotime"]["interval"] = ui->time_delay->value();
}

void dtuarmconfigwidget::GetLine(Json::Value &root)
{
	root["pic100"]["used"] = ui->line_check->isChecked();
	root["pic100"]["serial"] = "/dev/" + ui->line_tty->currentText().toStdString();
	root["pic100"]["baudrate"] = ui->line_baud->currentText().toInt();
}

void dtuarmconfigwidget::GetLCD(Json::Value &root)
{
	root["mcu"]["serial"] = "/dev/" + ui->lcd_tty->currentText().toStdString();
	root["mcu"]["baudrate"] = ui->lcd_baud->currentText().toInt();
}

void dtuarmconfigwidget::GetAllBay(Json::Value &root)
{
	root["bay"]["remote"] = interval_info;
}

void dtuarmconfigwidget::GetCurBay()
{
	if (interval_info.empty())
		return;

	if (curBayNo < 0)
		return;

	interval_info[curBayNo]["used"] = ui->interval_check->isChecked();
	interval_info[curBayNo]["asdu"] = ui->interval_addr->currentIndex() + 1;
	interval_info[curBayNo]["protocol"] = protocol[ui->interval_mode->currentIndex()].toStdString();

	interval_info[curBayNo]["RPC"]["ip"] = ui->interval_pri_ip->getIP().toStdString();
	interval_info[curBayNo]["RPC"]["port"] = ui->interval_pri_port->value();

	interval_info[curBayNo]["CS101"]["mode"] = ui->interval_101_mode->currentIndex();
	interval_info[curBayNo]["CS101"]["serial"] = ui->interval_101_tty->currentText().toStdString();
	interval_info[curBayNo]["CS101"]["baudrate"] = ui->interval_101_baud->currentText().toInt();
	interval_info[curBayNo]["CS101"]["databits"] = 8 - ui->interval_101_data->currentIndex();
	interval_info[curBayNo]["CS101"]["parity"] = ui->interval_101_parity->currentIndex();
	interval_info[curBayNo]["CS101"]["stopbits"] = ui->interval_101_stop->currentIndex() + 1;

	interval_info[curBayNo]["CS104"]["ip"] = ui->interval_104_ip->getIP().toStdString();
	interval_info[curBayNo]["CS104"]["port"] = ui->interval_104_port->value();
}

void dtuarmconfigwidget::GetInterval(Json::Value &bay)
{
	//check = ui->interval_check->isChecked();
	//addr = ui->interval_addr->currentIndex();
	//mode = protocol[ui->interval_mode->currentIndex()].toStdString();
}

void dtuarmconfigwidget::GetCommonunit(Json::Value &root)
{
	root["public"]["rpcip"] = ui->com_ip->getIP().toStdString();
	root["public"]["rpcport"] = ui->com_port->value();
}

/******************************* 窗口切换 *******************************************/
void dtuarmconfigwidget::ASDU_select()
{
	switch (ui->asdu_mode->currentIndex())
	{
	// 间隔单元
	case 1: {
		ui->treeWidget->setEnabled(false);
		ui->interval_check->setEnabled(false);
		ui->interval_addr->setEnabled(false);
		ui->interval_mode->setEnabled(false);

		ui->com_ip->setEnabled(true);
		ui->com_port->setEnabled(true);

		interval_connect(false);

	}; break;
	// 公共单元
	case 0: {

		ui->treeWidget->setEnabled(true);
		ui->interval_check->setEnabled(true);

		ui->com_ip->setEnabled(false);
		ui->com_port->setEnabled(false);

		interval_checkf();
	}; break;
	}
}

void dtuarmconfigwidget::Time_check()
{
	bool status = ui->time_check->isChecked();
	ui->time_delay->setEnabled(status);
}

void dtuarmconfigwidget::Line_check()
{
	bool status = ui->line_check->isChecked();
	ui->line_tty->setEnabled(status);
	ui->line_baud->setEnabled(status);
}

void dtuarmconfigwidget::interval_checkf()
{
	if (ui->asdu_mode->currentIndex() == 0)
	{
		bool status = ui->interval_check->isChecked();

		ui->interval_addr->setEnabled(status);
		ui->interval_mode->setEnabled(status);

		if (status)
			ui->interval_point->setText("使用");
		else
			ui->interval_point->setText("停用");

		interval_connect(status);
	}
	else
	{
		ui->interval_addr->setEnabled(false);
		ui->interval_mode->setEnabled(false);
		interval_connect(false);
	}
}

void dtuarmconfigwidget::interval_connect(bool status)
{
	if (ui->interval_check->isChecked())
	{
		switch (ui->interval_mode->currentIndex())
		{
			// 私有协议
		case 0: {
			internal_pri(status);
			internal_101(false);
			internal_104(false);
		}break;
			// 101协议
		case 1: {
			internal_pri(false);
			internal_101(status);
			internal_104(false);
		}break;
			// 104协议
		case 2: {
			internal_pri(false);
			internal_101(false);
			internal_104(status);
		}break;
		}
	}
	else
	{
		internal_pri(false);
		internal_101(false);
		internal_104(false);
	}
}

void dtuarmconfigwidget::internal_pri(bool status)
{
	ui->interval_pri_ip->setEnabled(status);
	ui->interval_pri_port->setEnabled(status);
}

void dtuarmconfigwidget::internal_101(bool status)
{
	ui->interval_101_mode->setEnabled(status);
	ui->interval_101_tty->setEnabled(status);
	ui->interval_101_baud->setEnabled(status);
	ui->interval_101_data->setEnabled(status);
	ui->interval_101_parity->setEnabled(status);
	ui->interval_101_stop->setEnabled(status);
}

void dtuarmconfigwidget::internal_104(bool status)
{
	ui->interval_104_ip->setEnabled(status);
	ui->interval_104_port->setEnabled(status);
}

