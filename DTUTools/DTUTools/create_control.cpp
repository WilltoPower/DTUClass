#include "create_control.h"

#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <windows.h>
#include <regex>
#include <dtubuffer.h>
#include "dtucommon.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

std::vector<std::string> Split(const std::string& src, const std::string& separator)
{
	std::regex re(separator);
	std::sregex_token_iterator
		first{ src.begin(), src.end(), re, -1 },
		last;
	return { first, last };
}

QCheckBox* createSwitch(std::string desc,bool def)
{
	QCheckBox* pCtrl = new QCheckBox();
	pCtrl->setChecked(def);
	pCtrl->setText(QString::fromStdString(desc).replace(";","/"));
	return pCtrl;
}

QComboBox* createComBox(QString str, int defindex)
{
	auto combox = new QComboBox;
	QStringList lists = str.split(";");
	combox->addItems(lists);
	combox->setCurrentIndex(defindex);
	combox->setEditable(true);
	combox->lineEdit()->setAlignment(Qt::AlignCenter);
	combox->lineEdit()->setReadOnly(true);
	return combox;
}

QComboBox* createComBox(QStringList v, int defindex)
{
	auto combox = new QComboBox;
	combox->addItems(v);
	combox->setCurrentIndex(defindex);
	combox->setEditable(true);
	combox->lineEdit()->setAlignment(Qt::AlignCenter);
	combox->lineEdit()->setReadOnly(true);
	return combox;
}

QSpinBox* createSpinBox(int min, int max, int step, int def)
{
	auto spinBox = new SDLPCTSpinBox();
	spinBox->setMaximum(max);
	spinBox->setMinimum(min);
	spinBox->setSingleStep(step);
	spinBox->setValue(def);
	spinBox->setAlignment(Qt::AlignCenter);
	return spinBox;
}

QDoubleSpinBox* createDoubleBox(double min, double max, double step, double def, double des)
{
	auto spinBox = new SDLPCTDoubleSpinBox();
	spinBox->setMaximum((max));
	spinBox->setMinimum(min);
	spinBox->setSingleStep(step);
	spinBox->setValue(def);
	spinBox->setDecimals(des);
	spinBox->setAlignment(Qt::AlignCenter);
	return dynamic_cast<SDLPCTDoubleSpinBox*>(spinBox);
}

QLineEdit* createEditLine(QString def /*= ""*/, bool readOnly, QRegExp* exp /*= nullptr*/)
{
	auto editLine = new QLineEdit;
	editLine->setText(def);
	editLine->setReadOnly(readOnly);
	editLine->setAlignment(Qt::AlignCenter);
	if (exp)
	{
		editLine->setValidator(new QRegExpValidator(*exp));
	}
	return editLine;
}

QListWidget* createList(QStringList v)
{
	QListWidget *pList = new QListWidget;

	for (auto i = 0; i < v.size(); i++)
	{
		QListWidgetItem* pItem = new QListWidgetItem;
		pList->setItemWidget(pItem, createEditLine(v[i]));
		pList->addItem(pItem);
	}
	return pList;
}

QLabel* createLabel(bool str)
{
	auto label = new QLabel;
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setAutoFillBackground(true);
	if (str)
	{
		label->setText("1");
		label->setStyleSheet("background-color: rgb(255, 0, 0);");
	}
	else
	{
		label->setText("0");
		label->setStyleSheet("background-color: rgb(37, 127, 28);");
	}
	return label;
}

IPAddress* createIPWidget()
{
	auto ipw = new IPAddress;
	ipw->setIP(QString::fromLocal8Bit("0.0.0.0"));
	return ipw;
}

dturmctrlWidget* createRmcWidget(uint16_t fixid, bool needPre)
{
	auto rmcwidget = new dturmctrlWidget;
	rmcwidget->setfixID(fixid);
	rmcwidget->setNeedPre(needPre);
	return rmcwidget;
}

QLabel* createStrLabel(QString str)
{
	auto label = new QLabel;
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setAutoFillBackground(true);
	label->setText("   " + str + "   ");
	return label;
}

QLabel* createDoubleStrLabel(QString str)
{
	auto label = new QLabel;
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setAutoFillBackground(true);
	label->setText("   " + str + "   ");
	return label;
}

QWidget* createControl(uint16_t type, QTCTRL desc,bool editable)
{
	switch (type) {
	case SWITCH:
	{
		// 创建Checkbox
		return createSwitch(std::get<QTCRL_LIST>(desc),(static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str())) == 1));
	}
	case COMBO:
	{
		// 创建Combobox
		int defindex = static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str()));
		return createComBox(QString::fromStdString(std::get<QTCRL_LIST>(desc)), defindex);
	}
	case SPIN:
	{
		// 创建Spinbox
		int def = static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str()));
		return createSpinBox(static_cast<int>(std::get<QTCRL_MINI>(desc)),
								static_cast<int>(std::get<QTCRL_MAXI>(desc)),
								static_cast<int>(std::get<QTCRL_STEP>(desc)), def);
	}
	case DSPIN:
	{
		// 创建DSpinbox
		return createDoubleBox(std::get<QTCRL_MINI>(desc), 
								std::get<QTCRL_MAXI>(desc), 
								std::get<QTCRL_STEP>(desc),
								atof(std::get<QTCRL_DEFA>(desc).c_str()));
	}
	case EDIT:
	{
		// 创建LineEdit
		return createEditLine(QString::fromStdString(std::get<QTCRL_LIST>(desc)).simplified(), editable?false:true);
	}
	case LABLE:
	{
		// 创建Label
		return createLabel((static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str())) == 1));
	}
	case IPWIDGET:
	{
		return createIPWidget();
	}
	case STRLABEL:
	{
		return createStrLabel(QString::fromStdString(std::get<QTCRL_DEFA>(desc)));
	}
	case STRDLABEL:
	{
		return createDoubleStrLabel(QString::fromStdString(std::get<QTCRL_DEFA>(desc)));
	}
	case RMCWIDGET:
	{
		return createRmcWidget();
	}
	default: {
		break;
	}
	}
	return nullptr;
}

void getControlValue(uint16_t type, char* value, uint32_t size, QWidget* pWidget, bool extend)
{
	if (!value || size == 0) {
		return;
	}
	switch (type) {
	case SWITCH:
	{
		auto ret = static_cast<char>(static_cast<QCheckBox*>(pWidget)->isChecked());
		memcpy_s(value, sizeof(char), &ret, sizeof(char));
		break;
	}
	case COMBO:
	{
		auto index = static_cast<QComboBox*>(pWidget)->currentIndex();
		memcpy_s(value, min(size,sizeof(index)), &index, min(size,sizeof(index)));
		break;
	}
	case SPIN:
	{
		auto v = ((SDLPCTSpinBox*)pWidget)->value();
		memcpy_s(value, min(size, sizeof(v)), &v, min(size, sizeof(v)));
		break;
	}
	case DSPIN:
	{
		auto v = (float)((SDLPCTDoubleSpinBox*)pWidget)->value();
		if (extend) {
			// 扩大1000下发
			int iV = (int)(v * 1000);
			memcpy_s(value, min(size, sizeof(iV)), &iV, min(size, sizeof(iV)));
		}
		else {
			memcpy_s(value, min(size, sizeof(v)), &v, min(size, sizeof(v)));
		}

		break;
	}
	case IPWIDGET:
	{
		uint32_t v = IPToInt(static_cast<IPAddress*>(pWidget)->getIP().toStdString());
		memcpy_s(value, min(size,sizeof(v)), &v, min(size, sizeof(v)));
		break;
	}
	case STRLABEL:
	case STRDLABEL:
	{
		// 字符串label什么都不处理
		break;
	}
	default: {
		break;
	}
	}
}

void setControlValue(uint16_t type, const char* value, uint32_t size, QWidget* pWidget,bool extend)
{
	if (!value || size == 0) {
		return;
	}
	if (pWidget == nullptr)
		return;
	switch (type) {
	case SWITCH:
	{
		char v = 0;
		memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		static_cast<QCheckBox*>(pWidget)->setChecked(static_cast<bool>(v));
		break;
	}
	case COMBO:
	{
		uint32_t index = 0;
		memcpy_s(&index, min(size, sizeof(index)), value, min(size, sizeof(index)));
		static_cast<QComboBox*>(pWidget)->setCurrentIndex(index);
		break;
	}
	case SPIN:
	{
		uint32_t v = 0;
		memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		((SDLPCTSpinBox*)pWidget)->setValue(v);
		break;
	}
	case DSPIN:
	{
		float v = 0;
		if (extend) {
			int iV = 0;
			memcpy_s(&iV, min(sizeof(iV), size), value, min(sizeof(iV), size));
			v = (float)iV / 1000.0f;
		}
		else {
			memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		}

		static_cast<SDLPCTDoubleSpinBox*>(pWidget)->setValue(v);
		break;
	}
	case EDIT:
	{
		auto str = QString::fromStdString(std::string(value,size));
		static_cast<QLineEdit*>(pWidget)->setText(str);
		break;
	}
	case LABLE:
	{
		auto str = QString::fromStdString(std::string(value, size));
		((QLabel*)pWidget)->setText(str);
		if (str == "1")
			static_cast<QLabel*>(pWidget)->setStyleSheet("background-color: rgb(255, 0, 0);");
		else
			static_cast<QLabel*>(pWidget)->setStyleSheet("background-color: rgb(37, 127, 28);");
		break;
	}
	case IPWIDGET:
	{
		DTU::buffer data(value,size);
		static_cast<IPAddress*>(pWidget)->setIP(QString::fromStdString(IntToIP(data.value<uint32_t>())));
		break;
	}
	case STRLABEL:
	{
		static_cast<QLabel*>(pWidget)->setText(QString::fromStdString(std::string(value, size)));
		break;
	}
	case STRDLABEL:
	{
		DTU::buffer data(value,size);
		static_cast<QLabel*>(pWidget)->setText(QString::number(data.value<float>()));
		break;
	}
	default: {
		break;
	}
	}
}

quint32 IPV4StringToInteger(const QString& ip)
{
	QStringList ips = ip.split(".");
	if (ips.size() == 4) {
		return ips.at(0).toInt()
			| ips.at(1).toInt() << 8
			| ips.at(2).toInt() << 16
			| ips.at(3).toInt() << 24;
	}
	return 0;
}

QString IPV4IntegerToString(quint32 ip)
{
	return QString("%1.%2.%3.%4")
		.arg(ip & 0xFF)
		.arg((ip >> 8) & 0xFF)
		.arg((ip >> 16) & 0xFF)
		.arg((ip >> 24) & 0xFF);
}

SDLPCTSpinBox::SDLPCTSpinBox(QWidget *parent /*= nullptr*/)
	:QSpinBox(parent)
{

}

void SDLPCTSpinBox::wheelEvent(QWheelEvent *event)
{
	UNREFERENCED_PARAMETER(event);
}

SDLPCTDoubleSpinBox::SDLPCTDoubleSpinBox(QWidget *parent /*= nullptr*/)
	:QDoubleSpinBox(parent)
{

}

void SDLPCTDoubleSpinBox::wheelEvent(QWheelEvent *event)
{
	UNREFERENCED_PARAMETER(event);
}
