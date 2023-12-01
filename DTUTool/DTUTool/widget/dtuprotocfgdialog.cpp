#pragma execution_character_set("utf-8")

#include "dtuprotocfgdialog.h"
#include "ui_dtuprotocfgdialog.h"

#include <QStandardItemModel>
#include <QLineEdit>

#include "dtutask.h"
#include "dtubuffer.h"
#include "dtulog.h"
#include "dtusystemconfig.h"

using namespace DTUCFG;

dtuprotocfgDialog::dtuprotocfgDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuprotocfgDialog)
{
    ui->setupUi(this);
	this->load_ui();
}

dtuprotocfgDialog::~dtuprotocfgDialog()
{
    delete ui;
}

void dtuprotocfgDialog::load_ui()
{
	ui->comMode->setEditable(true);
	ui->comMode->lineEdit()->setAlignment(Qt::AlignCenter);
	ui->comMode->lineEdit()->setReadOnly(true);
	QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->comMode->model());
	for (int i = 0; i < model->rowCount(); ++i) {
		QStandardItem *item = model->item(i);
		item->setTextAlignment(Qt::AlignCenter);
	}
	connect(ui->check104, SIGNAL(stateChanged(int)), this, SLOT(change_check(int)));
	connect(ui->check101, SIGNAL(stateChanged(int)), this, SLOT(change_check(int)));
}

void dtuprotocfgDialog::change_check(int state)
{
	QCheckBox *check = (QCheckBox*)sender();
	if (state == Qt::Checked) {
		check->setText("启用");
	}
	else {
		check->setText("停用");
	}

}

void dtuprotocfgDialog::init()
{
	this->read_config();
}

void dtuprotocfgDialog::read_config()
{
	if (!execute_test_arm_connect())
		return;

	DTU::buffer result;
	if (DTU_SUCCESS == execute_query_data(DTU_GET_PROTO_CONFIG, result)) {
		if (DSYSCFG::instance().csunpack(result)) {
			
			auto CFG = DSYSCFG::instance().Get_IEC60870_Slave_CFG();
			auto CFG104 = CFG.CS104;
			auto CFG101 = CFG.CS101;

			ui->spinADDR->setValue(DSYSCFG::instance().ASDU());

			// 设置104配置
			ui->check104->setChecked(CFG104.use);
			ui->widgetIP->setIP(QString::fromStdString(CFG104.eth.ip));
			ui->spinPort->setValue(CFG104.eth.port);
			ui->widget104Usual->setProtoType(104);
			ui->widget104Usual->unpack(CFG);

			// 设置101配置
			ui->check101->setChecked(CFG101.use);
			ui->comMode->setCurrentIndex(CFG101.mode);
			ui->widgetSerial->unpack(CFG101.serial);
			ui->spinLinkADDR->setValue(CFG101.otheraddr);
			ui->widget101Usual->setProtoType(101);
			ui->widget101Usual->unpack(CFG);
		}
	}
}

void dtuprotocfgDialog::save_config()
{
	if (!execute_test_arm_connect())
		return;

	auto &ret = DSYSCFG::instance().Modify_IEC60870_Slave_CFG();

	DSYSCFG::instance().ASDU(ui->spinADDR->value());

	// 保存104配置
	ret.CS104 = ui->widget104Usual->pack().CS104;
	ret.CS104.use = ui->check104->isChecked();
	ret.CS104.eth.ip = ui->widgetIP->getIP().toStdString();
	ret.CS104.eth.port = ui->spinPort->value();

	// 保存101配置
	ret.CS101 = ui->widget101Usual->pack().CS101;
	ret.CS101.serial = ui->widgetSerial->pack();
	ret.CS101.use = ui->check101->isChecked();
	ret.CS101.mode = static_cast<LinkLayerMode>(ui->comMode->currentIndex());
	ret.CS101.otheraddr = ui->spinLinkADDR->value();

	auto packer = DSYSCFG::instance().cspack();

	// 获取文件路径
	std::string execPath;
	if (DTU_FAILED(execute_get_filepath(execPath))) {
		DTULOG(DTU_ERROR, "保存规约配置获取路径失败");
		return;
	}
	execPath = execPath + "/update/config/csprotocol.json";
	// 传送文件
	if (DTU_SUCCESS != execute_set_file(execPath, packer)) {
		DTULOG(DTU_ERROR, "保存规约配置传送失败");
		return;
	}
	// 执行升级
	uint16_t tag = 0x1000;
	execute_updateprogram(tag);
}