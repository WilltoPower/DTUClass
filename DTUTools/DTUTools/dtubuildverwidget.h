#pragma once

#include <QWidget>
#include "ui_dtubuildverwidget.h"
#include <dtubuffer.h>
#include <map>
#include <tuple>
#include <QLabel>
#include <QLineEdit>

// 内部版本

using namespace DTU;

class dtubuildverwidget : public QWidget
{
	Q_OBJECT

public:
	dtubuildverwidget(QWidget *parent = Q_NULLPTR);
	~dtubuildverwidget();
	void load_ui();
	void addVersion(uint8_t index,DTU::buffer &data);
	void addOneBlock(uint8_t index,uint8_t data,int &no);
private slots:
	void GetVersion();
private:
	Ui::dtubuildverwidget *ui;
};