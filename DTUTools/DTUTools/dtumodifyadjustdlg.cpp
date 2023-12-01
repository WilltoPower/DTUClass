#include "dtumodifyadjustdlg.h"
#include "ui_dtumodifyadjustdlg.h"

#include <QRegExp>
#include <math.h>

#define QSTR(str) QString::fromLocal8Bit(str)

dtumodifyadjustDlg::dtumodifyadjustDlg(DTUParamAdjust &_adjParam, std::vector<CHN_ANALOG> &m_selectAnalog, bool curMenu, int channel, int column, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtumodifyadjustDlg)
{
    ui->setupUi(this);

    ui->channel_no->setText(QString::number(channel));

	_channel = channel;

	init(_adjParam,m_selectAnalog);

	if (curMenu)
	{
		_use = USE_ADJPARAM;
	}
	else
	{
		_use = USE_SELECTANALOG;
	}

    QStringList list;
    list.clear();
    list << "变比" << "截距" << "零漂" << "角度偏差";
    ui->comboBox_param->addItems(list);
	ui->comboBox_param->setCurrentIndex(column - 2);

	show_infomation(column-2);

	this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
	this->setWindowTitle("修改整定定值");

	// QRegExp rx("^[0-9]+([.]{1}[0-9]+){0,1}$");
	QRegExp rx("^[-]?[0-9]+([.]{1}[0-9]+){0,1}$");

	ui->lineEdit_modify->setValidator(new QRegExpValidator(rx, this));
}

dtumodifyadjustDlg::~dtumodifyadjustDlg()
{
    delete ui;
}

void dtumodifyadjustDlg::modify_adjust()
{
	float modify_value = ui->lineEdit_modify->text().toFloat();
	int index = ui->comboBox_param->currentIndex();
	int BianBi = 0, JieJu = 0, LingPiao = 0, JiaoDuPianCha = 0;

		for (auto &item : m_selectAnalog)
		{
			if (item._chNo == _channel)
			{
				switch (index)
				{
				case 0:item.Kc = modify_value; item.wModify = item.wModify | 0x1000; break;//变比
				case 1:item.X0 = modify_value; item.wModify = item.wModify | 0x0100; break;//截距
				case 2:item.ZeroExcursion = modify_value; item.wModify = item.wModify | 0x0010; break;//零漂
				case 3:item.Ka = modify_value; item.wModify = item.wModify | 0x0001; break;	//角度偏差
				}
				item.isModify = MODIFY_FLAG_FLAG;
				break;
			}
		}

	if (_use == USE_ADJPARAM)
	{
		switch (index)
		{
		case 0:_adjParam.SetValue(_channel, modify_value, ADJUST_RATIO); break;		//变比
		case 1:_adjParam.SetValue(_channel, modify_value, ADJUST_INTERCEPT); break;	//截距
		case 2:_adjParam.SetValue(_channel, modify_value, ADJUST_ZERO); break;		//零漂
		case 3:_adjParam.SetValue(_channel, cos(modify_value), ADJUST_ANGLE_COS);	//角度偏差
			   _adjParam.SetValue(_channel, sin(modify_value), ADJUST_ANGLE_SIN); break;
		}
	}
	this->close();
}

void dtumodifyadjustDlg::show_infomation(int index)
{
	float ret;
	if (_use == USE_SELECTANALOG)
	{
		for (auto &item : m_selectAnalog)
		{
			if (item._chNo == _channel)
			{
				switch (index)
				{
				case 0:ret = item.Kc; break;			//变比
				case 1:ret = item.X0; break;			//截距
				case 2:ret = item.ZeroExcursion; break;	//零漂
				case 3:ret = item.Ka; break;			//角度偏差
				}
				break;
			}
		}
	}
	else if (_use == USE_ADJPARAM)
	{
		switch (index)
		{
		case 0:ret = _adjParam.GetValue(_channel, ADJUST_RATIO); break;				//变比
		case 1:ret = _adjParam.GetValue(_channel, ADJUST_INTERCEPT); break;			//截距
		case 2:ret = _adjParam.GetValue(_channel, ADJUST_ZERO); break;				//零漂
		case 3:ret = acos(_adjParam.GetValue(_channel, ADJUST_ANGLE_COS)); break;	//角度偏差
		}
	}
	QString data = QString("%1").arg(ret);
	ui->lineEdit_modify->setText(data);
}

void dtumodifyadjustDlg::init(DTUParamAdjust &_adjParam, std::vector<CHN_ANALOG> &m_selectAnalog)
{
	this->_adjParam.Adjdata().remove();
	this->_adjParam.Adjdata().append(_adjParam.Adjdata());

	this->m_selectAnalog.clear();
	this->m_selectAnalog.assign(m_selectAnalog.begin(), m_selectAnalog.end());
}

void dtumodifyadjustDlg::get_result(DTUParamAdjust &_adjParam, std::vector<CHN_ANALOG> &m_selectAnalog)
{
	_adjParam.Adjdata().remove();
	_adjParam.Adjdata().append(this->_adjParam.Adjdata());

	m_selectAnalog.clear();
	m_selectAnalog.assign(this->m_selectAnalog.begin(),this->m_selectAnalog.end());
}