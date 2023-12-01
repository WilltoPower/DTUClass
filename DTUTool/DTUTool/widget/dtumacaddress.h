#ifndef MACADDRESS_H
#define MACADDRESS_H

/**
 * 1. 可设置IP地址，自动填入框。
 * 2. 可清空IP地址。
 * 3. 支持按下小圆点自动切换。
 * 4. 支持退格键自动切换。
 * 5. 支持IP地址过滤。
 * 6. 可设置背景色、边框颜色、边框圆角角度。
 */

// HWaddr 4E:14:62:15:EC:19

#include <QWidget>

class QLabel;
class QLineEdit;

#ifdef quc
class Q_DECL_EXPORT MacAddress : public QWidget
#else
class MacAddress : public QWidget
#endif

{
    Q_OBJECT

    Q_PROPERTY(QString mac READ getMac WRITE setMac)
signals:
	// 信号:内容改变
	void MacChanged(const QString &text);
public:
    explicit MacAddress(QWidget *parent = 0);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QLabel *labDot1;    //第一个小圆点
    QLabel *labDot2;    //第二个小圆点
    QLabel *labDot3;    //第三个小圆点
    QLabel *labDot4;    //第四个小圆点
    QLabel *labDot5;    //第五个小圆点


    QLineEdit *txtMac1;  //IP地址网段输入框1
    QLineEdit *txtMac2;  //IP地址网段输入框2
    QLineEdit *txtMac3;  //IP地址网段输入框3
    QLineEdit *txtMac4;  //IP地址网段输入框4
    QLineEdit *txtMac5;  //IP地址网段输入框5
    QLineEdit *txtMac6;  //IP地址网段输入框6

    QString mac;        //IP地址
    QString bgColor;    //背景颜色
    QString borderColor;//边框颜色
    int borderRadius;   //边框圆角角度

private slots:
    void textChanged(const QString &text);

public:
    //获取IP地址
    QString getMac()                        const;
	void setEnabled(bool status = true)     const;

    QSize sizeHint()                        const;
    QSize minimumSizeHint()                 const;

public Q_SLOTS:
    //设置IP地址
    void setMac(const QString &mac);
    //清空
    void clear();

    //设置背景颜色
    void setBgColor(const QString &bgColor);
    //设置边框颜色
    void setBorderColor(const QString &borderColor);
    //设置边框圆角角度
    void setBorderRadius(int borderRadius);

};

#endif // IPADDRESS_H
