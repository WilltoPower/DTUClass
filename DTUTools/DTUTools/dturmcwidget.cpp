#include "dturmcwidget.h"

#include <QLabel>

#include "dtulog.h"
#include "dtutask.h"
#include "create_control.h"

using namespace DTU;

dturmcwidget::dturmcwidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dturmcwidgetClass())
{
	ui->setupUi(this);
	load_ui();
}

dturmcwidget::~dturmcwidget()
{
	delete ui;
}

void dturmcwidget::load_ui()
{
	auto rmtable = DBManager::instance().GetRmctrlTable();

	QStringList header;
	header << "序号" << "点表值" << "描述" << "操作";
	
	createTabWidget(ui->tableWidget ,rmtable.size(), header);

	int index = 0;
	for (const auto &item : rmtable)
	{
		ui->tableWidget->setCellWidget(index, 0, createStrLabel(QString::number(index + 1)));
		ui->tableWidget->setCellWidget(index, 1, createStrLabel("0x" + QString("%1").arg(item.second.fixid, 2, 16, QLatin1Char('0')).toUpper()));
		ui->tableWidget->setCellWidget(index, 2, createStrLabel(QString::fromStdString(item.second.desc)));
		auto rmcwidget = createRmcWidget(item.second.fixid);
		rmcwidget->setDelayWidget(ui->spinBox);
		connect(rmcwidget, SIGNAL(execCmd(int, uint16_t)), this, SLOT(rmctrlhanlder(int, uint16_t)));
		ui->tableWidget->setCellWidget(index, 3, rmcwidget);

		index++;
	}
}

// 创建TableWidget控件
QTableWidget *dturmcwidget::createTabWidget(QTableWidget *ptable, int row, QStringList header)
{
	// 设置行数
	ptable->setRowCount(row);
	// 设置列数
	ptable->setColumnCount(header.size());
	// 设置表头
	ptable->setHorizontalHeaderLabels(header);

	// ui修改
	// 自动延展
	ptable->horizontalHeader()->setStretchLastSection(true);
	//设置为不可修改
	ptable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	// 隐藏水平表头
	//ptable->horizontalHeader()->setVisible(false);
	// 隐藏垂直表头
	ptable->verticalHeader()->setVisible(false);
	// 先自适应宽度
	ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	// 然后设置要根据内容使用宽度的列
	ptable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	return ptable;
}

void dturmcwidget::rmctrlhanlder(int opt, uint16_t fixid)
{
	if (execute_test_arm_connect())
	{
		int ret = DTU_SUCCESS;
		switch (opt)
		{
		case 0: { // 预设
			ret = execute_rmctrl(fixid, RC_CMD_PRE, ui->spinBox->value(), RC_CMD_TOOL);
		}break;
		case 1: { // 执行
			ret = execute_rmctrl(fixid, RC_CMD_EXE, ui->spinBox->value(), RC_CMD_TOOL);
		}break;
		case 2: { // 取消
			ret = execute_rmctrl(fixid, RC_CMD_CAN, ui->spinBox->value(), RC_CMD_TOOL);
		}break;
		}
		if (ret != DTU_SUCCESS)
			QtDTULOG(DTU_ERROR, "下发遥控命令错误");
	}
}