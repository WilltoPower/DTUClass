#ifndef DTU_RMCTRL_WIDGET
#define DTU_RMCTRL_WIDGET

#include <QWidget>

class QSpinBox;
class QTimer;
class QPushButton;

#ifdef quc
class Q_DECL_EXPORT dturmctrlWidget : public QWidget
#else
class dturmctrlWidget : public QWidget
#endif

{
    Q_OBJECT

    //Q_PROPERTY(QString ip READ getIP WRITE setIP)
signals:
    // 执行命令
    void execCmd(int cmd,uint16_t fixno);
public:
    explicit dturmctrlWidget(QWidget *parent = 0);

private:
    QPushButton *btn1 = nullptr;    // 第一个按钮
    QPushButton *btn2 = nullptr;    // 第二个按钮
    QPushButton *btn3 = nullptr;    // 第三个按钮

    QString bgColor;    //背景颜色
    QString borderColor;//边框颜色
    int borderRadius;   //边框圆角角度

private slots:
    void execPreset();  //预设
    void execconfirm(); //执行
    void execCancel();  //取消
    void handleTimeout();//超时处理
public:
    // 设置点表号
    void setfixID(uint16_t fixno);
    // 设置超时时间
    void setTimeout(uint16_t sec);
    // 设置是否需要预设
    void setNeedPre(bool needPre);
	// 设置获取时间延时控件
	void setDelayWidget(QSpinBox *widget);
private:
    // 设置控件遥控点表
    uint16_t fixid = 0;
    // 超时时间
    uint16_t OutTime = 20;
    // 是否需要预设
    bool thisneedPre = false;
    // 是否需要下发
    QTimer *timer = nullptr;
	QSpinBox *delay_widg = nullptr;
};

#endif
