#pragma once

#include <QWidget>
#include "ui_dtuoutput.h"

// ����
#include "dtuadjustwidget.h"

// ��ʷ�ļ�
#include "dturulefilewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtuoutputClass; };
QT_END_NAMESPACE

class dtuoutput : public QWidget
{
	Q_OBJECT

public:
	dtuoutput(QWidget *parent = nullptr);
	~dtuoutput();

	void setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2);

public slots:
	void back();

private:
	Ui::dtuoutputClass *ui;
	dtuadjustwidget* adjustwidget = nullptr;
	dturulefilewidget *rulewidget = nullptr;
};
