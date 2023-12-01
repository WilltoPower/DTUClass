#pragma once

#include <QWidget>
#include "ui_dtutestapiwidgetsclass.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtuTestAPIWidgetsClassClass; };
QT_END_NAMESPACE

class dtuTestAPIWidgetsClass : public QWidget
{
	Q_OBJECT

public:
	dtuTestAPIWidgetsClass(QWidget *parent = nullptr);
	~dtuTestAPIWidgetsClass();

private:
	Ui::dtuTestAPIWidgetsClassClass *ui;

private slots:
	void TESTAPI();

};
