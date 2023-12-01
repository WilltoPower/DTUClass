#include "dtugoosecfg.h"
#include "ui_dtugoosecfg.h"
#include <string>
#include "dtutask.h"
#include "dtulog.h"

#include "dtusystemconfig.h"

using namespace DTUCFG;

dtugoosecfg::dtugoosecfg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dtugoosecfg)
{
    ui->setupUi(this);
    // 控件转换槽函数
    connect(ui->checkN1,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
    connect(ui->checkN2,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
    connect(ui->checkN3,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
    connect(ui->checkM1,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
    connect(ui->checkM2,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
    connect(ui->checkM3,SIGNAL(stateChanged(int)),this,SLOT(checkTrans()));
	ui->macLocal->setMac("01:0C:CD:01:01:01");
	updatecfg();
}

dtugoosecfg::~dtugoosecfg()
{
    delete ui;
}

void dtugoosecfg::updatecfg()
{
    DSYSCFG::GooseCFG cfg;

	if (execute_test_arm_connect())
	{
		if (DTU_SUCCESS != execute_read_goose_cfg(cfg))
		{
			QtDTULOG(DTU_ERROR,"update()读取GOOSE配置错误");
			return;
		}

		ui->macLocal->setMac(QString::fromStdString(cfg.mac));
		ui->spin_cur->setValue(cfg.appid);
		ui->com_net->setCurrentIndex(QString::fromStdString(cfg.eth.substr(3, 1)).toInt());

		cfg.eth = ui->com_net->currentText().toStdString();

		// N1
		ui->checkN1->setChecked(cfg.nside[0].use);
		ui->spinN1->setValue(cfg.nside[0].appid);
		// N2
		ui->checkN2->setChecked(cfg.nside[1].use);
		ui->spinN2->setValue(cfg.nside[1].appid);
		// N3
		ui->checkN3->setChecked(cfg.nside[2].use);
		ui->spinN3->setValue(cfg.nside[2].appid);

		// M1
		ui->checkM1->setChecked(cfg.mside[0].use);
		ui->spinM1->setValue(cfg.mside[0].appid);
		// M2
		ui->checkM2->setChecked(cfg.mside[1].use);
		ui->spinM2->setValue(cfg.mside[1].appid);
		// M3
		ui->checkM3->setChecked(cfg.mside[2].use);
		ui->spinM3->setValue(cfg.mside[2].appid);
	}
	else
	{
		QtDTULOG(DTU_WARN,"后台程序未连接,停止读取");
	}
}

void dtugoosecfg::savecfg()
{
	if (!execute_test_arm_connect())
	{
		QtDTULOG(DTU_WARN,"后台程序未连接,停止写入");
		return;
	}

	DSYSCFG::GooseCFG cfg;
    cfg.eth = ui->com_net->currentText().toStdString();
    cfg.mac = ui->macLocal->getMac().toStdString();
    cfg.appid = ui->spin_cur->value();

    // N1
	cfg.nside[0].use = ui->checkN1->isChecked();
    cfg.nside[0].appid = ui->spinN1->value();
    // N2
	cfg.nside[1].use = ui->checkN2->isChecked();
    cfg.nside[1].appid = ui->spinN2->value();
    // N3
	cfg.nside[2].use = ui->checkN3->isChecked();
    cfg.nside[2].appid = ui->spinN3->value();

    // M1
    cfg.mside[0].use = ui->checkM1->isChecked();
    cfg.mside[0].appid = ui->spinM1->value();
    // M2
    cfg.mside[1].use = ui->checkM2->isChecked();
    cfg.mside[1].appid = ui->spinM2->value();
    // M3
    cfg.mside[2].use = ui->checkM3->isChecked();
    cfg.mside[2].appid = ui->spinM3->value();

	if (DTU_SUCCESS != execute_save_goose_cfg(cfg))
	{
		QtDTULOG(DTU_ERROR, "save()读取GOOSE配置错误");
		return;
	}
}

void dtugoosecfg::checkTrans()
{
    QObject* lSender = sender();
    if (lSender == 0)
        return;
    if (lSender->objectName() == "checkN1") {
        checkUITrans(ui->checkN1, nullptr,ui->spinN1,ui->checkN1->isChecked());
    }
    else if (lSender->objectName() == "checkN2") {
        checkUITrans(ui->checkN2, nullptr,ui->spinN2,ui->checkN2->isChecked());
    }
    else if (lSender->objectName() == "checkN3") {
        checkUITrans(ui->checkN3, nullptr,ui->spinN3,ui->checkN3->isChecked());
    }
    else if (lSender->objectName() == "checkM1") {
        checkUITrans(ui->checkM1, nullptr,ui->spinM1,ui->checkM1->isChecked());
    }
    else if (lSender->objectName() == "checkM2") {
        checkUITrans(ui->checkM2, nullptr,ui->spinM2,ui->checkM2->isChecked());
    }
    else if (lSender->objectName() == "checkM3") {
        checkUITrans(ui->checkM3, nullptr,ui->spinM3,ui->checkM3->isChecked());
    }
}

void dtugoosecfg::checkUITrans(QCheckBox *check, MacAddress *macaddr,QSpinBox *box, bool state)
{
    if(state)
        check->setText("启用");
    else
        check->setText("停用");
    box->setEnabled(state);
}

