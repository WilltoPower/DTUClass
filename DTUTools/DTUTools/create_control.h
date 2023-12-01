#pragma once
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

#include "dtuipaddress.h"
#include "dturmctrlWidget.h"

class SwitchControl;
class QComboBox;
class QLineEdit;
class QListWidget;
class QLabel;
class QCheckBox;

class SDLPCTSpinBox : public QSpinBox
{
	Q_OBJECT

public:
	explicit SDLPCTSpinBox(QWidget *parent = nullptr);
private:
	void wheelEvent(QWheelEvent *event);
};
class SDLPCTDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	explicit SDLPCTDoubleSpinBox(QWidget *parent = nullptr);
private:
	void wheelEvent(QWheelEvent *event);
};

#define SWITCH    0  // checkbox
#define COMBO     1	 // combobox
#define SPIN      2  // 整数spinbox
#define DSPIN     3  // 浮点数doublespinbox
#define EDIT      4  // editline
#define LABLE     5  // lable 只显示1/0
#define IPWIDGET  6  // IP地址控件
#define STRLABEL  7  // 字符串label
#define STRDLABEL 8  // 浮点数label
#define RMCWIDGET 9  // 遥控控件

using CTRLATTR = std::tuple<double, double, std::string, bool, std::string, double, std::string>;

using QTCTRL = std::tuple<std::string,double,double,std::string,double,std::string>;

#define  QTCRL_DESC 0  // 描述
#define  QTCRL_MINI 1  // 最小值
#define  QTCRL_MAXI 2  // 最大值
#define  QTCRL_DEFA 3  // 默认值
#define  QTCRL_STEP 4  // 步长
#define	 QTCRL_LIST 5  // 列表


#define V_MIN 0
#define V_MAX 1
#define V_DEF 2
#define V_READ 3 //XX
#define V_LIST 4
#define V_STEP 5
#define V_EXP  6 //XX

IPAddress* createIPWidget();
QCheckBox* createSwitch(std::string desc, bool def = false);
QSpinBox* createSpinBox(int min, int max, int step, int def);
QDoubleSpinBox* createDoubleBox(double min, double max, double step, double def, double des = 3);
QComboBox* createComBox(QString str, int defaultIndex = 0);//XX
QComboBox* createComBox(QStringList v, int defaultIndex = 0);
QLineEdit* createEditLine(QString def = "", bool readOnly = false, QRegExp* exp = nullptr);
QListWidget* createList(QStringList v);
QLabel* createLabel(bool str = true);
QLabel* createStrLabel(QString str);
IPAddress* createIPWidget();
dturmctrlWidget* createRmcWidget(uint16_t fixid = 0,bool needPre = true);
//////////////////



QWidget* createControl(uint16_t type, QTCTRL desc, bool editable = true);

void getControlValue(uint16_t type, char* value, uint32_t size, QWidget* pWidget, bool extend = false);

void setControlValue(uint16_t type, const char* value, uint32_t size, QWidget* pWidget, bool extend = false);


quint32 IPV4StringToInteger(const QString& ip);
QString IPV4IntegerToString(quint32 ip);


QLabel* createStrLabel(QString str);