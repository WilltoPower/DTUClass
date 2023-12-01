#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QWidget>
#include "dtutestapiwidgetsclass.h"

class QAbstractButton;

namespace Ui {
class frmMain;
}

class frmMain : public QWidget
{
    Q_OBJECT

public:
    explicit frmMain(QWidget *parent = 0);
    ~frmMain();

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::frmMain *ui;

    QList<int> iconsMain;
    QList<QAbstractButton *> btnsMain;

    QList<int> iconsConfig;
    QList<QAbstractButton *> btnsConfig;

	QList<int> iconsParam;
	QList<QAbstractButton *> btnsParam;

	QList<int> iconsReport;
	QList<QAbstractButton *> btnsReport;

private:
    //根据QSS样式获取对应颜色值
    QString borderColor;
    QString normalBgColor;
    QString darkBgColor;
    QString normalTextColor;
    QString darkTextColor;

    void getQssColor(const QString &qss, const QString &flag, QString &color);
    void getQssColor(const QString &qss, QString &textColor,
                     QString &panelColor, QString &borderColor,
                     QString &normalColorStart, QString &normalColorEnd,
                     QString &darkColorStart, QString &darkColorEnd,
                     QString &highColor);

private slots:
    void initForm();
    void initStyle();
    void buttonClick();
    void initLeftMain();
    void initLeftConfig();
	void initLeftParam();
	// 报告左栏初始化
	void initLeftReport();
    void leftMainClick();
    void leftConfigClick();
	// 定值左栏点击事件
	void leftParamClick();
	// 报告左栏点击事件
	void leftReportClick();
	void initWidget();

private slots:
    void on_btnMenu_Min_clicked();
    void on_btnMenu_Max_clicked();
    void on_btnMenu_Close_clicked();

private:
	template<typename _Typename>
	_Typename* initOneWidget(QWidget *widget)
	{
		_Typename *dlg = new _Typename(widget);
		dlg->show();
		dlg->setGeometry(0, 0, widget->width(), widget->height());
		return dlg;
	}
private slots:
	void paramPageChanged(int index);
	void reportPageChanged(int index);
	void execCreateFolder();

private slots:
	void everythingReady();

signals:
	void everythingisReady();

private:
	dtuTestAPIWidgetsClass *TESTAPI = nullptr;
};

#endif // FRMMAIN_H
