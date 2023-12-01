#include "dtutools.h"
#include "dtusysconfig.h"
#include <dturpcmanager.h>
#include <dtucommon.h>
#include <dtustorage.h>
#include <dtuparamconfig.h>
#include "dtuversiondlg.h"
#include "dtutask.h"
#include <QMessagebox>
#include "dtucmdcode.h"
#include "dturpcclient.h"
#include <QProgressBar>

DTUTools::DTUTools(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	auto p = takeCentralWidget();
	if (p) {
		delete p;
	}
	setDockNestingEnabled(true);
	showMaximized();
	QWidget::setWindowFlags(Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
	// 设置图标
	this->setWindowIcon(QIcon(":/DTUTools/icon/LOGO.ico"));
	// 创建所需文件夹
	execCreateFolder();
	// 载入配置
	dtusysconfig::instance().load();
	// 加载参数文件
	char exeFullPath[MAX_PATH] = {};
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::string strFullPath = (std::string)(exeFullPath);
	int nStart = strFullPath.find_last_of(TEXT("\\"));
	std::string strexepath = strFullPath.substr(0, nStart);
	DTU::DSTORE::instance().load(strexepath + "\\config\\dtu.db");

	uint32_t currentGroup = 1;
	uint32_t maxGroup = 1;
	// 加载UI界面
	load_ui();

	execute_read_group(currentGroup, maxGroup);
	// 连接状态置为断开
	set_arm_connect_state(false);
	// 初始化RPC
	dturpcclient::instance().init(DESTIP(), DESTPORT());
	dturpcclient::instance().run();

	// 主界面放置到最前
	ui.dockWidget_menu->raise();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(TimeOutFunc()));
}

void DTUTools::load_ui()
{
	QRect rcWnd = rect();

	// DOCK 左边Tab
	_pSysConfigWidget = new DTUSysConfigWidget(this);
	ui.dockWidget_sysconfig->setWidget(_pSysConfigWidget);
	// DOCK 第一个Tab 主窗口
	_mainmenuwidget = new dtumainmenu(this);
	ui.dockWidget_menu->setWidget(_mainmenuwidget);
	// DOCK 第二个Tab 整定窗口
	_AdjustWidget = new dtuadjustwidget(this);
	ui.dockWidget_adjust->setWidget(_AdjustWidget);
	// DOCK 第三个Tab 规约报文查看窗口
	_csprotocolWidget = new dtucsprotocolwidget(this);
	ui.dockWidget_csprotocol->setWidget(_csprotocolWidget);

	// 暂存窗口
	splitDockWidget(ui.dockWidget_sysconfig, ui.dockWidget_menu, Qt::Horizontal);
	tabifyDockWidget(ui.dockWidget_menu, ui.dockWidget_adjust);
	tabifyDockWidget(ui.dockWidget_menu, ui.dockWidget_csprotocol);

	auto w1 = 1 * rcWnd.width() / 5;
	auto w2 = 4 * rcWnd.width() / 5;

	resizeDocks({ ui.dockWidget_sysconfig }, { w1 }, Qt::Horizontal);
	resizeDocks({ ui.dockWidget_menu }, { w2 }, Qt::Horizontal);

	// 工具栏不可移动
	ui.mainToolBar->setMovable(false);

	// 初始化底部状态栏
	load_state_ui();
	// 初始化软件编译时间
	load_mktime_ui();
}

void DTUTools::load_state_ui()
{
	ui.statusBar->showMessage("Welcome! ", 20000);
	progress = new QProgressBar;
	ui.statusBar->addPermanentWidget(progress);
	progress->setFormat(QString::fromLocal8Bit("任务进度:%1%").arg(QString::number(0, 'f', 1)));
	progress->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // 对齐方式
	progress->setValue(10);
	progress->setMaximumHeight(25);
	progress->setMaximumWidth(240);
	progress->hide();

	// 底部状态栏槽函数
	//connect(_pReportWidget, SIGNAL(send_status_info(QString, int, int, int, QString, int, bool)),
	//	this, SLOT(set_status_info(QString, int, int, int, QString, int, bool)));
	//connect(_pSettingConfigWidget, SIGNAL(send_status_info(QString, int, int, int, QString, int, bool)),
	//	this, SLOT(set_status_info(QString, int, int, int, QString, int, bool)));
	//connect(_pSettingConfigWidget->AdjustWidget, SIGNAL(send_status_info(QString, int, int, int, QString, int, bool)),
	//	this, SLOT(set_status_info(QString, int, int, int, QString, int, bool)));
	//connect(_NetWidget, SIGNAL(send_status_info(QString, int, int, int, QString, int, bool)),
	//	this, SLOT(set_status_info(QString, int, int, int, QString, int, bool)));
}

void DTUTools::load_mktime_ui()
{
	QString dateTime;
	dateTime += __DATE__;
	dateTime += " ";
	dateTime += __TIME__;
	QString version = "软件编译时间:" + dateTime;
	ui.statusBar->addPermanentWidget(new QLabel(version, this));
}

void DTUTools::show_version()
{
	dtuversiondlg dlg;
	dlg.exec();
	dlg.close();
}

void DTUTools::update_soft()
{
	if (!_updateWidget)
	{
		_updateWidget = new dtuupdateWidget();
	}
	if (_updateWidget)
	{
		_updateWidget->show();
	}

}

void DTUTools::restart_soft()
{
	if (QMessageBox::Yes == QMessageBox::question(this,"重启设备", "确定重启设备?", QMessageBox::Yes | QMessageBox::No))
	{
		execute_rmctrl(0,PC_W_ARM_REBOOT,0,0);
		DTULOG(DTU_INFO,"后台arm程序正在重启, 请稍后");
	}
}

void DTUTools::build_version()
{
	if (!_buildverwidget)
	{
		_buildverwidget = new dtubuildverwidget();
	}
	if (_buildverwidget)
	{
		_buildverwidget->show();
	}
}

void DTUTools::arm_config()
{
	if(!_armconfigwidget)
		_armconfigwidget = new dtuarmconfigwidget();
	if(_armconfigwidget)
		_armconfigwidget->show();
}

void DTUTools::goose_config()
{
	if (!_goosecfg) {
		_goosecfg = new dtugoosecfg();
		_goosecfg->show();
	}
	else {
		delete _goosecfg;
		_goosecfg = new dtugoosecfg();
		_goosecfg->show();
	}
}

void DTUTools::calibrate_time()
{
	_calibrateTime = new dtuCalibrateTimeDlg();
	_calibrateTime->exec();
	if (_calibrateTime)
	{
		delete _calibrateTime;
	}
}

void DTUTools::set_status_info(QString showstr,int startv, int endv, int msec,QString statusstr,int timeout,bool flag)
{
	if (flag)
		progress->setStyleSheet("QProgressBar{background:white;} QProgressBar::chunk{border-radius:5px;background:green}");
	else
		progress->setStyleSheet("QProgressBar{background:white;} QProgressBar::chunk{border-radius:5px;background:red}");
	progress->show();
	for (int p = startv; p <= endv; p++)
	{
		progress->setValue(p);
		progress->setFormat(showstr + " " + QString::fromLocal8Bit("任务进度:%1%").arg(QString::number(p)));
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ui.statusBar->showMessage(statusstr, timeout);
	timer->start(msec);
}

void DTUTools::TimeOutFunc()
{
	progress->hide();
}


void DTUTools::execCreateFolder()
{
	create_dir_in_exec("\\protect\\comtrade\\");
	create_dir_in_exec("\\protect\\factory\\");
	create_dir_in_exec("\\protect\\protect\\");//保护动作报告文件夹
	create_dir_in_exec("\\protect\\selfcheck\\");// 自检文件夹
	create_dir_in_exec("\\HISTORY\\SOE\\");
	create_dir_in_exec("\\HISTORY\\CO\\");
	create_dir_in_exec("\\HISTORY\\EXV\\");
	create_dir_in_exec("\\HISTORY\\FIXPT\\");
	create_dir_in_exec("\\HISTORY\\ULOG\\");
	create_dir_in_exec("\\COMTRADE\\");
	create_dir_in_exec("\\FACTORY\\");
	create_dir_in_exec("\\HISTORY\\FRZ\\");//电能量相关文件夹
	create_dir_in_exec("\\HISTORY\\FLOWREV\\");
	create_dir_in_exec("\\update\\");// 升级文件夹
	create_dir_in_exec("\\output\\");// 导出文件夹
}