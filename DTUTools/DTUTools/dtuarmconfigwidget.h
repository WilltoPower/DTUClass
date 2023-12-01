#pragma once

#include <QWidget>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QComboBox>
#include "json/json.h"

namespace Ui { class dtuarmconfigwidget; };

class dtuarmconfigwidget : public QWidget
{
	Q_OBJECT

public:
	dtuarmconfigwidget(QWidget *parent = Q_NULLPTR);
	~dtuarmconfigwidget();

private:
	Ui::dtuarmconfigwidget *ui;

private:
	void load_ui();
	// 设置间隔单元连接槽函数
	void load_connect();

public slots:
	void SetAll();// 设置到界面
	void GetAll();// 从界面获取
	void GetCurBay();

// 设置界面
private:
	QStringList protocol;
	QList<QComboBox*> tty_ui_list; 
	QList<QComboBox*> baud_ui_list;
	Json::Value interval_info;
	// 找出src与dest不重合的部分
	QStringList notInclude(QStringList src,QStringList dest);

	void SetASDU(int mode,int addr,int other,std::string path,int port);
	void SetTime(bool check, int time);
	void SetLine(bool check, std::string tty,int baud);
	void SetLCD(std::string tty,int baud);
	void SetBay(int index);
	void SetInterval(bool check, int addr, std::string mode);
	void SetCommonunit(std::string ip, int port);

	void GetASDU(Json::Value &root);
	void GetTime(Json::Value &root);
	void GetLine(Json::Value &root);
	void GetLCD(Json::Value &root);
	int curBayNo = -1;
	void GetAllBay(Json::Value &root);
	void GetInterval(Json::Value &bay);
	void GetCommonunit(Json::Value &bay);
private slots:
	void interval_double_click(QTreeWidgetItem *item, int column);
// 控件切换槽函数
private slots:
	// 公共单元/间隔单元切换
	void ASDU_select();
	// 自动校时开/关
	void Time_check();
	// 线损模块
	void Line_check();
	// 间隔单元通信配置开关
	void interval_checkf();
	void interval_connect(bool status = true);
	// 间隔单元私有规约开关
	void internal_pri(bool status);
	// 间隔单元101规约开关
	void internal_101(bool status);
	// 间隔单元104规约开关
	void internal_104(bool status);
};
