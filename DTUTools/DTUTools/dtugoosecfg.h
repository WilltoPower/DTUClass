#ifndef DTUGOOSECFG_H
#define DTUGOOSECFG_H

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include "dtumacaddress.h"

namespace Ui {
class dtugoosecfg;
}

class dtugoosecfg : public QWidget
{
    Q_OBJECT

public:
    explicit dtugoosecfg(QWidget *parent = 0);
    ~dtugoosecfg();

private:
    Ui::dtugoosecfg *ui;
private slots:
    void updatecfg();
    void savecfg();
    void checkTrans();
    void checkUITrans(QCheckBox *check,MacAddress *macaddr,QSpinBox *box, bool state);
};

#endif // DTUGOOSECFG_H
