#pragma execution_character_set("utf-8")

#include "dtusoftinfodialog.h"
#include "ui_dtusoftinfodialog.h"

#include "quihelper.h"
#include "dtubuffer.h"
#include "dtutask.h"
#include "dtulog.h"
#include "dtuinversiondialog.h"

dtusoftinfoDialog::dtusoftinfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtusoftinfoDialog)
{
    ui->setupUi(this);
	this->load_ui();
	connect(this, SIGNAL(softinfodialogisReady()), this, SLOT(update_version()));
	emit softinfodialogisReady();
}

dtusoftinfoDialog::~dtusoftinfoDialog()
{
    delete ui;
}

void dtusoftinfoDialog::load_ui()
{
	this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
	ui->labTitle->setStyleSheet("QLabel{font:20px;}");
	ui->labTitle->setWordWrap(true);
	// �����ޱ߿�
	QUIHelper::setFramelessForm(this);
	QIcon icon(":/image/LOGO.ico");
	QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(120,120)));
	ui->labIcon->setPixmap(pixmap);
	// ���ñ���ʱ��
	QString dateTime;
	dateTime += __DATE__;
	dateTime += " ";
	dateTime += __TIME__;
	ui->lab_time->setText(dateTime);
	// ��װɸѡ��
	ui->labIcon->installEventFilter(this);
}

void dtusoftinfoDialog::update_version()
{
	if (!execute_test_arm_connect()) {
		DTULOG(DTU_WARN, (char*)"��̨����δ����");
		return;
	}

	DTU::buffer result;
	execute_query_data(PC_R_SOFT_PROG, result);
	if (result.size() != 128)
	{
		DTULOG(DTU_ERROR, (char*)"�汾��Ϣ��������");
		return;
	}
	// �ն�ID
	QString ret;
	ret = QString::fromLocal8Bit(result.query(0, 32), 32);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_id->setText(ret);
	// �ն˳���
	ret = QString::fromLocal8Bit(result.query(32, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_factory->setText(ret);
	// �ն��ͺ�
	ret = QString::fromLocal8Bit(result.query(48, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_model->setText(ret);
	// Ӳ���汾
	ret = QString::fromLocal8Bit(result.query(64, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_hard->setText(ret);
	// ����汾
	ret = QString::fromLocal8Bit(result.query(80, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_soft->setText(ret);
	// CRC
	ret = QString::fromLocal8Bit(result.query(96, 8), 8);
	ret = ret.remove(QRegExp("\\s"));
	ui->label_crc->setText(ret);
}

void dtusoftinfoDialog::btn_double_clicked()
{
	dtuinversionDialog vdialog;
	vdialog.exec();
}

bool dtusoftinfoDialog::eventFilter(QObject * watched, QEvent * event)
{
	if (watched == ui->labIcon)
	{
		if (event->type() == QEvent::MouseButtonDblClick) {
			btn_double_clicked(); //����˫���¼�
		}
	}
	return QWidget::eventFilter(watched, event);
}