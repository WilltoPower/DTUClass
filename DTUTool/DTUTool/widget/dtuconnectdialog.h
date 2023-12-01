#ifndef DTUCONNECTDIALOG_H
#define DTUCONNECTDIALOG_H

#include <QDialog>
#include <QToolButton>
#include <atomic>

class frmMain;

namespace Ui {
class dtuconnectDialog;
}

class dtuconnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuconnectDialog(QWidget *parent = 0);
    ~dtuconnectDialog();

protected:
	bool eventFilter(QObject *target, QEvent *event);

private:
    Ui::dtuconnectDialog *ui;

public:
	void setButton(QToolButton* btn);
	void setConnectInfo();

public slots:
	void setIcon(bool state);

private:
	void load_ui();
	// 控制主界面连接状态
	QToolButton* btnmainconnect = nullptr;
	std::atomic_bool issetbutton;

private slots:
	void click_connect_button();

signals:
	void connect_change();

};

#endif // DTUCONNECTDIALOG_H
