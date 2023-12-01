#include "dtuonefixmodify.h"

#include <QRegExp>
#include <QMessageBox>

#include "dtutask.h"
#include "dtulog.h"

using namespace DTU;

dtuonefixmodify::dtuonefixmodify(int index, uint16_t fixid, QString desc, QString add, 
	QTableWidgetItem *item,QWidget *parent)
	: QWidget(parent), ui(new Ui::dtuonefixmodifyClass())
{
	ui->setupUi(this);
	// 设置正则表达式
	setRegx();
	ui->lab_desc->setText(desc);
	ui->lab_adddesc->setText(add);
	ui->lineEdit_hex->setText("0x" + QString("%1").arg(fixid, 2, 16, QLatin1Char('0')).toUpper());
	ui->lineEdit_dec->setText(QString::number(fixid));

	item_ptr = item;

	this->index = index;

	connect(ui->lineEdit_new_hex, SIGNAL(editingFinished()), this, SLOT(HEXin()));
	connect(ui->lineEdit_new_dec, SIGNAL(editingFinished()), this, SLOT(DECin()));
}

dtuonefixmodify::~dtuonefixmodify()
{
	delete ui;
}

void dtuonefixmodify::setRegx()
{
	QRegExp regx1("[A-Fa-fxX0-9]{3,6}");
	QValidator *validator1 = new QRegExpValidator(regx1, ui->lineEdit_new_hex);
	ui->lineEdit_new_hex->setValidator(validator1);
	QRegExp regx2("[0-9]{1,5}");
	QValidator *validator2 = new QRegExpValidator(regx2, ui->lineEdit_new_dec);
	ui->lineEdit_new_dec->setValidator(validator2);
}

void dtuonefixmodify::HEXin()
{
	if (ui->lineEdit_new_hex->text().isEmpty())
		return;
	bool ok;
	ui->lineEdit_new_dec->setText(QString::number(ui->lineEdit_new_hex->text().toInt(&ok, 16)));
}

void dtuonefixmodify::DECin()
{
	if (ui->lineEdit_new_dec->text().isEmpty())
		return;
	int no = ui->lineEdit_new_dec->text().toInt();
	if (no > 0xFFFF) {
		no = 0xFFFF;
	}
	ui->lineEdit_new_hex->setText("0x" +
			QString("%1").arg(no, 4, 16, QLatin1Char('0')).toUpper());
}

void dtuonefixmodify::checkid()
{
	// 检查点表值是否正确
	bool ok;
	uint16_t newer = ui->lineEdit_new_hex->text().toInt(&ok, 16);
	if (!execute_fixno_check(static_cast<MapFixno>(index), newer)) {
		QMessageBox::critical(this, "检验点表", "点表不合法",
			QMessageBox::Ok);
	}
	else {
		QMessageBox::information(this, "检验点表", "可以使用此点表",
			QMessageBox::Ok);
	}
}

void dtuonefixmodify::modifyfixid()
{
	if (ui->lineEdit_new_dec->text().isEmpty() && ui->lineEdit_new_hex->text().isEmpty())
		return;
	if (QMessageBox::Ok == QMessageBox::question(this, "却认修改点表", "是否确认修改点表",
		QMessageBox::Ok | QMessageBox::Cancel))
	{
		bool ok;
		uint16_t older = ui->lineEdit_dec->text().toInt();
		uint16_t newer = ui->lineEdit_new_hex->text().toInt(&ok, 16);
		printf("SET mod old %u new %u index %d\n",older , newer, index);
		if (!execute_fixno_modify(static_cast<MapFixno>(index), older, newer)) {
			QtDTULOG(DTU_ERROR,"修改点表失败");
			QMessageBox::critical(this, "修改点表", "修改失败",
				QMessageBox::Ok);
		}
		else {
			QMessageBox::information(this, "修改点表", "修改成功",
				QMessageBox::Ok);
			item_ptr->setText("0x" +
				QString("%1").arg(newer, 4, 16, QLatin1Char('0')).toUpper());
		}
	}
}