#ifndef DTUINFOMATIONDIALOG_H
#define DTUINFOMATIONDIALOG_H

#include <QDialog>
#include <QTableWidget>

#include "dtudbmanager.h"

// 整定
#include "dtuadjustwidget.h"

// 历史文件
#include "dturulefilewidget.h"

namespace Ui {
class dtuinfomationDialog;
}

class dtuinfomationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dtuinfomationDialog(QWidget *parent = 0);
    ~dtuinfomationDialog();

private:
    Ui::dtuinfomationDialog *ui;

public:
	void init();
	void setCurIndex(int no);

private:
	void load_ui();
	bool checkInfoDateSize(uint16_t infoID, int size);
	QTableWidget *createTabWidget(QTableWidget *itable, int row, QStringList header);
	QTableWidget *createTabWidget(QTableWidget *widget, int row, int column);
	void load_one_widget(QTableWidget *widget, uint16_t infoid);

public:
	//void setCurWidgetInfoID(uint16_t infoid, DTU::dtuInfoTable &table);
	//uint16_t curInfoID();

public slots:
	void updateinfo(unsigned short infoID, QByteArray data);
	void get_disk_usage();
	void setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2);

private:
	uint16_t curPage = 0;
	uint16_t infoID = 0xFF;
	DTU::dtuInfoTable infoindex;
};

#endif // DTUINFOMATIONDIALOG_H
