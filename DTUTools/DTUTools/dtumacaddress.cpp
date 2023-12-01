#include "dtumacaddress.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qboxlayout.h"
#include "qregexp.h"
#include "qvalidator.h"
#include "qevent.h"
#include "qdebug.h"

#define SeparatorChar ":"

MacAddress::MacAddress(QWidget *parent) : QWidget(parent)
{
    bgColor = "#FFFFFF";
    borderColor = "#A6B5B8";
    borderRadius = 3;

    //用于显示小圆点的标签,居中对齐
    labDot1 = new QLabel;
    labDot1->setAlignment(Qt::AlignCenter);
    labDot1->setText(SeparatorChar);

    labDot2 = new QLabel;
    labDot2->setAlignment(Qt::AlignCenter);
    labDot2->setText(SeparatorChar);

    labDot3 = new QLabel;
    labDot3->setAlignment(Qt::AlignCenter);
    labDot3->setText(SeparatorChar);

    labDot4 = new QLabel;
    labDot4->setAlignment(Qt::AlignCenter);
    labDot4->setText(SeparatorChar);

    labDot5 = new QLabel;
    labDot5->setAlignment(Qt::AlignCenter);
    labDot5->setText(SeparatorChar);

    //用于输入IP地址的文本框,居中对齐
    txtMac1 = new QLineEdit;
    txtMac1->setObjectName("txtMac1");
    txtMac1->setAlignment(Qt::AlignCenter);
    txtMac1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac1, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    txtMac2 = new QLineEdit;
    txtMac2->setObjectName("txtMac2");
    txtMac2->setAlignment(Qt::AlignCenter);
    txtMac2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac2, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    txtMac3 = new QLineEdit;
    txtMac3->setObjectName("txtMac3");
    txtMac3->setAlignment(Qt::AlignCenter);
    txtMac3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac3, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    txtMac4 = new QLineEdit;
    txtMac4->setObjectName("txtMac4");
    txtMac4->setAlignment(Qt::AlignCenter);
    txtMac4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac4, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    txtMac5 = new QLineEdit;
    txtMac5->setObjectName("txtMac5");
    txtMac5->setAlignment(Qt::AlignCenter);
    txtMac5->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac5, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    txtMac6 = new QLineEdit;
    txtMac6->setObjectName("txtMac6");
    txtMac6->setAlignment(Qt::AlignCenter);
    txtMac6->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(txtMac6, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    
    //设置IP地址校验过滤
    //QString pattern = "(2[0-5]{2}|2[0-4][0-9]|1?[0-9]{1,2})";
    QString pattern = "^([0-9A-Fa-f]{2})";
    //确切的说 QRegularExpression QRegularExpressionValidator 从5.0 5.1开始就有
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
    QRegularExpression regExp(pattern);
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);
#else
    QRegExp regExp(pattern);
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
#endif

    txtMac1->setValidator(validator);
    txtMac2->setValidator(validator);
    txtMac3->setValidator(validator);
    txtMac4->setValidator(validator);
    txtMac5->setValidator(validator);
    txtMac6->setValidator(validator);

    //绑定事件过滤器,识别键盘按下
    txtMac1->installEventFilter(this);
    txtMac2->installEventFilter(this);
    txtMac3->installEventFilter(this);
    txtMac4->installEventFilter(this);
    txtMac5->installEventFilter(this);
    txtMac6->installEventFilter(this);

    QFrame *frame = new QFrame;
    frame->setObjectName("frameMac");

    QStringList qss;
    qss.append(QString("QFrame#frameMac{border:1px solid %1;border-radius:%2px;}").arg(borderColor).arg(borderRadius));
    qss.append(QString("QLabel{min-width:15px;background-color:%1;}").arg(bgColor));
    qss.append(QString("QLineEdit{background-color:%1;border:none;}").arg(bgColor));
    qss.append(QString("QLineEdit#txtMac1{border-top-left-radius:%1px;border-bottom-left-radius:%1px;}").arg(borderRadius));
    qss.append(QString("QLineEdit#txtMac4{border-top-right-radius:%1px;border-bottom-right-radius:%1px;}").arg(borderRadius));
    frame->setStyleSheet(qss.join(""));

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);
    verticalLayout->addWidget(frame);

    //将控件按照横向布局排列
    QHBoxLayout *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(txtMac1);
    layout->addWidget(labDot1);
    layout->addWidget(txtMac2);
    layout->addWidget(labDot2);
    layout->addWidget(txtMac3);
    layout->addWidget(labDot3);
    layout->addWidget(txtMac4);
    layout->addWidget(labDot4);
    layout->addWidget(txtMac5);
    layout->addWidget(labDot5);
    layout->addWidget(txtMac6);

	// 修改MAC可修改区
	txtMac1->setEnabled(false);
	txtMac2->setEnabled(false);
	txtMac3->setEnabled(false);
	txtMac4->setEnabled(false);
	txtMac5->setEnabled(false);
}

bool MacAddress::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QLineEdit *txt = (QLineEdit *)watched;
        if (txt == txtMac1 || txt == txtMac2 || txt == txtMac3 || txt == txtMac4 || txt == txtMac5 || txt == txtMac6) {
            QKeyEvent *key = (QKeyEvent *)event;

            //如果当前按下了小数点则移动焦点到下一个输入框
            if (key->text() == SeparatorChar) {
                this->focusNextChild();
            }

            //如果按下了退格键并且当前文本框已经没有了内容则焦点往前移
            if (key->key() == Qt::Key_Backspace) {
                if(txt != txtMac1) {
                    if (txt->text().length() <= 1) {
                        this->focusNextPrevChild(false);
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void MacAddress::textChanged(const QString &text)
{
    QObject* lSender = sender();
	if (lSender == nullptr)
		return;
    if (lSender->objectName() == "txtMac1") {
        txtMac1->setText(txtMac1->text().toUpper());
    }
    else if (lSender->objectName() == "txtMac2") {
        txtMac2->setText(txtMac2->text().toUpper());
    }
    else if (lSender->objectName() == "txtMac3") {
        txtMac3->setText(txtMac3->text().toUpper());
    }
    else if (lSender->objectName() == "txtMac4") {
        txtMac4->setText(txtMac4->text().toUpper());
    }
    else if (lSender->objectName() == "txtMac5") {
        txtMac5->setText(txtMac5->text().toUpper());
    }
    else if (lSender->objectName() == "txtMac6") {
        txtMac6->setText(txtMac6->text().toUpper());
    }

//    int len = text.length();
//    bool ok;
//    int value = text.toInt(&ok,16);

//    //判断当前是否输入完成一个网段,是的话则自动移动到下一个输入框
//    if (len == 2) {
//        if (value >= 0x10 && value <= 0xFF) {
//            this->focusNextChild();
//        }
//    }

    //拼接成完整IP地址
    mac = QString("%1:%2:%3:%4:%5:%6").arg(txtMac1->text(), 2, QLatin1Char('0'))
                                        .arg(txtMac2->text(), 2, QLatin1Char('0'))
                                        .arg(txtMac3->text(), 2, QLatin1Char('0'))
                                        .arg(txtMac4->text(), 2, QLatin1Char('0'))
                                        .arg(txtMac5->text(), 2, QLatin1Char('0'))
                                        .arg(txtMac6->text(), 2, QLatin1Char('0'));
    emit MacChanged(mac);
}

QString MacAddress::getMac() const
{
    return this->mac;
}

void MacAddress::setEnabled(bool status) const
{
	txtMac1->setEnabled(status);
	txtMac2->setEnabled(status);
	txtMac3->setEnabled(status);
	txtMac4->setEnabled(status);
    txtMac5->setEnabled(status);
    txtMac6->setEnabled(status);
}

QSize MacAddress::sizeHint() const
{
    return QSize(250, 20);
}

QSize MacAddress::minimumSizeHint() const
{
    return QSize(30, 10);
}

void MacAddress::setMac(const QString &mac)
{
    //先检测Mac地址是否合法
    // QRegExp regExp("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
    QRegExp regExp("^([0-9A-F]{2}:?){6}$");
	if (!regExp.exactMatch(mac)) {
		return;
	}



    if (this->mac != mac) {
        this->mac = mac;

        //将Mac地址填入各个网段
        QStringList list = mac.split(SeparatorChar);
        txtMac1->setText(list.at(0));
        txtMac2->setText(list.at(1));
        txtMac3->setText(list.at(2));
        txtMac4->setText(list.at(3));
        txtMac5->setText(list.at(4));
        txtMac6->setText(list.at(5));
    }
}

void MacAddress::clear()
{
    txtMac1->clear();
    txtMac2->clear();
    txtMac3->clear();
    txtMac4->clear();
    txtMac5->clear();
    txtMac6->clear();
    txtMac1->setFocus();
}

void MacAddress::setBgColor(const QString &bgColor)
{
    if (this->bgColor != bgColor) {
        this->bgColor = bgColor;
    }
}

void MacAddress::setBorderColor(const QString &borderColor)
{
    if (this->borderColor != borderColor) {
        this->borderColor = borderColor;
    }
}

void MacAddress::setBorderRadius(int borderRadius)
{
    if (this->borderRadius != borderRadius) {
        this->borderRadius = borderRadius;
    }
}
