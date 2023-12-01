#pragma execution_character_set("utf-8")

#include "dtuinversiondialog.h"
#include "ui_dtuinversiondialog.h"

#include "quihelper.h"
#include "dtutask.h"
#include "dtulog.h"

dtuinversionDialog::dtuinversionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuinversionDialog)
{
    ui->setupUi(this);
	this->load_ui();
	connect(this,SIGNAL(inversionDialogReady()),this,SLOT(update_version()));
	emit inversionDialogReady();
}

dtuinversionDialog::~dtuinversionDialog()
{
    delete ui;
}

void dtuinversionDialog::load_ui()
{
	QUIHelper::setFramelessForm(this);
}

void dtuinversionDialog::addOneLabel(QLabel *lab, DTU::buffer &data)
{
	lab->setText(QString("%1%2-%3-%4").arg(data.get(3, 1).value<uint8_t>(), 2, 10, QLatin1Char('0'))
					.arg(data.get(2, 1).value<uint8_t>(), 2, 10, QLatin1Char('0'))
					.arg(data.get(1, 1).value<uint8_t>(), 2, 10, QLatin1Char('0'))
					.arg(data.get(0, 1).value<uint8_t>(), 2, 10, QLatin1Char('0')));
}

void dtuinversionDialog::loadOneData(QLabel *lab, DTU::buffer &data, int &offset)
{
	auto ret1 = data.get(sizeof(uint32_t) * offset, sizeof(uint32_t));
	this->addOneLabel(lab, ret1);
	offset++;
}

void dtuinversionDialog::update_version()
{
	if (!execute_test_arm_connect()) {
		DTULOG(DTU_WARN,"后台程序未连接");
	}

	DTU::buffer result;
	if (DTU_SUCCESS != execute_query_data(PC_R_PRI_PRO_INFO, result)) {
		DTULOG(DTU_ERROR, "获取内部版本错误");
		return;
	}

	if (result.size() != 32) {
		DTULOG(DTU_ERROR, "错误的内部版本长度");
		return;
	}

	int count = 0;
	this->loadOneData(ui->lab_ck8100, result, count);
	this->loadOneData(ui->lab_ck8101, result, count);
	this->loadOneData(ui->lab_ck860, result, count);
	this->loadOneData(ui->lab_lcd, result, count);
	this->loadOneData(ui->lab_kernel, result, count);
	this->loadOneData(ui->lab_spl, result, count);
	this->loadOneData(ui->lab_uboot, result, count);
}