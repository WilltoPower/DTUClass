#include "dtusysconfigwidget.h"
#include "dtusysconfig.h"
#include "create_control.h"
#include "dtutask.h"
#include <QMessageBox>
#include <dtulog.h>
#include <dtucmdcode.h>
#include <dtuerror.h>
#include <dtuparamconfig.h>
#include <dtustorage.h>
#include "dturpcclient.h"

DTUSysConfigWidget::DTUSysConfigWidget(QWidget *parent)
	:_bStop(false), QWidget(parent)
{
	ui.setupUi(this);

	load_ui();
	connect(&dturpcclient::instance(), SIGNAL(update_status(bool)), this, SLOT(connect_status(bool)));
	connect(&dturpcclient::instance(), SIGNAL(update_GOOSE(QByteArray)), this, SLOT(goose_status(QByteArray)));
}

DTUSysConfigWidget::~DTUSysConfigWidget()
{
	_bStop = true;
}

void DTUSysConfigWidget::load_ui()
{
	// 磁盘容量初始
	ui.progressBar_diskusage->setFormat(QString("磁盘使用:%1%").arg(QString::number(0, 'f', 1)));
	ui.progressBar_diskusage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // 对齐方式

	// RPC界面加载
	//QRegExp regexp1("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
	ui.widgetIP->setIP(QString::fromLocal8Bit(DESTIP().c_str()));
	ui.spinBox_remote_port->setValue(DESTPORT());
	ui.spinBox_remote_port->setAlignment(Qt::AlignCenter);
	connect(this, SIGNAL(connect_change()), &dturpcclient::instance(), SLOT(change_net_param()));
}

void DTUSysConfigWidget::connect_status(bool status)
{
	ui.lab_status->setAutoFillBackground(true);
	if (status) {
		set_arm_connect_state(true);
		ui.lab_status->setStyleSheet("background-color: rgb(37, 127, 28);");
		ui.lab_status->setText("已连接");
		get_disk_usage();
	}
	else {
		set_arm_connect_state(false);
		ui.lab_status->setStyleSheet("background-color: rgb(255, 55, 55);");
		ui.lab_status->setText("未连接");
	}
}

void DTUSysConfigWidget::goose_status(QByteArray status)
{
	ui.lab_status_goose->setAutoFillBackground(true);

	DTU::buffer result(status.data(), status.size());

	uint16_t cmd = result.get(0, sizeof(uint16_t)).value<uint16_t>();
	int ret = result.get(2, sizeof(int)).value<int>();

	if (cmd == PC_W_GOOSE_COMM_RST && ret == DTU_SUCCESS) {
		ui.lab_status_goose->setStyleSheet("background-color: rgb(37, 127, 28);");
		ui.lab_status_goose->setText("GOOSE连接");
	}
	else {
		ui.lab_status_goose->setStyleSheet("background-color: rgb(255, 55, 55);");
		ui.lab_status_goose->setText("GOOSE断链");
	}
}

void DTUSysConfigWidget::get_disk_usage()
{
	if (!execute_test_arm_connect())
		return;
	Disk_info info;
	if (execute_get_disksuage(info, 1) != DTU_SUCCESS)
	{
		QtDTULOG(DTU_ERROR, (char*)"读取磁盘用量错误");
		return;
	}
	if (info._used < 70)
	{
		ui.progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(135,206,250); }");
	}
	else if (info._used >= 70 && info._used < 90)
	{
		ui.progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,215,0); }");
	}
	else if(info._used >= 90 && info._used <100)
	{
		ui.progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,69,0); }");
	}
	else if(info._used >=100)
	{
		info._used = 100;
		ui.progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,69,0); }");
	}

	ui.progressBar_diskusage->setFormat(QString("磁盘使用:%1%").arg(QString::number(info._used, 'f', 1)));
	ui.progressBar_diskusage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // 对齐方式
	ui.progressBar_diskusage->setValue(info._used);
	ui.lab_disk_usage->setText(QString::fromStdString(info._used_s) + "/" + QString::fromStdString(info._size_s));
}

void DTUSysConfigWidget::connect_dev()
{
	dtusysconfig::instance().set_dest_ip(ui.widgetIP->getIP().toStdString());
	dtusysconfig::instance().set_dest_port(ui.spinBox_remote_port->value());
	dtusysconfig::instance().save();
	emit connect_change();
}