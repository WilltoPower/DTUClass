#include "dturmctrlWidget.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <thread>
#include <QTimer>
#include <QDebug>
#include <QSpinBox>

dturmctrlWidget::dturmctrlWidget(QWidget *parent) : QWidget(parent)
{
    bgColor = "#FFFFFF";
    borderColor = "#A6B5B8";
    borderRadius = 3;


    btn1 = new QPushButton;
    btn1->setText("预置");
    btn2 = new QPushButton;
    btn2->setText("执行");
    btn3 = new QPushButton;
    btn3->setText("撤销");

    QFrame *frame = new QFrame;
    frame->setObjectName("frameRMC");

    QStringList qss;
    qss.append(QString("QFrame#frameRMC{border:1px solid %1;border-radius:%2px;}").arg(borderColor).arg(borderRadius));
    qss.append(QString("QLabel{min-width:15px;background-color:%1;}").arg(bgColor));
    qss.append(QString("QLineEdit{background-color:%1;border:none;}").arg(bgColor));
    qss.append(QString("QLineEdit#txtIP1{border-top-left-radius:%1px;border-bottom-left-radius:%1px;}").arg(borderRadius));
    qss.append(QString("QLineEdit#txtIP4{border-top-right-radius:%1px;border-bottom-right-radius:%1px;}").arg(borderRadius));
    frame->setStyleSheet(qss.join(""));

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);
    verticalLayout->addWidget(frame);

    //将控件按照横向布局排列
    QHBoxLayout *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(btn1);
    layout->addWidget(btn2);
    layout->addWidget(btn3);

    btn1->setEnabled(true);
    btn2->setEnabled(false);
    btn3->setEnabled(false);

    connect(btn1,SIGNAL(clicked()),this,SLOT(execPreset()));
    connect(btn2,SIGNAL(clicked()),this,SLOT(execconfirm()));
    connect(btn3,SIGNAL(clicked()),this,SLOT(execCancel()));
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(handleTimeout()));
}

void dturmctrlWidget::execPreset()
{
    btn1->setEnabled(false);
    btn2->setEnabled(true);
    btn3->setEnabled(true);
    emit execCmd(0,fixid);
	if (delay_widg != nullptr)
		timer->start(delay_widg->value() * 1000);
	else
		timer->start(OutTime*1000);
}

void dturmctrlWidget::execconfirm()
{
    if(thisneedPre)
    {
        btn1->setEnabled(true);
        btn2->setEnabled(false);
        btn3->setEnabled(false);
        timer->stop();
    }
    emit execCmd(1,fixid);
}

void dturmctrlWidget::execCancel()
{
    if(thisneedPre)
    {
        btn1->setEnabled(true);
        btn2->setEnabled(false);
        btn3->setEnabled(false);
        timer->stop();
    }
    emit execCmd(2,fixid);
}

void dturmctrlWidget::setfixID(uint16_t fixno)
{
    this->fixid = fixno;
}

void dturmctrlWidget::setTimeout(uint16_t sec)
{
    this->OutTime = sec;
}

void dturmctrlWidget::setNeedPre(bool needPre)
{
    this->thisneedPre = needPre;
    if(!needPre)
    {
        btn1->setEnabled(false);
        btn2->setEnabled(true);
        btn3->setEnabled(true);
    }
}

void dturmctrlWidget::setDelayWidget(QSpinBox *widget)
{
	if (widget != nullptr)
		delay_widg = widget;
}

void dturmctrlWidget::handleTimeout()
{
    execCancel();
}
