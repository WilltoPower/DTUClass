#ifndef DTUINVERSIONDIALOG_H
#define DTUINVERSIONDIALOG_H

#include <QDialog>
#include <QLabel>

#include "dtubuffer.h"

namespace Ui {
class dtuinversionDialog;
}

class dtuinversionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuinversionDialog(QWidget *parent = 0);
    ~dtuinversionDialog();

private:
    Ui::dtuinversionDialog *ui;

private:
	void load_ui();
	void addOneLabel(QLabel *lab, DTU::buffer &data);
	void loadOneData(QLabel *lab, DTU::buffer &data, int &offset);

private slots:
	void update_version();

signals:
	void inversionDialogReady();

};

#endif // DTUINVERSIONDIALOG_H
