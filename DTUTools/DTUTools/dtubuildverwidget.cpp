#include "dtubuildverwidget.h"
//#pragma execution_character_set("UTF-8") // 防止乱码

#include <QStringList>
#include <dtutask.h>
#include <dtucmdcode.h>
#include <dtulog.h>

#define ROWCOUNT 8

dtubuildverwidget::dtubuildverwidget(QWidget *parent) : QWidget(parent),
ui(new Ui::dtubuildverwidget)
{
	ui->setupUi(this);
	load_ui();
	if (execute_test_arm_connect()) {
		GetVersion();
	}
}

dtubuildverwidget::~dtubuildverwidget() {}

void dtubuildverwidget::load_ui()
{
	ui->tableWidget->setRowCount(ROWCOUNT);
	ui->tableWidget->setColumnCount(5);

	// 隐藏表头
	ui->tableWidget->verticalHeader()->hide();
	ui->tableWidget->horizontalHeader()->hide();

	QStringList nameList;
	nameList << " 8100版本 " << " 8101版本 " << " 860版本 "
		<< " LCD版本 " << " Kernel版本 " << " SPL版本 " << " Uboot版本 " << " 保留 ";

	for (int i = 0; i < ROWCOUNT; i++)
	{
		QLabel *label = new QLabel(this);
		QLineEdit *editV = new QLineEdit(this);
		QLineEdit *editR = new QLineEdit(this);
		QLineEdit *editC = new QLineEdit(this);
		QLineEdit *editB = new QLineEdit(this);

		editV->setReadOnly(true);
		editV->setAlignment(Qt::AlignHCenter);
		editR->setReadOnly(true);
		editR->setAlignment(Qt::AlignHCenter);
		editC->setReadOnly(true);
		editC->setAlignment(Qt::AlignHCenter);
		editB->setReadOnly(true);
		editB->setAlignment(Qt::AlignHCenter);

		label->setText(nameList[i]);

		label->setMaximumWidth(80);
		editV->setMaximumWidth(50);
		editR->setMaximumWidth(50);
		editC->setMaximumWidth(50);
		editB->setMaximumWidth(50);

		ui->tableWidget->setCellWidget(i, 0, label);
		ui->tableWidget->setCellWidget(i, 1, editV);
		ui->tableWidget->setCellWidget(i, 2, editR);
		ui->tableWidget->setCellWidget(i, 3, editC);
		ui->tableWidget->setCellWidget(i, 4, editB);

		//ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
		//ui->tableWidget->verticalHeader()->setStretchLastSection(true);
	}

	ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);// 列宽自适应
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);// 列宽自适应
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);// 列宽自适应
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);// 列宽自适应
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);// 列宽自适应

	ui->tableWidget->setMaximumWidth(50*5+80);
}

void dtubuildverwidget::addOneBlock(uint8_t index, uint8_t data, int &no)
{
	QWidget *widget = ui->tableWidget->cellWidget(index, no);
	QLineEdit *linedit = (QLineEdit*)widget;
	linedit->setText(QString::number(data));
}

void dtubuildverwidget::addVersion(uint8_t index,DTU::buffer &data)
{
	int offset = 0;

	for (int i = 1; i <= 4; i++)
	{
		addOneBlock(index, data.get(3 - offset, 1).value<uint8_t>(),i);
		offset++;
	}
}

void dtubuildverwidget::GetVersion()
{
	DTU::buffer result;
	if (DTU_SUCCESS != execute_query_data(PC_R_PRI_PRO_INFO, result))
	{
		DTULOG(DTU_ERROR,"dtubuildverwidget::GetVersion() Get build Version Error");
		return;
	}

	if (result.size() != 32)
	{
		DTULOG(DTU_ERROR, "dtubuildverwidget::GetVersion() Get build Version Error Length");
		return;
	}

	for (int i = 0;i<ROWCOUNT;i++)
	{
		auto ret = result.get(sizeof(uint32_t)*i, sizeof(uint32_t));
		addVersion(i, ret);
	}
}