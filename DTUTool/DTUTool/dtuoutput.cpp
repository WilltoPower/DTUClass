#pragma execution_character_set("utf-8")

#include "dtuoutput.h"



#include "dtulog.h"
#include "dtutask.h"

// 定值和报告以数据库文件传输过来

dtuoutput::dtuoutput(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuoutputClass())
{
	ui->setupUi(this);
	ui->progressBar->setValue(0);
}

dtuoutput::~dtuoutput()
{
	delete ui;
}

void dtuoutput::setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2)
{
	adjustwidget = widget1;
	rulewidget = widget2;
}

void dtuoutput::back()
{
	if (adjustwidget == nullptr || rulewidget == nullptr) {
		ui->label->setText("指针错误，导出失败");
		DTULOG(DTU_ERROR, "指针错误");
		return;
	}

	if (!execute_test_arm_connect()) {
		ui->label->setText("后台未连接，导出失败");
		DTULOG(DTU_ERROR, "后台未连接");
		return;
	}

	ui->progressBar->setValue(0);
	// 读取整定定值
	adjustwidget->get_adjust_result();
	ui->progressBar->setValue(30);
	// 规约日志文件备份
	rulewidget->back();
	ui->progressBar->setValue(60);
	// 数据库读取
	execute_get_file_plus("/config/dtu.db", get_exec_dir() + "\\config\\dtu.db");
	// 导出ARM程序
	execute_get_file_plus("/sdl9200", get_exec_dir() + "\\output\\back\\sdl9200.out");
	// 导出配置
	execute_get_file_plus("/config/syscfg.json", get_exec_dir() + "\\output\\back\\syscfg.json");
	execute_get_file_plus("/config/DTUnetconfig.sh", get_exec_dir() + "\\output\\back\\DTUnetconfig.sh");
	execute_get_file_plus("/config/GooseConfig.xml", get_exec_dir() + "\\output\\back\\GooseConfig.xml");
	execute_get_file_plus("/config/netcfg.json", get_exec_dir() + "\\output\\back\\netcfg.json");
	execute_get_file_plus("/config/csprotocol.json", get_exec_dir() + "\\output\\back\\csprotocol.json");

	ui->progressBar->setValue(100);
	ui->label->setText("导出完成，重启程序以适应改变");
}