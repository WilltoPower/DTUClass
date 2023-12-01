#pragma execution_character_set("utf-8")

#include "frmmain.h"
#include "ui_frmmain.h"
#include "iconhelper.h"
#include "quihelper.h"

#include "dtuconnectdialog.h"
#include "dtutoolinit.h"
#include "QCreateHelper.h"

#include "dturpcclient.h"
#include "dtuparamwidget.h"
#include "dtureportwidget.h"
#include "dtusoftinfodialog.h"
#include "dtucommon.h"

#define WIDGET_INDEX_MAINPAGE 0
#define WIDGET_INDEX_CONFIG   1
#define WIDGET_INDEX_CSPROTO  2
#define WIDGET_INDEX_PARAM    3
#define WIDGET_INDEX_CONNECT  4
#define WIDGET_INDEX_REPORT   5
#define WIDGET_INDEX_ADJUST   6
#define WIDGET_INDEX_HISTORY  7
#define WIDGET_INDEX_RMC	  8
#define WIDGET_INDEX_FIXID	  9
#define WIDGET_INDEX_TERMI	  10

#define INFOMATION_OFF dturpcclient::instance().setInformationID(false);
#define INFOMATION_ON  dturpcclient::instance().setInformationID(true);

frmMain::frmMain(QWidget *parent) : QWidget(parent), ui(new Ui::frmMain)
{
    ui->setupUi(this);

    this->initForm();
    this->initStyle();
    this->initLeftMain();
    this->initLeftConfig();
	this->initLeftParam();
	this->initLeftReport();

	this->initWidget();

	// TEST API窗口
	//TESTAPI = new dtuTestAPIWidgetsClass;
	//TESTAPI->show();
}

frmMain::~frmMain()
{
    delete ui;
}

bool frmMain::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->widgetTitle) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            on_btnMenu_Max_clicked();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void frmMain::getQssColor(const QString &qss, const QString &flag, QString &color)
{
    int index = qss.indexOf(flag);
    if (index >= 0) {
        color = qss.mid(index + flag.length(), 7);
    }
    //qDebug() << TIMEMS << flag << color;
}

void frmMain::getQssColor(const QString &qss, QString &textColor, QString &panelColor,
                          QString &borderColor, QString &normalColorStart, QString &normalColorEnd,
                          QString &darkColorStart, QString &darkColorEnd, QString &highColor)
{
    getQssColor(qss, "TextColor:", textColor);
    getQssColor(qss, "PanelColor:", panelColor);
    getQssColor(qss, "BorderColor:", borderColor);
    getQssColor(qss, "NormalColorStart:", normalColorStart);
    getQssColor(qss, "NormalColorEnd:", normalColorEnd);
    getQssColor(qss, "DarkColorStart:", darkColorStart);
    getQssColor(qss, "DarkColorEnd:", darkColorEnd);
    getQssColor(qss, "HighColor:", highColor);
}

void frmMain::initForm()
{
    //设置无边框
    QUIHelper::setFramelessForm(this);
    //设置图标
    IconHelper::setIcon(ui->labIco, 0xf073, 30);
    IconHelper::setIcon(ui->btnMenu_Min, 0xf068);
    IconHelper::setIcon(ui->btnMenu_Max, 0xf067);
    IconHelper::setIcon(ui->btnMenu_Close, 0xf00d);

    //ui->widgetMenu->setVisible(false);
    ui->widgetTitle->setProperty("form", "title");
    //关联事件过滤器用于双击放大
    ui->widgetTitle->installEventFilter(this);
    ui->widgetTop->setProperty("nav", "top");

    QFont font;
    font.setPixelSize(25);
    ui->labTitle->setFont(font);
    ui->labTitle->setText("SDL9200管理软件");
    this->setWindowTitle(ui->labTitle->text());

	// 设置文字大小
    //ui->stackedWidget->setStyleSheet("QLabel{font:20px;}");
	ui->stackedWidget->setStyleSheet("QTableWidget{font:16px;}");

    QSize icoSize(32, 32);
    int icoWidth = 85;

    // 设置顶部导航按钮
    QList<QAbstractButton *> tbtns = ui->widgetTop->findChildren<QAbstractButton *>();
    for (QAbstractButton *btn : tbtns)
	{
        btn->setIconSize(icoSize);
        btn->setMinimumWidth(icoWidth);
        btn->setCheckable(true);
        connect(btn, SIGNAL(clicked()), this, SLOT(buttonClick()));
    }

    ui->btnMain->click();

    ui->widgetLeftMain->setProperty("flag", "left");
    ui->widgetLeftConfig->setProperty("flag", "left");
	ui->widgetLeftParam->setProperty("flag", "left");
	ui->widgetLeftReport->setProperty("flag", "left");

    ui->page1->setStyleSheet(QString("QWidget[flag=\"left\"] QAbstractButton{min-height:%1px;max-height:%1px;}").arg(60));
    ui->page2->setStyleSheet(QString("QWidget[flag=\"left\"] QAbstractButton{min-height:%1px;max-height:%1px;}").arg(25));

}

void frmMain::initStyle()
{
    //加载样式表
    QString qss;
    QFile file(":/qss/blacksoft.css");
    if (file.open(QFile::ReadOnly)) {
        qss = QLatin1String(file.readAll());
        QString paletteColor = qss.mid(20, 7);
        qApp->setPalette(QPalette(paletteColor));
        qApp->setStyleSheet(qss);
        file.close();
    }

    //先从样式表中取出对应的颜色
    QString textColor, panelColor, borderColor, normalColorStart, normalColorEnd, darkColorStart, darkColorEnd, highColor;
    getQssColor(qss, textColor, panelColor, borderColor, normalColorStart, normalColorEnd, darkColorStart, darkColorEnd, highColor);

    //将对应颜色设置到控件
    this->borderColor = highColor;
    this->normalBgColor = normalColorStart;
    this->darkBgColor = panelColor;
    this->normalTextColor = textColor;
    this->darkTextColor = normalTextColor;
}

void frmMain::buttonClick()
{
    QAbstractButton *b = (QAbstractButton *)sender();
    QString name = b->text();

    QList<QAbstractButton *> tbtns = ui->widgetTop->findChildren<QAbstractButton *>();
    foreach (QAbstractButton *btn, tbtns) {
        btn->setChecked(btn == b);
    }

    if (name == "主界面") {
		INFOMATION_ON
        ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_MAINPAGE);
    } else if (name == "系统设置") {
		static bool firstClick = true;
		if (firstClick) {
			ui->pageConfig1->read();
			firstClick = false;
		}
        ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_CONFIG);
		INFOMATION_OFF
    } else if (name == "报文监控") {
        ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_CSPROTO);
		INFOMATION_OFF
    } else if (name == "定值管理") {
        ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_PARAM);
		INFOMATION_OFF
	} else if (name == "连接管理") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_CONNECT);
		INFOMATION_OFF
	} else if (name == "报告管理") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_REPORT);
		INFOMATION_OFF
	} else if (name == "定值整定") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_ADJUST);
		INFOMATION_OFF
	} else if (name == "历史文件") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_HISTORY);
		INFOMATION_OFF
	} else if (name == "遥调遥控") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_RMC);
		INFOMATION_OFF
	} else if (name == "点表管理") { 
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_FIXID);
		INFOMATION_OFF
	} else if (name == "监视终端") {
		ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_TERMI);
		INFOMATION_OFF
	} else if (name == "软件信息") {
		dtusoftinfoDialog softdialog;
		softdialog.exec();
	} else if (name == "用户退出") {
        exit(0);
    }

}

void frmMain::initLeftMain()
{
    iconsMain << 0xf030 << 0xf03e << 0xf247;
    btnsMain << ui->tbtnMain1 << ui->tbtnMain2 << ui->tbtnMain3;

    int count = btnsMain.count();
    for (int i = 0; i < count; ++i) {
        QToolButton *btn = (QToolButton *)btnsMain.at(i);
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(leftMainClick()));
    }

    IconHelper::StyleColor styleColor;
    styleColor.position = "left";
    styleColor.iconSize = 18;
    styleColor.iconWidth = 35;
    styleColor.iconHeight = 25;
    styleColor.borderWidth = 4;
    styleColor.borderColor = borderColor;
    styleColor.setColor(normalBgColor, normalTextColor, darkBgColor, darkTextColor);
    IconHelper::setStyle(ui->widgetLeftMain, btnsMain, iconsMain, styleColor);
    ui->tbtnMain1->click();
}

void frmMain::initLeftConfig()
{
    iconsConfig << 0xf031 << 0xf036 << 0xf249 << 0xf055 << 0xf05a << 0xf249;
    btnsConfig << ui->tbtnConfig1 << ui->tbtnConfig2 << ui->tbtnConfig3 << ui->tbtnConfig4 << ui->tbtnConfig5 << ui->tbtnConfig6;

    int count = btnsConfig.count();
    for (int i = 0; i < count; ++i) {
        QToolButton *btn = (QToolButton *)btnsConfig.at(i);
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(leftConfigClick()));
    }

    IconHelper::StyleColor styleColor;
    styleColor.position = "left";
    styleColor.iconSize = 16;
    styleColor.iconWidth = 20;
    styleColor.iconHeight = 20;
    styleColor.borderWidth = 3;
    styleColor.borderColor = borderColor;
    styleColor.setColor(normalBgColor, normalTextColor, darkBgColor, darkTextColor);

    IconHelper::setStyle(ui->widgetLeftConfig, btnsConfig, iconsConfig, styleColor);
    ui->tbtnConfig1->click();
}

void frmMain::initLeftParam()
{
	iconsParam << 0xf031 << 0xf036 << 0xf249 << 0xf055 << 0xf05a << 0xf249
				<< 0xf031 << 0xf036 << 0xf249 << 0xf055 << 0xf05a << 0xf249
				<< 0xf031 << 0xf036 << 0xf249 << 0xf055;
	btnsParam << ui->tbtnParam1 << ui->tbtnParam2 << ui->tbtnParam3
				<< ui->tbtnParam4 << ui->tbtnParam5 << ui->tbtnParam6
				<< ui->tbtnParam7 << ui->tbtnParam8 << ui->tbtnParam9
				<< ui->tbtnParam10 << ui->tbtnParam11 << ui->tbtnParam12
				<< ui->tbtnParam13 << ui->tbtnParam14 << ui->tbtnParam15
				<< ui->tbtnParam16;

	int count = btnsParam.count();
	for (int i = 0; i < count; ++i) {
		QToolButton *btn = (QToolButton *)btnsParam.at(i);
		btn->setCheckable(true);
		btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		connect(btn, SIGNAL(clicked(bool)), this, SLOT(leftParamClick()));
	}

	IconHelper::StyleColor styleColor;
	styleColor.position = "left";
	styleColor.iconSize = 16;
	styleColor.iconWidth = 20;
	styleColor.iconHeight = 20;
	styleColor.borderWidth = 3;
	styleColor.borderColor = borderColor;
	styleColor.setColor(normalBgColor, normalTextColor, darkBgColor, darkTextColor);

	IconHelper::setStyle(ui->widgetLeftParam, btnsParam, iconsParam, styleColor);
	ui->tbtnParam1->click();
}

void frmMain::initLeftReport()
{
	iconsReport << 0xf031 << 0xf036 << 0xf249 << 0xf055 << 0xf05a << 0xf249 << 0xf249;

	btnsReport << ui->tbtnReport1 << ui->tbtnReport2 << ui->tbtnReport3 
		<< ui->tbtnReport4 << ui->tbtnReport5 << ui->tbtnReport6 << ui->tbtnReport7;
	
	int count = btnsReport.count();
	for (int i = 0; i < count; ++i) {
		QToolButton *btn = (QToolButton *)btnsReport.at(i);
		btn->setCheckable(true);
		btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		connect(btn, SIGNAL(clicked(bool)), this, SLOT(leftReportClick()));
	}

	IconHelper::StyleColor styleColor;
	styleColor.position = "left";
	styleColor.iconSize = 16;
	styleColor.iconWidth = 20;
	styleColor.iconHeight = 20;
	styleColor.borderWidth = 3;
	styleColor.borderColor = borderColor;
	styleColor.setColor(normalBgColor, normalTextColor, darkBgColor, darkTextColor);

	IconHelper::setStyle(ui->widgetLeftReport, btnsReport, iconsReport, styleColor);
	ui->tbtnReport1->click();
}

void frmMain::leftMainClick()
{
    QAbstractButton *b = (QAbstractButton *)sender();
    QString name = b->text();

    int count = btnsMain.count();
    for (int i = 0; i < count; ++i) {
        QAbstractButton *btn = btnsMain.at(i);
		bool equal = (btn == b);
        btn->setChecked(equal);
		if (equal) {
			ui->widgetMain->setCurIndex(i);
			dturpcclient::instance().setInformationID(i, true);
		}
    }
}

void frmMain::leftConfigClick()
{
    QToolButton *b = (QToolButton *)sender();
    QString name = b->text();

    int count = btnsConfig.count();
    for (int i = 0; i < count; ++i) {
        QAbstractButton *btn = btnsConfig.at(i);
        btn->setChecked(btn == b);
		if (btn == b) {
			ui->stackedWidgetConfig->setCurrentIndex(i);
			if (i == 4) {
				ui->pageConfig5->showTime();
				ui->pageConfig5->start();
			}
			else if (i == 1) {
				ui->pageConfig2->updatecfg();
			}
			else if (i == 2) {
				ui->pageConfig3->init();
			}
			else if (i == 0) {
				ui->pageConfig1->read();
			}
			else if (i == 5) {
				ui->pageConfig6->read();
			}
		}
    }
}

void frmMain::leftParamClick()
{
	QToolButton *b = (QToolButton *)sender();
	QString name = b->text();

	int count = btnsParam.count();
	for (int i = 0; i < count; ++i) {
		QAbstractButton *btn = btnsParam.at(i);
		btn->setChecked(btn == b);
		if (btn == b) {
			ui->stackedWidget2->setCurrentIndex(i);
		}
	}
}

void frmMain::leftReportClick()
{
	QToolButton *b = (QToolButton *)sender();
	QString name = b->text();

	int count = btnsReport.count();
	for (int i = 0; i < count; ++i) {
		QAbstractButton *btn = btnsReport.at(i);
		btn->setChecked(btn == b);
		if (btn == b) {
			ui->stackedWidgetReport->setCurrentIndex(i);
		}
	}
}

void frmMain::on_btnMenu_Min_clicked()
{
    showMinimized();
}

void frmMain::on_btnMenu_Max_clicked()
{
    static bool max = false;
    static QRect location = this->geometry();

    if (max) {
        this->setGeometry(location);
    } else {
        location = this->geometry();
        this->setGeometry(QUIHelper::getScreenRect());
    }

    this->setProperty("canMove", max);
    max = !max;
}

void frmMain::on_btnMenu_Close_clicked()
{
    close();
}

void frmMain::initWidget()
{
	connect(this, SIGNAL(everythingisReady()), this, SLOT(everythingReady()));
	// 设置图标
	this->setWindowIcon(QIcon(":/image/LOGO.ico"));
	system("chcp 65001");
	// 工具配置初始化
	DTUTool::dtuToolinit::instance().init();
	// 加载连接窗口
	((dtuconnectDialog*)(ui->widgetConn))->setButton(ui->btnConnect);
	((dtuconnectDialog*)(ui->widgetConn))->setConnectInfo();
	// 加载信息查看窗口
	((dtuinfomationDialog*)(ui->widgetMain))->init();
	// 加载定值窗口
	auto Paramindex = DTU::DBManager::instance().GetParamInfo();
	for (const auto &item : Paramindex)
	{
		dtuparam *ptable = new dtuparam;
		ptable->SetParamID(item.first);
		ui->stackedWidget2->addWidget(ptable);
	}
	connect(ui->stackedWidget2, SIGNAL(currentChanged(int)), this, SLOT(paramPageChanged(int)));
	// 加载报告窗口
	auto Reportindex = DTU::DBManager::instance().GetReportInfo();
	for (const auto &item : Reportindex)
	{
		if (item.first == ReportProSimple)
			continue;
		dtureport *rtable = new dtureport;
		rtable->setCurReportID(item.second.reportid);
		ui->stackedWidgetReport->addWidget(rtable);
	}
	connect(ui->stackedWidgetReport, SIGNAL(currentChanged(int)), this, SLOT(reportPageChanged(int)));
	// 加载遥控窗口
	ui->widgetRmc->load_ui();
	// 加载点表修改窗口
	ui->widgetFixid->load_ui();
	// 加载信息查看
	dturpcclient::instance().setInformationID(0, true);
	// 创建文件夹
	this->execCreateFolder();
	// 加载硬盘信息
	ui->widgetMain->get_disk_usage();
	// 加载备份窗口
	ui->widgetMain->setWidget(ui->widgetAdjust, ui->widgetRulefile);
	// 发送创建完成信号
	emit everythingisReady();
}

void frmMain::paramPageChanged(int index)
{
	auto pwidget = ui->stackedWidget2->widget(index);
	((dtuparam*)(pwidget))->read_param();
}

void frmMain::reportPageChanged(int index)
{
	auto pwidget = ui->stackedWidgetReport->widget(index);
	((dtureport*)(pwidget))->updatereport();
}

void frmMain::execCreateFolder()
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
	create_dir_in_exec("\\output\\back\\");// 导出文件夹
}

void frmMain::everythingReady()
{

#ifndef _DEBUG
	// 终端隐藏
	ui->pageTerminal->initWidget();
#endif

}