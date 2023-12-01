#pragma execution_character_set("utf-8")
#include "dtutestapiwidgetsclass.h"

#include "dtutask.h"

dtuTestAPIWidgetsClass::dtuTestAPIWidgetsClass(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuTestAPIWidgetsClassClass())
{
	ui->setupUi(this);
}

dtuTestAPIWidgetsClass::~dtuTestAPIWidgetsClass()
{
	delete ui;
}


void dtuTestAPIWidgetsClass::TESTAPI()
{
	std::string dest = "C:\\Users\\51570\\Desktop\\TTT.exe";
	std::string src = "/update/DTUTools.exe";
	execute_get_file_plus(src, dest);
	ui->lab1->setText("传输完成");
}