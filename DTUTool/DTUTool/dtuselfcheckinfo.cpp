#pragma execution_character_set("utf-8")

#include "dtuselfcheckinfo.h"

#include <QLabel>

static std::map<int, std::string> selfcheck = {
{0	,"�����Ȳ����쳣"},
{1	,"���湫�������쳣"},
{2	,"������ѹ�嶨ֵ�쳣"},
{3	,"���泣�汣�������쳣"},
{4	,"�����Զ��غ�բ�����쳣"},
{5	,"����͵�FA��ֵ�쳣"},
{6	,"����ֲ�ʽFA��ֵ�쳣"},
{7	,"���汣ͬ�ں�բ�����쳣"},
{8	,"�����Զ����ж�ֵ�쳣"},
{9	,"����С�����ӵض�ֵ�쳣"},
{10	,"������·���߸澯�����쳣"},
{11	,"���洫�����ز����쳣"},
{12	,"�����Զ��������쳣"},
{13	,"�����豸�����쳣"},
{14	,"���漫ֵ�����쳣"},
{15	,"����LED״̬�����쳣"},
{16	,"����EEPROM��ӦISR�쳣"},
{32	,"��ȡ��Ȳ����쳣"},
{33	,"��ȡ���������쳣"},
{34	,"��ȡ��ѹ�嶨ֵ�쳣"},
{35	,"��ȡ���汣�������쳣"},
{36	,"��ȡ�Զ��غ�բ�����쳣"},
{37	,"��ȡ�͵�FA��ֵ�쳣"},
{38	,"��ȡ�ֲ�ʽFA��ֵ�쳣"},
{39	,"��ȡ��ͬ�ں�բ�����쳣"},
{40	,"��ȡ�Զ����ж�ֵ�쳣"},
{41	,"��ȡС�����ӵض�ֵ�쳣"},
{42	,"��ȡ��·���߸澯������"},
{43	,"��ȡ�������ز����쳣"},
{44	,"��ȡ�Զ��������쳣"},
{45	,"��ȡ�豸�����쳣"},
{46	,"��ȡ��ֵ�����쳣"},
{47	,"��ȡLED״̬�����쳣"},
{48	,"��ȡEEPROM��ӦISR�쳣"},
{64	,"IO��չ1�쳣"},
{65	,"IO��չ2�쳣"},
{66	,"860��ʱ����ͨ���쳣"},
{67	,"8100��ʱ����ͨ���쳣"},
{68	,"I2C3д���ж��쳣"},
{69	,"���յ�860�����־�����쳣"},
{70	,"8101��ȡ�����ڴ��ȡ�쳣-860����ռ��"},
{71	,"8101���������8100�쳣"},
{72	,"���յ�goose���ݰ��쳣"},
{73	,"�����������쳣"},
{96	,"����վͨ���쳣"},
{97	,"���Դģ��ͨ���쳣"},
{98	,"����ʾģ��ͨ���쳣"},
{99	,"�빫����Ԫͨ���쳣"},
{100,"������Ԫ1ͨ���쳣"},
{101,"������Ԫ2ͨ���쳣"},
{102,"������Ԫ3ͨ���쳣"},
{103,"������Ԫ4ͨ���쳣"},
{104,"������Ԫ5ͨ���쳣"},
{105,"������Ԫ6ͨ���쳣"},
{106,"GOOSE����"},
{128,"װ��������־"},
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
	// ���ر���
	QStringList header;
	header << "���" << "���" << "��Ϣ";

	// ��������
	ui->tableWidget->setColumnCount(header.size());
	ui->tableWidget->setRowCount(this->info.count());
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
			label1->setText("���" + QString::number(nCount));
			label1->setAlignment(Qt::AlignCenter);
			ui->tableWidget->setCellWidget(row, 0, label1);

			QLabel *label2 = new QLabel;
			label2->setText("���" + QString::number(i));
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
				label3->setText("δ֪�Լ����");
				label3->setAlignment(Qt::AlignCenter);
				ui->tableWidget->setCellWidget(row, 2, label3);
			}
			row++;
			nCount++;
		}
	}
}