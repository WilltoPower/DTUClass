#include "protodialog.h"
#include "ui_protodialog.h"

#include <QLineEdit>
#include <QStandardItemModel>

using namespace DTUCFG;

ProtoDialog::ProtoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProtoDialog)
{
    ui->setupUi(this);
    this->load_ui();
}

ProtoDialog::~ProtoDialog()
{
    delete ui;
}

void ProtoDialog::setProtoType(int type)
{
    if(type == 101) {
        ui->tabWidget->setTabEnabled(1, true);
        prototype = type;
    }
    else {
        ui->tabWidget->setTabEnabled(1, false);
    }
}

DTUCFG::DSYSCFG::StationCFG ProtoDialog::pack()
{
    DSYSCFG::StationCFG cfg;
    if(prototype == 101) {
        // 通用配置
        cfg.CS101.VType.MeasuredValueType = static_cast<MEASURED_TYPE>(ui->combo_yc->currentIndex());
        cfg.CS101.VType.TelegramValueType = static_cast<TELEGRAM_TYPE>(ui->combo_yx->currentIndex());
        cfg.CS101.ALParam.sizeofCA = ui->spin_ca->value();
        cfg.CS101.ALParam.sizeofCOT = ui->spin_cot->value();
        cfg.CS101.ALParam.sizeofIOA = ui->spin_ioa->value();
        cfg.CS101.EXParam.isSequence = ui->check_zip->isChecked();
        // 链路层配置
        cfg.CS101.LLParam.SingalCharACK = ui->check_char->isChecked();
        cfg.CS101.LLParam.TimeoutForACK = ui->spin_ack->value();
        cfg.CS101.LLParam.TimeoutForRepeat = ui->spin_ack_timeout->value();
        cfg.CS101.LLParam.LinkAddrLength = ui->spin_llength->value();
    }
    else {
        // 通用配置
        cfg.CS104.VType.MeasuredValueType = static_cast<MEASURED_TYPE>(ui->combo_yc->currentIndex());
        cfg.CS104.VType.TelegramValueType = static_cast<TELEGRAM_TYPE>(ui->combo_yx->currentIndex());
        cfg.CS104.ALParam.sizeofCA = ui->spin_ca->value();
        cfg.CS104.ALParam.sizeofCOT = ui->spin_cot->value();
        cfg.CS104.ALParam.sizeofIOA = ui->spin_ioa->value();
        cfg.CS104.EXParam.isSequence = ui->check_zip->isChecked();
    }
    return cfg;
}

void ProtoDialog::unpack(DTUCFG::DSYSCFG::StationCFG &cfg)
{
    if(prototype == 101) {
        // 通用配置
        ui->combo_yc->setCurrentIndex(static_cast<int>(cfg.CS101.VType.MeasuredValueType));
        ui->combo_yx->setCurrentIndex(static_cast<int>(cfg.CS101.VType.TelegramValueType));
        ui->spin_ca->setValue(cfg.CS101.ALParam.sizeofCA);
        ui->spin_cot->setValue(cfg.CS101.ALParam.sizeofIOA);
        ui->spin_ioa->setValue(cfg.CS101.ALParam.sizeofIOA);
        ui->check_zip->setChecked(cfg.CS101.EXParam.isSequence);
        // 链路层配置
        ui->check_char->setCheckable(cfg.CS101.LLParam.SingalCharACK);
        ui->spin_ack->setValue(cfg.CS101.LLParam.TimeoutForACK);
        ui->spin_ack_timeout->setValue(cfg.CS101.LLParam.TimeoutForRepeat);
        ui->spin_llength->setValue(cfg.CS101.LLParam.LinkAddrLength);
    }
    else {
        // 通用配置
        ui->combo_yc->setCurrentIndex(cfg.CS104.VType.MeasuredValueType);
        ui->combo_yx->setCurrentIndex(cfg.CS104.VType.TelegramValueType);
        ui->spin_ca->setValue(cfg.CS104.ALParam.sizeofCA);
        ui->spin_cot->setValue(cfg.CS104.ALParam.sizeofIOA);
        ui->spin_ioa->setValue(cfg.CS104.ALParam.sizeofIOA);
        ui->check_zip->setChecked(cfg.CS104.EXParam.isSequence);
    }
}

void ProtoDialog::load_ui()
{
    this->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    this->setFixedSize(327, 293);// 设置固定窗口大小
    // 屏蔽鼠标滚轮事件
    ui->combo_yc->installEventFilter(this);
    ui->combo_yx->installEventFilter(this);
    ui->spin_ca->installEventFilter(this);
    ui->spin_cot->installEventFilter(this);
    ui->spin_ioa->installEventFilter(this);
    ui->spin_ack->installEventFilter(this);
    ui->spin_ack_timeout->installEventFilter(this);

    // 设置Combobox居中
    this->setAlignCenter(ui->combo_yc);
    this->setAlignCenter(ui->combo_yx);


}

bool ProtoDialog::eventFilter(QObject *target, QEvent *event)
{
    //屏蔽鼠标滚轮事件
    if(event->type() == QEvent::Wheel&&(target->inherits("QComboBox")||target->inherits("QSpinBox"))) {
        return true;
    }
    return false;
}

void ProtoDialog::setAlignCenter(QComboBox *widget)
{
    widget->setEditable(true);
    widget->lineEdit()->setReadOnly(true);
    widget->lineEdit()->setAlignment(Qt::AlignCenter);
    for(int pos=0;pos<widget->count();pos++) {
        static_cast<QStandardItemModel*>(widget->model())->item(pos)->setTextAlignment(Qt::AlignCenter);
    }
}

void ProtoDialog::check_zip_click(bool state)
{
    if(state) {
        ui->check_zip->setText("启用压缩格式");
    }
    else {
        ui->check_zip->setText("禁用压缩格式");
    }
}

void ProtoDialog::check_char_click(bool state)
{
    if(state) {
        ui->check_char->setText("启用单字节回复");
    }
    else {
        ui->check_char->setText("禁用单字节回复");
    }
}

