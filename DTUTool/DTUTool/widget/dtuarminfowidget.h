#ifndef DTUARMINFOWIDGET_H
#define DTUARMINFOWIDGET_H

#include <QWidget>

#include "dtuoutput.h"

// 整定
#include "dtuadjustwidget.h"

// 历史文件
#include "dturulefilewidget.h"

namespace Ui {
class dtuarminfoWidget;
}

class dtuarminfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit dtuarminfoWidget(QWidget *parent = 0);
    ~dtuarminfoWidget();

	void setWidget(dtuadjustwidget* widget1, dturulefilewidget *widget2);

private:
    Ui::dtuarminfoWidget *ui;

public slots:
	void get_disk_usage();
	void backonece();

private:
	dtuoutput *dtuoutput_ptr = nullptr;

};

#endif // DTUARMINFOWIDGET_H
