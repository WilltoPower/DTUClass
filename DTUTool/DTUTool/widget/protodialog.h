#ifndef PROTODIALOG_H
#define PROTODIALOG_H

#include <QDialog>
#include <QComboBox>

#include "dtusystemconfig.h"

namespace Ui {
class ProtoDialog;
}

// 327 X 293 (W X H)
class ProtoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProtoDialog(QWidget *parent = 0);
    ~ProtoDialog();

public:
    // 设置规约
    void setProtoType(int type);
    // 从UI打包设置
    DTUCFG::DSYSCFG::StationCFG pack();
    // 将数据从配置解包到界面
    void unpack(DTUCFG::DSYSCFG::StationCFG &cfg);
    // 设置界面编辑有效性
    //void setEnabled(bool flag);

private:
    Ui::ProtoDialog *ui;

private:
    void load_ui();
    int prototype = 104;

private:
    bool eventFilter(QObject *target, QEvent *event);
    // combobox设置文字居中
    void setAlignCenter(QComboBox* widget);

private slots:
    void check_zip_click(int state);
    void check_char_click(int state);

};

#endif // PROTODIALOG_H
