#pragma once

#include <QWidget>
#include "ui_dtuinformationwidget.h"

#include "dtudbmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dtuinformationClass; };
QT_END_NAMESPACE

class dtuinformation : public QWidget
{
	Q_OBJECT

public:
	dtuinformation(QWidget *parent = nullptr);
	~dtuinformation();

private:
	Ui::dtuinformationClass *ui;

public:
	void setCurWidgetInfoID(uint16_t infoid, DTU::dtuInfoTable &table);
	uint16_t curInfoID();
private:
	void load_ui();
	bool checkInfoDateSize(uint16_t infoID, int size);
	QTableWidget *createTabWidget(QTableWidget *itable, int row, QStringList header);
private:
	uint16_t infoID = 0xFF;
	DTU::dtuInfoTable infoindex;
public slots:
	void updateinfo(unsigned short infoID, QByteArray data);
};
