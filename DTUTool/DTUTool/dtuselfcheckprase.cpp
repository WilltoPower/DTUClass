#pragma execution_character_set("utf-8")

#include "dtuselfcheckprase.h"

#include <Qbytearray>
#include <QString>
#include <QMessageBox>

#include <string>
#include <map>

#define ONE_PAGE_NUM 20

#include "dtulog.h"

using namespace pugi;

dtuselfcheckprase::dtuselfcheckprase(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuselfcheckpraseClass())
{
	ui->setupUi(this);
	load_ui();
}

dtuselfcheckprase::~dtuselfcheckprase()
{
	delete ui;
}

void dtuselfcheckprase::load_ui()
{
	// ���ر���
	QStringList header;
	header << "���" << "ʱ��" << "��Ϣ";

	// ��������
	ui->tableWidget->setColumnCount(header.size());
	ui->tableWidget->setRowCount(ONE_PAGE_NUM);
	// ���ñ�ͷ
	ui->tableWidget->setHorizontalHeaderLabels(header);

	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);// �Զ���չ
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//����Ϊ�����޸�

	ui->tableWidget->verticalHeader()->setVisible(false);    // ���ش�ֱ��ͷ
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//������Ӧ���
	ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//Ȼ������Ҫ��������ʹ�ÿ�ȵ���

	// ��������ѡ��
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	// ����ֻ��ѡ��һ��
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	ui->progressBar->setValue(0);

	connect(this, SIGNAL(updateprocessvalue(int)), this, SLOT(updateProcess(int)));
	connect(this, SIGNAL(setLabelColor(int, int, bool)),this, SLOT(setColor(int, int, bool)));
	connect(ui->spinBox, SIGNAL(valueChanged(int)), this ,SLOT(updatePage(int)));
	connect(this, SIGNAL(selfcheckpraseover()), this, SLOT(initover()));

	for (int row=0;row< ONE_PAGE_NUM;row++)
	{
		for (int column = 0; column < 3; column++)
		{
			QLabel *label = new QLabel;
			label->setAlignment(Qt::AlignCenter);
			labelmap[row][column] = label;
			ui->tableWidget->setCellWidget(row, column, label);
		}
	}

	
}

void dtuselfcheckprase::load_file(const std::string& fullPath)
{
	findThread = new std::thread([&] {
		auto ret = this->doc.load_file(fullPath.c_str());

		if (ret.status != status_ok) {
			DTULOG(DTU_ERROR, "�޷����ļ�%s", fullPath.c_str());
			return;
		}

		root = doc.child("selfcheckLog");

		int num = root.attribute("num").as_int();

		if ((num % ONE_PAGE_NUM) > 0)
			ui->spinBox->setMaximum((num / ONE_PAGE_NUM) + 1);
		else
			ui->spinBox->setMaximum(num / ONE_PAGE_NUM);

		for (auto &item : root)
		{
			int no = item.attribute("no").as_int();

			std::string all;
			std::string temp;

			temp = item.attribute("a6").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));
			temp = item.attribute("a5").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));
			temp = item.attribute("a4").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));
			temp = item.attribute("a3").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));
			temp = item.attribute("a2").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));
			temp = item.attribute("a1").as_string();
			temp.erase(temp.begin(), temp.begin() + 2);
			all.append(strHexToBin(temp));

			info.insert({no, allselfcheckinfo(item.attribute("tm").as_string(), all)});

			float dd = (float)no / (float)num * 100;

			emit updateprocessvalue((int)dd);
		}
		int a = 0;
		int b = a;
		updatePage(ui->spinBox->value());
		emit selfcheckpraseover();
	});
}

void dtuselfcheckprase::updateProcess(int value)
{
	ui->progressBar->setValue(value);
}

void dtuselfcheckprase::updatePage(int pageno)
{
	// page�Ŵ�1��ʼ nCount��0��ʼ
	for (int nCount = ((pageno - 1) * ONE_PAGE_NUM); nCount < ((pageno - 1) * ONE_PAGE_NUM) + ONE_PAGE_NUM; nCount++)
	{
		if (nCount + 1 > info.size()) {
			labelmap[nCount%ONE_PAGE_NUM][0]->setText("");
			labelmap[nCount%ONE_PAGE_NUM][1]->setText("");
			labelmap[nCount%ONE_PAGE_NUM][2]->setText("");

			emit setLabelColor(nCount%ONE_PAGE_NUM, 2, false);
		}
		else {
			labelmap[nCount%ONE_PAGE_NUM][0]->setText(QString::number(nCount + 1));
			labelmap[nCount%ONE_PAGE_NUM][1]->setText(QString::fromStdString(info[nCount + 1].tm));
			labelmap[nCount%ONE_PAGE_NUM][2]->setText(QString(info[nCount+1].state.any() ? "�����쳣" : "���쳣"));

			emit setLabelColor(nCount%ONE_PAGE_NUM, 2, info[nCount+1].state.any());
		}


	}
}

std::string dtuselfcheckprase::strHexToBin(const std::string& hexstr)
{
	bool ok = false;
	QString hexStr = QString::fromStdString(hexstr);
	int val = hexStr.toInt(&ok, 16);
	QString binStr = hexStr.setNum(val, 2);
	QString add0Str = QString("00000000000000000000000000000000");
	if (hexStr.length() < 32)
		binStr = add0Str.left(32 - binStr.length()) + binStr;
	return binStr.toStdString();
}

void dtuselfcheckprase::setColor(int row, int column, bool state)
{
	if (!state)
		labelmap[row][column]->setStyleSheet("background-color: rgb(0, 139, 0)");// ��ɫ
	else
		labelmap[row][column]->setStyleSheet("background-color: rgb(165, 42 ,42)");// ��ɫ
}

void dtuselfcheckprase::initover()
{
	connect(ui->tableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleclick(QModelIndex)));
}

void dtuselfcheckprase::doubleclick(QModelIndex index)
{
	auto ita = labelmap.find(index.row());
	
	if (ita == labelmap.end())
		return;
	
	if (!prasewidget) {
		prasewidget = new dtuselfcheckinfo;
	}

	auto ret = ((QLabel*)(ui->tableWidget->cellWidget(index.row(), 0)))->text();

	if (!ret.isEmpty()) {
		prasewidget->show();
		int sno = ((QLabel*)(ui->tableWidget->cellWidget(index.row(), 0)))->text().toInt();
		prasewidget->setinfo(info[sno].state);
	}
	

}