#pragma execution_character_set("utf-8")

#include "dtulinelossdialog.h"
#include "ui_dtulinelossdialog.h"

#include <QLineEdit>
#include <QStandardItemModel>

#include "dtutask.h"
#include "dtulog.h"
#include "dtusystemconfig.h"

using namespace DTUCFG;

dtuLineLossDialog::dtuLineLossDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuLineLossDialog)
{
    ui->setupUi(this);
	this->load_ui();
	ui->widgetProto->setProtoType(101);
}

dtuLineLossDialog::~dtuLineLossDialog()
{
    delete ui;
}


void dtuLineLossDialog::init()
{
	this->read();
}

void dtuLineLossDialog::read()
{
	if (execute_test_arm_connect())
	{
		DTU::buffer result;
		if (DTU_SUCCESS != execute_query_data(DTU_GET_SYS_CONFIG, result))
		{
			DTULOG(DTU_ERROR, "获取后台配置失败");
			return;
		}

		DSYSCFG::instance().sysunpack(result);
		auto ret = DSYSCFG::instance().GetLineCFG();
		
		ui->checkBox->setChecked(ret.use);
		ui->comMode->setCurrentIndex(ret.proto.mode);
		ui->spinADDR->setValue(ret.proto.otheraddr);

		ui->widgetSerial->unpack(ret.proto.serial);
		DSYSCFG::StationCFG cfg;
		cfg.CS101 = ret.proto;
		ui->widgetProto->unpack(cfg);
	}
}

void dtuLineLossDialog::save()
{
	auto &cfg = DSYSCFG::instance().ModifyLineCFG();

	cfg.proto = ui->widgetProto->pack().CS101;
	cfg.proto.serial = ui->widgetSerial->pack();

	cfg.use = ui->checkBox->isChecked();
	cfg.proto.otheraddr = ui->spinADDR->value();
	cfg.proto.mode = static_cast<LinkLayerMode>(ui->comMode->currentIndex());

	auto packer = DSYSCFG::instance().syspack();

	// 获取文件路径
	std::string execPath;
	if (DTU_FAILED(execute_get_filepath(execPath))) {
		DTULOG(DTU_ERROR, "保存线损模块配置获取路径失败");
		return;
	}
	execPath = execPath + "/update/config/syscfg.json";
	// 传送文件
	if (DTU_SUCCESS != execute_set_file(execPath, packer)) {
		DTULOG(DTU_ERROR, "保存线损模块配置传送失败");
		return;
	}
	// 执行升级
	uint16_t tag = 0x1000;
	execute_updateprogram(tag);
}

void dtuLineLossDialog::load_ui()
{
	this->setLineEdit(ui->comMode);
	connect(ui->checkBox, SIGNAL(stateChanged(int)), this, SLOT(checkChange(int)));
}

void dtuLineLossDialog::checkChange(int state)
{
	QCheckBox *check = (QCheckBox*)sender();
	if (state == Qt::Checked) {
		check->setText("启用");
	}
	else {
		check->setText("停用");
	}
}

void dtuLineLossDialog::setLineEdit(QComboBox *widget)
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