#pragma execution_character_set("utf-8")

#include "dtuarmconfigdialog.h"
#include "ui_dtuarmconfigdialog.h"

#include <QStandardItemModel>
#include <QLineEdit>

#include "dtutask.h"
#include "dtulog.h"
#include "dtusystemconfig.h"

using namespace DTUCFG;

dtuarmconfigDialog::dtuarmconfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuarmconfigDialog)
{
    ui->setupUi(this);
    this->load_ui();
}

dtuarmconfigDialog::~dtuarmconfigDialog()
{
    delete ui;
}

void dtuarmconfigDialog::load_ui()
{
    this->setLineEdit(ui->comBay);
    this->setLineEdit(ui->comProtoPublic);
    this->setLineEdit(ui->comMode);
    this->setLineEdit(ui->comProto);

    connect(ui->comMode,SIGNAL(currentIndexChanged(int)),this,SLOT(modeChange(int)));
    connect(ui->comProto,SIGNAL(currentIndexChanged(int)),this,SLOT(protoChange(int)));
    connect(ui->checkBay,SIGNAL(stateChanged(int)),this,SLOT(checkChange(int)));
    connect(ui->checkSync,SIGNAL(stateChanged(int)),this,SLOT(checkChange(int)));
	connect(ui->comBay,SIGNAL(currentIndexChanged(int)),this,SLOT(BayCFGChange(int)));
}

void dtuarmconfigDialog::setLineEdit(QComboBox *widget)
{
    widget->setEditable(true);
    widget->lineEdit()->setAlignment(Qt::AlignCenter);
    widget->lineEdit()->setReadOnly(true);
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(widget->model());
    for (int i = 0; i < model->rowCount(); ++i) {
        QStandardItem *item = model->item(i);
        item->setTextAlignment(Qt::AlignCenter);
    }
}

void dtuarmconfigDialog::modeChange(int index)
{
    ui->stackedWidget1->setCurrentIndex(index);
	if (index == 0) {
		this->BayCFGChange(ui->comBay->currentIndex());
		this->protoChange(ui->comProto->currentIndex());
	}
	else {
		ui->comProtoPublic->setCurrentIndex(2);
		ui->comProtoPublic->setEnabled(false);
		ui->widgetIPPublic->setIP(QString::fromStdString(DSYSCFG::instance().GetPublicCFG().ProtoRPC.ip));
		ui->spinPortPublic->setValue(DSYSCFG::instance().GetPublicCFG().ProtoRPC.port);
		ui->spinASDUPublic->setValue(DSYSCFG::instance().GetPublicCFG().ca);
	}
}

void dtuarmconfigDialog::checkChange(int state)
{
    QCheckBox *check = (QCheckBox*)sender();
    if (state == Qt::Checked) {
        check->setText("启用");
    }
    else {
        check->setText("停用");
    }
}

void dtuarmconfigDialog::protoChange(int index)
{
    ui->stackedWidget2->setCurrentIndex(index);
	this->BayCFGSet(index);
}

void dtuarmconfigDialog::BayCFGChange(int index)
{
	ui->checkBay->setChecked(DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].use);
	ui->spinBay->setValue(DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].ca);
	ui->comProto->setCurrentIndex(DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].proto);

	ui->widgetIPRPC->setIP(
		QString::fromStdString(DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].ProtoRPC.ip));

	ui->spinPortRPC->setValue(DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].ProtoRPC.port);
}

void dtuarmconfigDialog::BayCFGSet(int index)
{
	switch (index) 
	{
	case DSYSCFG::PROTO_CS101: {
		auto cfg = DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].ProroCS.CS101.serial;
		ui->widgetSerial->unpack(cfg);
	}break;
	case DSYSCFG::PROTO_CS104: {
		auto cfg = DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]].ProroCS.CS104.eth;
		ui->spinPort104->setValue(cfg.port);
		ui->widgetIP104->setIP(QString::fromStdString(cfg.ip));
	}break;
	case DSYSCFG::PROTO_CSRPC: {
		auto cfg = DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]+1].ProtoRPC;
		ui->spinPortRPC->setValue(cfg.port);
		ui->widgetIPRPC->setIP(QString::fromStdString(cfg.ip));
	}break;
	}
}

void dtuarmconfigDialog::read()
{
	if (execute_test_arm_connect())
	{
		DTU::buffer result;
		if (DTU_SUCCESS != execute_query_data(DTU_GET_SYS_CONFIG, result))
		{
			DTULOG(DTU_ERROR, "获取后台配置失败");
			return;
		}
		// 解析
		DSYSCFG::instance().sysunpack(result);
		// 设置UI
		ui->comMode->setCurrentIndex(DSYSCFG::instance().GetUnitType() - 1);
		ui->spinPort->setValue(DSYSCFG::instance().GetRPCCFG().port);

		ui->checkSync->setChecked(DSYSCFG::instance().GetSyncCFG().use);
		ui->spinSync->setValue(DSYSCFG::instance().GetSyncCFG().timeInSec);

		if (DSYSCFG::instance().isPublic()) {
			ui->comBay->setCurrentIndex(0);
		}

		// 设置设备号
		ui->comdevno->setCurrentIndex(DSYSCFG::instance().devno);
		// 设置IED名称
		ui->lineEdit_IED->setText(QString::fromStdString(DSYSCFG::instance().iedName));

		// 设置MAC地址
		ui->check_eth0->setChecked(DSYSCFG::instance().MacCFG[0].use);
		ui->check_eth0->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[0].eth));
		ui->lineEdit_mac_0->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[0].mac));

		ui->check_eth1->setChecked(DSYSCFG::instance().MacCFG[1].use);
		ui->check_eth1->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[1].eth));
		ui->lineEdit_mac_1->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[1].mac));

		ui->check_eth2->setChecked(DSYSCFG::instance().MacCFG[2].use);
		ui->check_eth2->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[2].eth));
		ui->lineEdit_mac_2->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[2].mac));

		ui->check_eth3->setChecked(DSYSCFG::instance().MacCFG[3].use);
		ui->check_eth3->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[3].eth));
		ui->lineEdit_mac_3->setText(QString::fromStdString(DSYSCFG::instance().MacCFG[3].mac));

		ui->spin_linkaddr->setValue(DSYSCFG::instance().linkAddr);

		BayMap.clear();
		int index = 0;
		for (auto item : DSYSCFG::instance().ModifyUnitCFG().info)
		{
			BayMap.insert({ index,item.second.ca });
			index++;
		}

		this->modeChange(ui->comMode->currentIndex());
		this->BayCFGChange(ui->comBay->currentIndex());
		this->protoChange(ui->comProto->currentIndex());

	}
}

void dtuarmconfigDialog::save()
{
	// 单元配置
	DSYSCFG::instance().ModifyUnitCFG().type = static_cast<DSYSCFG::UNITTYPE>(ui->comMode->currentIndex() + 1);
	DSYSCFG::instance().ModifyRPCCFG().port = ui->spinPort->value();
	// 自动校时
	DSYSCFG::instance().ModifySyncCFG().use = ui->checkSync->isChecked();
	DSYSCFG::instance().ModifySyncCFG().timeInSec = ui->spinSync->value();
	// 设备配置
	DSYSCFG::instance().devno = ui->comdevno->currentIndex();
	DSYSCFG::instance().iedName = ui->lineEdit_IED->text().toStdString();

	DSYSCFG::instance().linkAddr = ui->spin_linkaddr->value();

	if (ui->comMode->currentIndex() == 0) {
		// 公共单元模式 保存间隔单元信息
		auto &ret = DSYSCFG::instance().ModifyUnitCFG().info[BayMap[ui->comBay->currentIndex()]];
		ret.use = ui->checkBay->isChecked();
		ret.ca = ui->spinBay->value();
		ret.proto = static_cast<DSYSCFG::UNITPROTOCOL>(ui->comProto->currentIndex());
		switch (ui->comProto->currentIndex())
		{
		case 0: {
			ret.ProroCS.CS101.serial = ui->widgetSerial->pack();
		}; break;
		case 1: {
			ret.ProroCS.CS104.eth.ip = ui->widgetIP104->getIP().toStdString();
			ret.ProroCS.CS104.eth.port = ui->spinPort104->value();
		}; break;
		case 2: {
			ret.ProtoRPC.ip = ui->widgetIPRPC->getIP().toStdString();
			ret.ProtoRPC.port = ui->spinPortRPC->value();
		}; break;
		}
	}
	else {
		// 间隔单元模式 保存公共单元信息
		auto &ret = DSYSCFG::instance().ModifyPublicCFG();

		ret.ca = ui->spinASDUPublic->value();

		switch (ui->comProtoPublic->currentIndex())
		{
		case 0: {
		}; break;
		case 1: {
		}; break;
		case 2: {
			ret.ProtoRPC.ip = ui->widgetIPPublic->getIP().toStdString();
			ret.ProtoRPC.port = ui->spinPortPublic->value();
		}; break;
		}
	}

	// 设置MAC地址
	DSYSCFG::instance().MacCFG[0].use = ui->check_eth0->isChecked();
	DSYSCFG::instance().MacCFG[0].eth = ui->check_eth0->text().toStdString();
	DSYSCFG::instance().MacCFG[0].mac = ui->lineEdit_mac_0->text().toStdString();

	DSYSCFG::instance().MacCFG[1].use = ui->check_eth1->isChecked();
	DSYSCFG::instance().MacCFG[1].eth = ui->check_eth1->text().toStdString();
	DSYSCFG::instance().MacCFG[1].mac = ui->lineEdit_mac_1->text().toStdString();

	DSYSCFG::instance().MacCFG[2].use = ui->check_eth2->isChecked();
	DSYSCFG::instance().MacCFG[2].eth = ui->check_eth2->text().toStdString();
	DSYSCFG::instance().MacCFG[2].mac = ui->lineEdit_mac_2->text().toStdString();

	DSYSCFG::instance().MacCFG[3].use = ui->check_eth3->isChecked();
	DSYSCFG::instance().MacCFG[3].eth = ui->check_eth3->text().toStdString();
	DSYSCFG::instance().MacCFG[3].mac = ui->lineEdit_mac_3->text().toStdString();

	auto packer = DSYSCFG::instance().syspack();
	
	// 获取文件路径
	std::string execPath;
	if (DTU_FAILED(execute_get_filepath(execPath))) {
		DTULOG(DTU_ERROR, "保存系统配置获取路径失败");
		return;
	}
	execPath = execPath + "/update/config/syscfg.json";
	// 传送文件
	if (DTU_SUCCESS != execute_set_file(execPath, packer)) {
		DTULOG(DTU_ERROR, "保存系统配置传送失败");
		return;
	}
	// 执行升级
	uint16_t tag = 0x1000;
	execute_updateprogram(tag);
}