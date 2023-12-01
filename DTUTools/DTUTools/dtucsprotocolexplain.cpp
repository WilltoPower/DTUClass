#include "dtucsprotocolexplain.h"

dtucsprotocolexplain::dtucsprotocolexplain(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtucsprotocolexplainClass())
{
	ui->setupUi(this);
}

dtucsprotocolexplain::~dtucsprotocolexplain()
{
	delete ui;
}

void dtucsprotocolexplain::setText(QString &text,QString time, QString proto, QString dir, QString size)
{
	ui->textBrowser->setStyleSheet("font-size : 22px");
	ui->textBrowser->setText(text);
	ui->lab_time->setText(time);
	ui->lab_proto->setText(proto);
	ui->lab_dir->setText(dir);
	ui->lab_size->setText(size);
}