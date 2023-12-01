#ifndef DTUARMCONFIGDIALOG_H
#define DTUARMCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>

#include <map>

namespace Ui {
class dtuarmconfigDialog;
}

class dtuarmconfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuarmconfigDialog(QWidget *parent = 0);
    ~dtuarmconfigDialog();

private:
    Ui::dtuarmconfigDialog *ui;

public slots:
	void read();
	void save();

private:
    void load_ui();
    void setLineEdit(QComboBox* widget);

private slots:
    void modeChange(int index);
    void checkChange(int state);
    void protoChange(int index);
	void BayCFGChange(int index);
	void BayCFGSet(int index);

private:
	std::map<int, int> BayMap;

};

#endif // DTUARMCONFIGDIALOG_H
