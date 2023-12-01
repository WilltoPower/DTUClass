#pragma once

#include <QWidget>
#include "ui_dtuparamwidget.h"

#include "dtudbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtuparamClass; };
QT_END_NAMESPACE

class dtuparam : public QWidget
{
	Q_OBJECT

public:
	dtuparam(QWidget *parent = nullptr);
	~dtuparam();

private:
	Ui::dtuparamClass *ui;

public:
	void SetParamID(uint16_t paramid);
	uint16_t GetCurWidgetParamID();

public slots:
	void read_param();

private slots:
	void save_param();
	void set_default();

private:
	void load_ui();
	// 检查参数正确性
	bool checkData(uint16_t paramid, int datasize);
	QTableWidget *createTabWidget(QTableWidget *ptable, int row, QStringList header);

private:
	// 当前ParamID
	uint16_t paramID = 0;
	// 定值信息
	DTU::dtuParamIndexTable Paramindex;
};
