#include "QCreateHelper.h"

#include <windows.h>
#include <regex>

#include <QEvent>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QStandardItem>

#include "dtubuffer.h"
#include "dtucommon.h"
#include "dturmctrlWidget.h"

static QCreateHelper qchelp;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

QWidget *QCreateHelper::createWidget(uint16_t type, QTCTRL desc, bool editable)
{
	switch (type) 
	{
	case WID_SWI: {
		// 创建Checkbox
		return createCheck(std::get<QTCRL_LIST>(desc), (static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str())) == 1));
	}
	case WID_COM: {
		// 创建Combobox
		int defindex = static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str()));
		return createComboBox(QString::fromStdString(std::get<QTCRL_LIST>(desc)), defindex);
	}
	case WID_SPI: {
		// 创建Spinbox
		int def = static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str()));
		return createSpinBox(static_cast<int>(std::get<QTCRL_MINI>(desc)),
			static_cast<int>(std::get<QTCRL_MAXI>(desc)),
			static_cast<int>(std::get<QTCRL_STEP>(desc)), def);
	}
	case WID_DSP: {
		// 创建DSpinbox
		return createDoubleBox(std::get<QTCRL_MINI>(desc),
			std::get<QTCRL_MAXI>(desc),
			std::get<QTCRL_STEP>(desc),
			atof(std::get<QTCRL_DEFA>(desc).c_str()));
	}
	case WID_EDI: {
		// 创建LineEdit
		return createEditLine(QString::fromStdString(std::get<QTCRL_LIST>(desc)).simplified(), editable ? false : true);
	}
	case WID_LAB: {
		// 创建Label
		return createPoint((static_cast<int>(atof(std::get<QTCRL_DEFA>(desc).c_str())) == 1));
	}
	case WID_IPW: {
		return createIPBox();
	}
	case WID_STS: {
		return createStrLabel(QString::fromStdString(std::get<QTCRL_DEFA>(desc)));
	}
	case WID_STD: {
		return createDoubleStrLabel(QString::fromStdString(std::get<QTCRL_DEFA>(desc)));
	}
	case WID_RMC: {
		//return createRmcWidget();
		return nullptr;
	}
	default: {
		break;
	}
	}
	return nullptr;
}

void QCreateHelper::getWidgetValue(uint16_t type, char* value, uint32_t size, QWidget* pWidget, bool extend)
{
	if (!value || size == 0) {
		return;
	}

	switch (type)
	{
	case WID_SWI:{
		auto tempwidget = static_cast<QWidget*>(pWidget);
		QCheckBox *qbox = (QCheckBox*)tempwidget->children().at(1);
		auto ret = static_cast<char>(qbox->isChecked());
		memcpy_s(value, sizeof(char), &ret, sizeof(char));
		break;
	}
	case WID_COM: {
		auto index = static_cast<DTUComboBox*>(pWidget)->currentIndex();
		memcpy_s(value, min(size, sizeof(index)), &index, min(size, sizeof(index)));
		break;
	}
	case WID_SPI: {
		auto v = ((DTUSpinBox*)pWidget)->value();
		memcpy_s(value, min(size, sizeof(v)), &v, min(size, sizeof(v)));
		break;
	}
	case WID_DSP: {
		auto v = (float)((DTUDoubleSpinBox*)pWidget)->value();
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
	case WID_IPW: {
		uint32_t v = IPToInt(static_cast<IPAddress*>(pWidget)->getIP().toStdString());
		memcpy_s(value, min(size, sizeof(v)), &v, min(size, sizeof(v)));
		break;
	}
	case WID_STS:
	case WID_STD:
	{
		// 字符串label什么都不处理
		break;
	}
	default: {
		break;
	}
	}
}

void QCreateHelper::setWidgetValue(uint16_t type, const char* value, uint32_t size, QWidget* pWidget, bool extend)
{
	if (!value || size == 0)
		return;
	
	if (pWidget == nullptr)
		return;

	switch (type) 
	{
	case WID_SWI: {
		char v = 0;
		memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		auto tempwidget = static_cast<QWidget*>(pWidget);
		QCheckBox *qbox = (QCheckBox*)tempwidget->children().at(1);
		qbox->setChecked(static_cast<bool>(v));
		break;
	}
	case WID_COM: {
		uint32_t index = 0;
		memcpy_s(&index, min(size, sizeof(index)), value, min(size, sizeof(index)));
		static_cast<DTUComboBox*>(pWidget)->setCurrentIndex(index);
		break;
	}
	case WID_SPI: {
		uint32_t v = 0;
		memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		((DTUSpinBox*)pWidget)->setValue(v);
		break;
	}
	case WID_DSP: {
		float v = 0;
		if (extend) {
			int iV = 0;
			memcpy_s(&iV, min(sizeof(iV), size), value, min(sizeof(iV), size));
			v = (float)iV / 1000.0f;
		}
		else {
			memcpy_s(&v, min(sizeof(v), size), value, min(sizeof(v), size));
		}

		static_cast<DTUDoubleSpinBox*>(pWidget)->setValue(v);
		break;
	}
	case WID_EDI: {
		auto str = QString::fromStdString(std::string(value, size));
		static_cast<QLineEdit*>(pWidget)->setText(str);
		break;
	}
	case WID_LAB: {
		auto str = QString::fromStdString(std::string(value, size));
		setPoint((QLabel*)pWidget, (str == "1"));
		break;
	}
	case WID_IPW: {
		DTU::buffer data(value, size);
		static_cast<IPAddress*>(pWidget)->setIP(QString::fromStdString(IntToIP(data.value<uint32_t>())));
		break;
	}
	case WID_STS: {
		static_cast<QLabel*>(pWidget)->setText(QString::fromStdString(std::string(value, size)));
		break;
	}
	case WID_STD: {
		DTU::buffer data(value, size);
		static_cast<QLabel*>(pWidget)->setText(QString::number(data.value<float>()));
		break;
	}
	default: {
		break;
	}
	}
}

QWidget *QCreateHelper::createCheck(std::string &desc, bool def)
{
	QCheckBox* pCtrl = new QCheckBox();
	QHBoxLayout *hLayout = new QHBoxLayout();	// 水平布局
	QWidget *widget = new QWidget();			// 返回窗口
	
	pCtrl->setChecked(def);
	pCtrl->setText(QString::fromStdString(desc).replace(";", "/"));

	hLayout->setMargin(0);      // 与窗体边无距离 尽量占满
	hLayout->addWidget(pCtrl, Qt::AlignCenter | Qt::AlignHCenter);     // 加入控件
	hLayout->setAlignment(pCtrl, Qt::AlignCenter);
	widget->setLayout(hLayout);

	return widget;
}

DTUComboBox *QCreateHelper::createComboBox(QStringList &v, int defaultIndex)
{
	auto combox = new DTUComboBox;
	combox->addItems(v);
	combox->setCurrentIndex(defaultIndex);
	combox->setEditable(true);
	combox->lineEdit()->setAlignment(Qt::AlignCenter);
	combox->lineEdit()->setReadOnly(true);
	return combox;
}

DTUComboBox *QCreateHelper::createComboBox(QString str, int defaultIndex)
{
	auto combox = new DTUComboBox;
	QStringList lists = str.split(";");
	combox->addItems(lists);
	combox->setCurrentIndex(defaultIndex);
	combox->setEditable(true);
	combox->lineEdit()->setAlignment(Qt::AlignCenter);
	combox->lineEdit()->setReadOnly(true);
	QStandardItemModel *model = qobject_cast<QStandardItemModel*>(combox->model());
	for (int i = 0; i < model->rowCount(); ++i) {
		QStandardItem *item = model->item(i);
		item->setTextAlignment(Qt::AlignCenter);
	}
	return combox;
}

DTUSpinBox *QCreateHelper::createSpinBox(int min, int max, int step, int def)
{
	DTUSpinBox *spinBox = new DTUSpinBox();
	spinBox->setMaximum(max);
	spinBox->setMinimum(min);
	spinBox->setSingleStep(step);
	spinBox->setValue(def);
	spinBox->setAlignment(Qt::AlignCenter);
	spinBox->installEventFilter(&qchelp);
	return spinBox;
}

DTUDoubleSpinBox *QCreateHelper::createDoubleBox(double min, double max, double step, double def, double des)
{
	DTUDoubleSpinBox *spinBox = new DTUDoubleSpinBox();
	spinBox->setMaximum((max));
	spinBox->setMinimum(min);
	spinBox->setSingleStep(step);
	spinBox->setValue(def);
	spinBox->setDecimals(des);
	spinBox->setAlignment(Qt::AlignCenter);
	return spinBox;
}

QLineEdit *QCreateHelper::createEditLine(QString def, bool readOnly, QRegExp* exp)
{
	QLineEdit *editLine = new QLineEdit;
	editLine->setText(def);
	editLine->setReadOnly(readOnly);
	editLine->setAlignment(Qt::AlignCenter);
	if (exp) {
		editLine->setValidator(new QRegExpValidator(*exp));
	}
	return editLine;
}

QLabel *QCreateHelper::createPoint(bool state)
{
	QLabel *lab = new QLabel;
	setPoint(lab, state);
	lab->setAlignment(Qt::AlignCenter);
	QFont font = lab->font();
	font.setWeight(99);

	lab->setMinimumWidth(55);
	lab->setMaximumWidth(55);

	return lab;
}

IPAddress *QCreateHelper::createIPBox(QString def)
{
	IPAddress *ledit = new IPAddress();
	ledit->setMaximumWidth(10000);
	if (!(def.isEmpty())) {
		ledit->setIP(def);
	}
	return ledit;
}

QLabel *QCreateHelper::createStrLabel(QString str)
{
	auto label = new QLabel;
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setAutoFillBackground(true);
	label->setText(str);
	return label;
}

QLabel *QCreateHelper::createDoubleStrLabel(QString str)
{
	auto label = new QLabel;
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setAutoFillBackground(true);
	label->setText(str);
	return label;
}

dturmctrlWidget *QCreateHelper::createRmcWidget(uint16_t fixid, bool needPre)
{
	auto rmcwidget = new dturmctrlWidget;
	rmcwidget->setfixID(fixid);
	rmcwidget->setNeedPre(needPre);
	return rmcwidget;
}

void QCreateHelper::setPoint(QLabel *lab, bool state)
{
	if (!state) {
		lab->setText("");
		lab->setStyleSheet("background-color: rgb(130, 130, 130);color:white;");
	}
	else {
		lab->setText("");
		lab->setStyleSheet("background-color: rgb(238, 99, 99);color:white;");
	}
}

DTUComboBox::DTUComboBox(QWidget *parent /*= nullptr*/)
	: QComboBox(parent)
{
}

void DTUComboBox::wheelEvent(QWheelEvent *event)
{
	UNREFERENCED_PARAMETER(event);
}

DTUSpinBox::DTUSpinBox(QWidget *parent /*= nullptr*/)
	: QSpinBox(parent)
{
}

void DTUSpinBox::wheelEvent(QWheelEvent *event)
{
	UNREFERENCED_PARAMETER(event);
}

DTUDoubleSpinBox::DTUDoubleSpinBox(QWidget *parent /*= nullptr*/)
	: QDoubleSpinBox(parent)
{
}

void DTUDoubleSpinBox::wheelEvent(QWheelEvent *event)
{
	UNREFERENCED_PARAMETER(event);
}
