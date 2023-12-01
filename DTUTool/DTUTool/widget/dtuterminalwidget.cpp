#include "dtuterminalwidget.h"
#include "ui_dtuterminalwidget.h"

#include <QProcess>
#include <windows.h>
#include <QWindow>
#include <QBoxLayout>

#include "dtucommon.h"

dtuTerminalWidget::dtuTerminalWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dtuTerminalWidget)
{
    ui->setupUi(this);
}

dtuTerminalWidget::~dtuTerminalWidget()
{
    delete ui;
}

void dtuTerminalWidget::initWidget()
{
	QString Name = QString::fromStdString(get_exec_dir() + "\\DTUTools.exe");
	WId hwnd = (WId)FindWindow(L"ConsoleWindowClass", (LPCTSTR)Name.unicode());

	//Ç¶Èë
	if (hwnd > 0)
	{
		QWindow *m_window;
		m_window = QWindow::fromWinId(WId(hwnd));
		QWidget *m_widget;
		m_widget = QWidget::createWindowContainer(m_window, ui->widget);

		QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
		layout->addWidget(m_widget);
		ui->widget->setLayout(layout);
	}

}