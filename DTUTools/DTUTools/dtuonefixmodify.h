#pragma once

#include <QWidget>
#include "ui_dtuonefixmodify.h"

#include <QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class dtuonefixmodifyClass; };
QT_END_NAMESPACE

class dtuonefixmodify : public QWidget
{
	Q_OBJECT

public:
	dtuonefixmodify(int index,uint16_t fixid, QString desc, QString add, QTableWidgetItem *item, QWidget *parent = nullptr);
	~dtuonefixmodify();

private:
	Ui::dtuonefixmodifyClass *ui;

private:
	void setRegx();

private slots:
	void HEXin();
	void DECin();
	void checkid();
	void modifyfixid();

private:
	int index = -1;
	QTableWidgetItem *item_ptr = nullptr;
};
