#pragma execution_character_set("utf-8")

#include "dtuselfcheckinfo.h"

#include <QLabel>

static std::map<int, std::string> selfcheck = {
{0	,"保存变比参数异常"},
{1	,"保存公共参数异常"},
{2	,"保存软压板定值异常"},
{3	,"保存常规保护参数异常"},
{4	,"保存自动重合闸参数异常"},
{5	,"保存就地FA定值异常"},
{6	,"保存分布式FA定值异常"},
{7	,"保存保同期合闸参数异常"},
{8	,"保存自动解列定值异常"},
{9	,"保存小电流接地定值异常"},
{10	,"保存线路断线告警参数异常"},
{11	,"保存传动开关参数异常"},
{12	,"保存自动化参数异常"},
{13	,"保存设备参数异常"},
{14	,"保存极值数据异常"},
{15	,"保存LED状态参数异常"},
{16	,"保存EEPROM对应ISR异常"},
{32	,"读取变比参数异常"},
{33	,"读取公共参数异常"},
{34	,"读取软压板定值异常"},
{35	,"读取常规保护参数异常"},
{36	,"读取自动重合闸参数异常"},
{37	,"读取就地FA定值异常"},
{38	,"读取分布式FA定值异常"},
{39	,"读取保同期合闸参数异常"},
{40	,"读取自动解列定值异常"},
{41	,"读取小电流接地定值异常"},
{42	,"读取线路断线告警参数异"},
{43	,"读取传动开关参数异常"},
{44	,"读取自动化参数异常"},
{45	,"读取设备参数异常"},
{46	,"读取极值数据异常"},
{47	,"读取LED状态参数异常"},
{48	,"读取EEPROM对应ISR异常"},
{64	,"IO扩展1异常"},
{65	,"IO扩展2异常"},
{66	,"860长时间无通信异常"},
{67	,"8100长时间无通信异常"},
{68	,"I2C3写入中断异常"},
{69	,"接收的860邮箱标志内容异常"},
{70	,"8101读取共享内存获取异常-860长期占用"},
{71	,"8101保存参数到8100异常"},
{72	,"接收的goose数据包异常"},
{73	,"检测采样数据异常"},
{96	,"与主站通信异常"},
{97	,"与电源模块通信异常"},
{98	,"与显示模块通信异常"},
{99	,"与公共单元通信异常"},
{100,"与间隔单元1通信异常"},
{101,"与间隔单元2通信异常"},
{102,"与间隔单元3通信异常"},
{103,"与间隔单元4通信异常"},
{104,"与间隔单元5通信异常"},
{105,"与间隔单元6通信异常"},
{106,"GOOSE断链"},
{128,"装置启动标志"},
};

dtuselfcheckinfo::dtuselfcheckinfo(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuselfcheckinfoClass())
{
	ui->setupUi(this);
}

dtuselfcheckinfo::~dtuselfcheckinfo()
{
	delete ui;
}

void dtuselfcheckinfo::load_ui()
{
	// 加载标题
	QStringList header;
	header << "序号" << "编号" << "信息";

	// 设置列数
	ui->tableWidget->setColumnCount(header.size());
	ui->tableWidget->setRowCount(this->info.count());
	// 设置表头
	ui->tableWidget->setHorizontalHeaderLabels(header);

	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);// 自动延展
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置为不可修改

	ui->tableWidget->verticalHeader()->setVisible(false);    // 隐藏垂直表头
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//先自适应宽度
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//然后设置要根据内容使用宽度的列

	// 设置整行选中
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	// 单次只能选中一个
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void dtuselfcheckinfo::setinfo(const std::bitset<192>& data)
{
	this->info = data;

	load_ui();

	int row = 0;
	int nCount = 1;
	for (int i= this->info.size()-1;i>0;i--)
	{
		if (info[i]) {
			auto ita = selfcheck.find(i);

			QLabel *label1 = new QLabel;
			label1->setText("序号" + QString::number(nCount));
			label1->setAlignment(Qt::AlignCenter);
			ui->tableWidget->setCellWidget(row, 0, label1);

			QLabel *label2 = new QLabel;
			label2->setText("编号" + QString::number(i));
			label2->setAlignment(Qt::AlignCenter);
			ui->tableWidget->setCellWidget(row, 1, label2);

			if (ita != selfcheck.end()) {
				QLabel *label3 = new QLabel;
				label3->setText(QString::fromStdString(ita->second));
				label3->setAlignment(Qt::AlignCenter);
				ui->tableWidget->setCellWidget(row, 2, label3);
			}
			else {
				QLabel *label3 = new QLabel;
				label3->setText("未知自检错误");
				label3->setAlignment(Qt::AlignCenter);
				ui->tableWidget->setCellWidget(row, 2, label3);
			}
			row++;
			nCount++;
		}
	}
}