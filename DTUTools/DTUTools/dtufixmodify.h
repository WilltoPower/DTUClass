#pragma once

#include <QWidget>
#include "ui_dtufixmodify.h"

#include <QStackedLayout>
#include <QTableWidget>

#include "dtudbmanager.h"


QT_BEGIN_NAMESPACE
namespace Ui { class dtufixmodifyClass; };
QT_END_NAMESPACE

class dtufixmodify : public QWidget
{
	Q_OBJECT

public:
	dtufixmodify(QWidget *parent = nullptr);
	~dtufixmodify();

private:
	Ui::dtufixmodifyClass *ui;

private:
	void load_ui();
	void createTabWidget(QTableWidget *&ptable, int row, QStringList header);
private:
	std::vector<int> indexvec;
	QTableWidget *CurWidget = nullptr;
	QStackedLayout *widgetlayout = nullptr;
	int CurNo = -1;
private slots:
	void serach();
	void front();
	void next();
	void indexchange(int index);
	void DoubleClickedItem(int row, int column);
};
