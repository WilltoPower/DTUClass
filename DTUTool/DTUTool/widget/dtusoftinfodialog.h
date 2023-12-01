#ifndef DTUSOFTINFODIALOG_H
#define DTUSOFTINFODIALOG_H

#include <QDialog>

namespace Ui {
class dtusoftinfoDialog;
}

class dtusoftinfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtusoftinfoDialog(QWidget *parent = 0);
    ~dtusoftinfoDialog();

private:
    Ui::dtusoftinfoDialog *ui;

private:
	void load_ui();

signals:
	void softinfodialogisReady();

private slots:
	void update_version();
	void btn_double_clicked();

protected:
	bool eventFilter(QObject * watched, QEvent * event);

};

#endif // DTUSOFTINFODIALOG_H
