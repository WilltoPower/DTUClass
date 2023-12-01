#pragma execution_character_set("utf-8")

#include "dtuarminfowidget.h"
#include "ui_dtuarminfowidget.h"

#include "dtutask.h"
#include "dtulog.h"

dtuarminfoWidget::dtuarminfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dtuarminfoWidget)
{
    ui->setupUi(this);
	dtuoutput_ptr = new dtuoutput;
}

dtuarminfoWidget::~dtuarminfoWidget()
{
    delete ui;
}

void dtuarminfoWidget::get_disk_usage()
{
	static bool first = true;
	if ((!execute_test_arm_connect()) && !first) {
		first = false;
		return;
	}

	Disk_info info;
	if (execute_get_disksuage(info, 1) != DTU_SUCCESS) {
		DTULOG(DTU_ERROR, (char*)"读取磁盘用量错误");
		return;
	}

	if (info._used < 70) {
		ui->progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(135,206,250); }");
	}
	else if (info._used >= 70 && info._used < 90) {
		ui->progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,215,0); }");
	}
	else if (info._used >= 90 && info._used < 100) {
		ui->progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,69,0); }");
	}
	else if (info._used >= 100) {
		info._used = 100;
		ui->progressBar_diskusage->setStyleSheet("QProgressBar::chunk{ background:rgb(255,69,0); }");
	}

	ui->progressBar_diskusage->setFormat(QString("磁盘使用:%1%").arg(QString::number(info._used, 'f', 1)));
	ui->progressBar_diskusage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // 对齐方式
	ui->progressBar_diskusage->setValue(info._used);
	ui->lab_disk_usage->setText(QString::fromStdString(info._used_s) + "/" + QString::fromStdString(info._size_s));
}

void dtuarminfoWidget::backonece()
{
	if (dtuoutput_ptr) {
		dtuoutput_ptr->show();
		dtuoutput_ptr->back();
	}
}

void dtuarminfoWidget::setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2)
{
	//ui->btn_back
	if (dtuoutput_ptr) {
		dtuoutput_ptr->setWidget(widget1, widget2);
	}
	else {
		dtuoutput_ptr = new dtuoutput;
		dtuoutput_ptr->setWidget(widget1, widget2);
	}
}