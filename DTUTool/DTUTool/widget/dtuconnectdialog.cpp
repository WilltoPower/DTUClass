#include "dtuconnectdialog.h"
#include "ui_dtuconnectdialog.h"

#include "dturpcclient.h"
#include "dtuconfigure.h"
#include "dtutask.h"

dtuconnectDialog::dtuconnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuconnectDialog)
{
    ui->setupUi(this);
	this->load_ui();

	connect(&dturpcclient::instance(), SIGNAL(update_status(bool)), this, SLOT(setIcon(bool)));
	//connect(&dturpcclient::instance(), SIGNAL(update_GOOSE(QByteArray)), this, SLOT(goose_status(QByteArray)));
	connect(this, SIGNAL(connect_change()), &dturpcclient::instance(), SLOT(change_net_param()));
}

dtuconnectDialog::~dtuconnectDialog()
{
    delete ui;
}

bool dtuconnectDialog::eventFilter(QObject *target, QEvent *event)
{
	//屏蔽鼠标滚轮事件
	if (event->type() == QEvent::Wheel && (target->inherits("QComboBox") || target->inherits("QSpinBox"))) {
		return true;
	}
	return false;
}

void dtuconnectDialog::load_ui()
{
	this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
	//this->setFixedSize(this->width(), this->height());// 设置固定窗口大小
	// 控件设置
	issetbutton = false;
	this->setIcon(false);
}

void dtuconnectDialog::setButton(QToolButton* btn)
{
	this->btnmainconnect = btn;
	issetbutton = true;
}

void dtuconnectDialog::setConnectInfo()
{
	auto &cfg = Get_RPC_CFG();
	ui->lineEdit->setText(QString::fromStdString(cfg.ip));
	ui->spinport->setValue(cfg.port);
}

void dtuconnectDialog::setIcon(bool state)
{
	static QIcon iconLink(":/image/link.png");
	static QIcon iconunLink(":/image/unlink.png");

	if (state)
	{
		set_arm_connect_state(true);
		ui->btnconnect->setIcon(iconLink);
		if(issetbutton)
			this->btnmainconnect->setIcon(iconLink);
	}
	else
	{
		set_arm_connect_state(false);
		ui->btnconnect->setIcon(iconunLink);
		if(issetbutton)
			this->btnmainconnect->setIcon(iconunLink);
	}
	
}

void dtuconnectDialog::click_connect_button()
{
	auto &mod = dtutoolcfg::instance().ModifyRPCCFG();
	mod.ip = ui->lineEdit->text().toStdString();
	mod.port = ui->spinport->value();
	dtutoolcfg::instance().save();
	emit connect_change();
}