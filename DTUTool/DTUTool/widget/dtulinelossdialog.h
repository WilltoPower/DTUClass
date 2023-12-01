#ifndef DTULINELOSSDIALOG_H
#define DTULINELOSSDIALOG_H

#include <QDialog>
#include <QCombobox>

namespace Ui {
class dtuLineLossDialog;
}

class dtuLineLossDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuLineLossDialog(QWidget *parent = 0);
    ~dtuLineLossDialog();

private:
    Ui::dtuLineLossDialog *ui;

public slots:
	void init();
	void read();
	void save();

private slots:
	void load_ui();
	void checkChange(int state);
	void setLineEdit(QComboBox *widget);

};

#endif // DTULINELOSSDIALOG_H
