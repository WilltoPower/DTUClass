#ifndef DTUTERMINALWIDGET_H
#define DTUTERMINALWIDGET_H

#include <QWidget>

namespace Ui {
class dtuTerminalWidget;
}

class dtuTerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit dtuTerminalWidget(QWidget *parent = 0);
    ~dtuTerminalWidget();

private:
    Ui::dtuTerminalWidget *ui;

public:
	void initWidget();

};

#endif // DTUTERMINALWIDGET_H
