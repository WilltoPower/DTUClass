#pragma execution_character_set("utf-8")

#include "dturulefilewidget.h"

#include <QStandardItemModel>
#include <QLineEdit>
#include <QMessageBox>
#include <fstream>

#include "dtutask.h"
#include "dtulog.h"

dturulefilewidget::dturulefilewidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dturulefilewidgetClass())
{
	ui->setupUi(this);
	load_ui();
}

dturulefilewidget::~dturulefilewidget()
{
	delete ui;
}

void dturulefilewidget::load_first()
{
	static bool isfirst = true;
	if (!isfirst)
		return;
	if (execute_test_arm_connect()) {
		get_arm_file_lists("/HISTORY/SOE/");
		isfirst = false;
	}
}

void dturulefilewidget::back()
{
	// 遥控记录
	ui->comboBox->setCurrentIndex(1);
	showfile(0, 1);
	// 日志
	ui->comboBox->setCurrentIndex(2);
	showfile(0, 1);
	// 自检
	ui->comboBox->setCurrentIndex(5);
	showfile(0, 1);
	// SOE记录
	ui->comboBox->setCurrentIndex(0);
	showfile(0, 1);
}

void dturulefilewidget::load_ui()
{
	QStringList header = load_header();

	// 设置列数
	ui->tableWidget->setColumnCount(header.size());
	// 设置表头
	ui->tableWidget->setHorizontalHeaderLabels(header);

	// ui修改
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);// 自动延展
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置为不可修改
	//ptable->horizontalHeader()->setVisible(false);// 隐藏水平表头
	ui->tableWidget->verticalHeader()->setVisible(false);    // 隐藏垂直表头
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//先自适应宽度
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//然后设置要根据内容使用宽度的列
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// 设置整行选中
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	// 单次只能选中一个
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	ui->btn_parse->setEnabled(false);
	ui->comboBox->setEditable(true);
	ui->comboBox->lineEdit()->setAlignment(Qt::AlignCenter);
	ui->comboBox->lineEdit()->setReadOnly(true);
	QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->comboBox->model());
	for (int i = 0; i < model->rowCount(); ++i) {
		QStandardItem *item = model->item(i);
		item->setTextAlignment(Qt::AlignCenter);
	}


}

QStringList dturulefilewidget::load_header()
{
	QStringList header;
	header << "序号" << "文件";
	return header;
}

std::string dturulefilewidget::indextoLinuxPath()
{
	std::string path;
	switch (ui->comboBox->currentIndex())
	{
	case 0:path = "/HISTORY/SOE/"; break;
	case 1:path = "/HISTORY/CO/"; break;
	case 2:path = "/HISTORY/ULOG/"; break;
	case 3:path = "/HISTORY/FIXPT/"; break;
	case 4:path = "/HISTORY/EXV/"; break;
	case 5:path = "/protect/selfcheck/"; break;
	default:
		DTULOG(DTU_ERROR,"未知的索引号,无法映射Linux路径");break;
	}
	return path;
}

std::string dturulefilewidget::indextoWindowsPath()
{
	std::string path;
	switch (ui->comboBox->currentIndex())
	{
	case 0:path = "\\HISTORY\\SOE\\"; break;
	case 1:path = "\\HISTORY\\CO\\"; break;
	case 2:path = "\\HISTORY\\ULOG\\"; break;
	case 3:path = "\\HISTORY\\FIXPT\\"; break;
	case 4:path = "\\HISTORY\\EXV\\"; break;
	case 5:path = "\\protect\\selfcheck\\"; break;
	default:
		DTULOG(DTU_ERROR, "未知的索引号,无法映射Win路径"); break;
	}
	return path;
}

void addOneItem(QTableWidget *table, int index, int no, QString desc)
{
	table->setItem(index, no, new QTableWidgetItem(desc));
	table->item(index, no)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void dturulefilewidget::updatelist()
{
	if (ui->comboBox->currentIndex() == 5)
		ui->btn_parse->setEnabled(true);
	else
		ui->btn_parse->setEnabled(false);

	if (!execute_test_arm_connect()) {
		FILELIST filelist;
		get_dir_files(get_exec_dir() + indextoWindowsPath(), filelist);

		ui->tableWidget->clearContents();
		ui->tableWidget->setRowCount(filelist.size());

		for (auto i = 0; i < filelist.size(); i++) {
			QString fileName = std::get<0>(filelist[i]).c_str();
			addOneItem(ui->tableWidget, i, 0, QString::number(i + 1));
			addOneItem(ui->tableWidget, i, 1, fileName);
		}
		return;
	}
		
	get_arm_file_lists(indextoLinuxPath());
}

void dturulefilewidget::showfile(int row,int column)
{
	QString fileName = ui->tableWidget->item(row, 1)->text();
	get_arm_file(fileName.toStdString());
}

void dturulefilewidget::get_arm_file_lists(std::string path)
{
	// 更新列表
	FILELIST result;
	if (DTU_SUCCESS != execute_get_dir(path,0,0,result)) {
		DTULOG(DTU_ERROR, (char*)"获取定点文件目录失败");
		return;
	}

	ui->tableWidget->clearContents();
	ui->tableWidget->setRowCount(result.size());

	for (auto i = 0; i < result.size(); i++) {
		QString fileName = std::get<0>(result[i]).c_str();
		addOneItem(ui->tableWidget, i, 0, QString::number(i+1));
		addOneItem(ui->tableWidget, i, 1, fileName);
	}
}

void dturulefilewidget::get_arm_file(std::string filename)
{
	std::string file = get_exec_dir() + indextoWindowsPath() + filename;
	DTU::buffer filecontent;
	if(execute_test_arm_connect())
	{
		// ARM连接 直接刷新同名文件
		if (DTU_SUCCESS != execute_get_file(indextoLinuxPath() + filename, filecontent))
		{
			QMessageBox::information(this, "错误", "无法获取文件", QMessageBox::Ok);
			return;
		}
		// 保存文件
		std::ofstream ofs;
		ofs.open(file, std::ios::binary | std::ios::ate | std::ios::out);
		ofs.write(filecontent.const_data(), filecontent.size());
		ofs.close();
		std::string fileContent(filecontent.const_data(),filecontent.size());
		ui->textBrowser->setText(QString::fromStdString(fileContent));
	}
	else
	{
		// ARM未连接 存在文件则直接打开文件
		std::ifstream t(file);
		if (t.good()) {
			std::stringstream buffer;
			buffer << t.rdbuf();
			t.close();
			ui->textBrowser->setText(buffer.str().c_str());
		}
		else {
			DTULOG(DTU_ERROR, (char*)"ARM未连接无法打开文件:%s", file.c_str());
		}
	}
}

void dturulefilewidget::parse_file()
{
	if (!widget_ptr)
		widget_ptr = new dtuselfcheckprase();

	widget_ptr->show();
	widget_ptr->load_file(get_exec_dir() + "\\protect\\selfcheck\\selfcheckLog.xml");
}