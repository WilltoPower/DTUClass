#include "serialdialog.h"
#include "ui_serialdialog.h"

#include <QStandardItemModel>
#include <QLineEdit>

#include <map>

using namespace DTUCFG;

SerialDialog::SerialDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SerialDialog)
{
    ui->setupUi(this);
    load_ui();
}

SerialDialog::~SerialDialog()
{
    delete ui;
}

SerialCFG SerialDialog::pack()
{
    SerialCFG cfg;
    cfg.name = ui->combobox_name->currentText().toStdString();          // 串口名称
    cfg.baudrate = ui->combo_baud->currentText().toInt();               // 波特率
    cfg.databits = ui->spin_data->value();                              // 数据位
    cfg.pairty = checkIndexToChar(ui->combo_pairty->currentIndex());    // 校验方式
    cfg.stopbits = ui->spin_stop->value();                              // 停止位
    return cfg;
}

void SerialDialog::unpack(SerialCFG& cfg)
{
    ui->combobox_name->setCurrentIndex(this->serialToBoard(cfg.name));  // 串口名称
    ui->combo_baud->setCurrentText(QString::number(cfg.baudrate));      // 波特率
    ui->spin_data->setValue(cfg.databits);                              // 数据位
    ui->combo_pairty->setCurrentIndex(checkCharToIndex(cfg.pairty));    // 校验方式
    ui->spin_stop->setValue(cfg.stopbits);                              // 停止位
}

void SerialDialog::load_ui()
{
    this->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    this->setFixedSize(220,240);// 设置固定窗口大小
    // 屏蔽滚轮事件
    //ui->spinBox->installEventFilter(this);
    ui->combobox_name->installEventFilter(this);
    ui->combo_baud->installEventFilter(this);
    ui->combo_pairty->installEventFilter(this);
    ui->spin_data->installEventFilter(this);
    ui->spin_stop->installEventFilter(this);
    // combobox居中
    this->setAlignCenter(ui->combobox_name);
    this->setAlignCenter(ui->combo_baud);
    this->setAlignCenter(ui->combo_pairty);
}

bool SerialDialog::eventFilter(QObject *target, QEvent *event)
{
    //屏蔽鼠标滚轮事件
    if(event->type() == QEvent::Wheel&&(target->inherits("QComboBox")||target->inherits("QSpinBox"))) {
        return true;
    }
    return false;
}

// "串口名称" + UI索引
static std::map<std::string, int> boardIndex = {
    {"ttyS3", 0}, {"ttyS5", 3},
    {"ttyS1", 1}, {"ttyS6", 4},
    {"ttyS2", 2}, {"ttyS4", 5},
};

int SerialDialog::serialToBoard(std::string &devname)
{
    int result = -1;
    auto ita = boardIndex.find(devname);
    if(ita != boardIndex.end()) {
        result = ita->second;
    }
    return result;
}

std::string SerialDialog::boardToSerial(int index)
{
    std::string result;
    for(auto &item : boardIndex) {
        if(item.second == index) {
            result = item.second;
        }
    }
    return result;
}

int SerialDialog::checkCharToIndex(char check)
{
    int result = 0; // 默认无校验
    switch (check) {
        case 'n':
        case 'N':result = 0;break;
        case 'o':
        case 'O':result = 1;break;
        case 'e':
        case 'E':result = 2;break;
    }
    return result;
}

char SerialDialog::checkIndexToChar(int index)
{
    char result = 'N';
    switch (index) {
        case 0:result = 'N';break;
        case 1:result = 'O';break;
        case 2:result = 'E';break;
    }
    return result;
}

void SerialDialog::setAlignCenter(QComboBox *widget)
{
    widget->setEditable(true);
    widget->lineEdit()->setReadOnly(true);
    widget->lineEdit()->setAlignment(Qt::AlignCenter);
    for(int pos=0;pos<widget->count();pos++) {
        static_cast<QStandardItemModel*>(widget->model())->item(pos)->setTextAlignment(Qt::AlignCenter);
    }
}
