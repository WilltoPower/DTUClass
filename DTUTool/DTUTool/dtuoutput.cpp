#pragma execution_character_set("utf-8")

#include "dtuoutput.h"



#include "dtulog.h"
#include "dtutask.h"

// ��ֵ�ͱ��������ݿ��ļ��������

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
		ui->label->setText("ָ����󣬵���ʧ��");
		DTULOG(DTU_ERROR, "ָ�����");
		return;
	}

	if (!execute_test_arm_connect()) {
		ui->label->setText("��̨δ���ӣ�����ʧ��");
		DTULOG(DTU_ERROR, "��̨δ����");
		return;
	}

	ui->progressBar->setValue(0);
	// ��ȡ������ֵ
	adjustwidget->get_adjust_result();
	ui->progressBar->setValue(30);
	// ��Լ��־�ļ�����
	rulewidget->back();
	ui->progressBar->setValue(60);
	// ���ݿ��ȡ
	execute_get_file_plus("/config/dtu.db", get_exec_dir() + "\\config\\dtu.db");
	// ����ARM����
	execute_get_file_plus("/sdl9200", get_exec_dir() + "\\output\\back\\sdl9200.out");
	// ��������
	execute_get_file_plus("/config/syscfg.json", get_exec_dir() + "\\output\\back\\syscfg.json");
	execute_get_file_plus("/config/DTUnetconfig.sh", get_exec_dir() + "\\output\\back\\DTUnetconfig.sh");
	execute_get_file_plus("/config/GooseConfig.xml", get_exec_dir() + "\\output\\back\\GooseConfig.xml");
	execute_get_file_plus("/config/netcfg.json", get_exec_dir() + "\\output\\back\\netcfg.json");
	execute_get_file_plus("/config/csprotocol.json", get_exec_dir() + "\\output\\back\\csprotocol.json");

	ui->progressBar->setValue(100);
	ui->label->setText("������ɣ�������������Ӧ�ı�");
}