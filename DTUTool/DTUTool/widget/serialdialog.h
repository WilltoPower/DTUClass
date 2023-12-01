#ifndef SERIALDIALOG_H
#define SERIALDIALOG_H

#include <QDialog>
#include <QComboBox>

#include <string>

#include "dtusystemconfig.h"

namespace Ui {
class SerialDialog;
}

// 240 X 240 (W X H)
class SerialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerialDialog(QWidget *parent = 0);
    ~SerialDialog();

private:
    Ui::SerialDialog *ui;

public:
    // 从UI界面打包数据
    DTUCFG::SerialCFG pack();
    // 从结构体解析数据到UI界面
    void unpack(DTUCFG::SerialCFG& cfg);

private:
    void load_ui();
    bool eventFilter(QObject *target, QEvent *event);

private:
    // Linux串口名称转后面板值
    int serialToBoard(std::string &devname);
    // 后面板值转Linux串口名称
    std::string boardToSerial(int index);

    // Linux校验方式转显示
    int checkCharToIndex(char check);
    // 显示转Linux校验方式
    char checkIndexToChar(int index);

    // combobox设置文字居中
    void setAlignCenter(QComboBox* widget);

private slots:
	void widgetChange();

};

#endif // SERIALDIALOG_H
