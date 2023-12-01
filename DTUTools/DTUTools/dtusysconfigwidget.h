#pragma once

#include <QWidget>
#include "ui_dtusysconfigwidget.h"
#include <map>
#include <dtubuffer.h>
class DTUSysConfigWidget : public QWidget
{
	Q_OBJECT

public:
	DTUSysConfigWidget(QWidget *parent = Q_NULLPTR);
	~DTUSysConfigWidget();
signals:
	void change_status(bool status);
	// 修改RPC连接地址时
	void connect_change();
public slots:
	// 连接状态
	void connect_status(bool status);
	// GOOSE状态
	void goose_status(QByteArray status);
	// 磁盘容量
	void get_disk_usage();
	// RPC通信配置
	void connect_dev();
private:
	void load_ui();
private:
	Ui::DTUSysConfigWidget ui;
	std::atomic_bool _bStop;
};
