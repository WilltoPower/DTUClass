#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

#include <tuple>
#include <string>

#include "dtuipaddress.h"

class QLabel;
class QCheckBox;
class QLineEdit;
class dturmctrlWidget;

enum WIDGET_NO {
	WID_SWI = 0,  // checkbox
	WID_COM = 1,  // combobox
	WID_SPI = 2,  // 整数spinbox
	WID_DSP = 3,  // 浮点数doublespinbox
	WID_EDI = 4,  // editline
	WID_LAB = 5,  // lable 只显示1/0
	WID_IPW = 6,  // IP地址控件
	WID_STS = 7,  // 字符串label
	WID_STD = 8,  // 浮点数label
	WID_RMC = 9,  // 遥控控件
};

#define  QTCRL_DESC 0  // 描述
#define  QTCRL_MINI 1  // 最小值
#define  QTCRL_MAXI 2  // 最大值
#define  QTCRL_DEFA 3  // 默认值
#define  QTCRL_STEP 4  // 步长
#define	 QTCRL_LIST 5  // 列表

using CTRLATTR = std::tuple<double, double, std::string, bool, std::string, double, std::string>;

using QTCTRL = std::tuple<std::string, double, double, std::string, double, std::string>;

class DTUComboBox : public QComboBox {
	Q_OBJECT
public:
	explicit DTUComboBox(QWidget *parent = nullptr);
private:
	void wheelEvent(QWheelEvent *event);
};

class DTUSpinBox : public QSpinBox {
	Q_OBJECT
public:
	explicit DTUSpinBox(QWidget *parent = nullptr);
protected:
	void wheelEvent(QWheelEvent *event);
};

class DTUDoubleSpinBox : public QDoubleSpinBox {
	Q_OBJECT
public:
	explicit DTUDoubleSpinBox(QWidget *parent = nullptr);
protected:
	void wheelEvent(QWheelEvent *event);
};

// 窗口创建类
class QCreateHelper : public QObject
{
	Q_OBJECT
public:
	static QWidget *createWidget(uint16_t type, QTCTRL desc, bool editable = true);
	static void getWidgetValue(uint16_t type, char* value, uint32_t size, QWidget* pWidget, bool extend = false);
	static void setWidgetValue(uint16_t type, const char* value, uint32_t size, QWidget* pWidget, bool extend = false);

public:
	// 创建Checkbox
	static QWidget *createCheck(std::string &desc, bool def = false);
	// 创建QCombobox
	static DTUComboBox *createComboBox(QStringList &v, int defaultIndex = 0);
	static DTUComboBox *createComboBox(QString str, int defaultIndex = 0);
	// 创建SpinBox
	static DTUSpinBox *createSpinBox(int min, int max, int step, int def);
	// 创建DoubleSpinBox
	static DTUDoubleSpinBox *createDoubleBox(double min, double max, double step, double def, double des = 3);
	// 创建EditLine
	static QLineEdit *createEditLine(QString def = "", bool readOnly = false, QRegExp* exp = nullptr);
	// 创建指示窗口
	static QLabel *createPoint(bool state);
	// 创建IP地址控件
	static IPAddress *createIPBox(QString def = "0.0.0.0");
	// 创建字符串Label
	static QLabel *createStrLabel(QString str);
	// 创建浮点数Label
	static QLabel *createDoubleStrLabel(QString str);
	// 创建遥控项
	static dturmctrlWidget *createRmcWidget(uint16_t fixid = 0, bool needPre = true);

public:
	// 修改指示窗口
	static void setPoint(QLabel *lab, bool state);
};