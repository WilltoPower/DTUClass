#pragma execution_character_set("utf-8")

#pragma once

#include <QWidget>
#include "ui_dtucsprotocolexplain.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtucsprotocolexplainClass; };
QT_END_NAMESPACE

class dtucsprotocolexplain : public QWidget
{
	Q_OBJECT

public:
	dtucsprotocolexplain(QWidget *parent = nullptr);
	~dtucsprotocolexplain();

private:
	Ui::dtucsprotocolexplainClass *ui;

public:
	void setText(QString &text, QString time, QString proto, QString dir, QString size);

};
