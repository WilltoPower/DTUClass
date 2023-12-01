#include "dtuversiondlg.h"
#include "dtutask.h"
#include <dtuprotocol.h>
#include <DTUCmdCode.h>
#include <dtulog.h>
dtuversiondlg::dtuversiondlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	if (execute_test_arm_connect())
	{
		update_version();
	}
}

dtuversiondlg::~dtuversiondlg()
{
}

void dtuversiondlg::update_version()
{
	DTU::buffer result;
	execute_query_data(PC_R_SOFT_PROG, result);
	//
	if (result.size() != 128)
	{
		QtDTULOG(DTU_ERROR,(char*)"版本信息长度有误");
		return;
	}
	// 终端ID
	QString ret;
	ret = QString::fromLocal8Bit(result.query(0, 32), 32);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_ter_id->setText(ret);
	// 终端厂商
	ret = QString::fromLocal8Bit(result.query(32, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_ter_factory->setText(ret);
	// 终端型号
	ret = QString::fromLocal8Bit(result.query(48, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_ter_type->setText(ret);
	// 硬件版本
	ret = QString::fromLocal8Bit(result.query(64, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_hard_ver->setText(ret);
	// 软件版本
	ret = QString::fromLocal8Bit(result.query(80, 16), 16);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_soft_ver->setText(ret);
	// CRC
	ret = QString::fromLocal8Bit(result.query(96, 8), 8);
	ret = ret.remove(QRegExp("\\s"));
	ui.lineEdit_crc->setText(ret);

}
