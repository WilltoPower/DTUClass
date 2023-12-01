#pragma once

#include <QWidget>
#include "ui_dturmcwidget.h"

#include "dtudbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dturmcwidgetClass; };
QT_END_NAMESPACE

class dturmcwidget : public QWidget
{
	Q_OBJECT

public:
	dturmcwidget(QWidget *parent = nullptr);
	~dturmcwidget();
	void load_ui();

private:
	Ui::dturmcwidgetClass *ui;

private:

	QTableWidget *createTabWidget(QTableWidget *ptable, int row, QStringList header);
private slots:
	void rmctrlhanlder(int opt, uint16_t fixid);
	void checkpassword();
};
