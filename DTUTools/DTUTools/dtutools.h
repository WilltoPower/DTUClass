#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_dtutools.h"
#include "dtusysconfigwidget.h"
#include "dtuupdateWidget.h"
#include "dtuerror.h"
#include "dtulog.h"
#include "dtucalibratetimedlg.h"
#include "dtucsprotocolwidget.h"
#include "dtubuildverwidget.h"
#include "dtuarmconfigwidget.h"
#include <QTimer>

#include "dtumainmenu.h"
#include "dtuadjustwidget.h"
#include "dtugoosecfg.h"

class DTUTools : public QMainWindow
{
    Q_OBJECT

public:
    DTUTools(QWidget *parent = Q_NULLPTR);
protected:
	void load_ui();

	// 底部状态栏加载
	void load_state_ui();
	void load_mktime_ui();
public slots:
	void show_version();
	void update_soft();
	void restart_soft();
	void calibrate_time();
	void build_version();
	void arm_config();
	void goose_config();
	// 底部状态栏信息
	void set_status_info(QString showstr, int startv, int endv, int msec, QString statusstr, int timeout,bool flag);
	void TimeOutFunc();
private:
	// 创建所需文件夹
	void execCreateFolder();
private:
    Ui::DTUToolsClass ui;

	QProgressBar* progress = nullptr;
	QTimer *timer = nullptr;
private:
	// 窗口指针
	// 左边栏信息指针
	DTUSysConfigWidget* _pSysConfigWidget = nullptr;
	// 主窗口
	dtumainmenu* _mainmenuwidget = nullptr;
	// 整定窗口指针
	dtuadjustwidget* _AdjustWidget = nullptr;
	// 规约窗口
	dtucsprotocolwidget *_csprotocolWidget = nullptr;

	// 校时窗口
	dtuCalibrateTimeDlg *_calibrateTime = nullptr;
	// 升级窗口
	dtuupdateWidget *_updateWidget = nullptr;
	// 新内部版本窗口
	dtubuildverwidget *_buildverwidget = nullptr;
	// 后台配置窗口
	dtuarmconfigwidget *_armconfigwidget = nullptr;
	// GOOSE配置窗口
	dtugoosecfg *_goosecfg = nullptr;
};
