#ifndef DTUPROTOCFGDIALOG_H
#define DTUPROTOCFGDIALOG_H

#include <QDialog>

namespace Ui {
class dtuprotocfgDialog;
}

class dtuprotocfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuprotocfgDialog(QWidget *parent = 0);
    ~dtuprotocfgDialog();

private:
    Ui::dtuprotocfgDialog *ui;

public:
	void load_ui();
	void init();

private slots:
	void read_config();
	void save_config();
	void change_check(int state);

};

#endif // DTUPROTOCFGDIALOG_H
