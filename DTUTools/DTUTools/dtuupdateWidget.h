#pragma once

#include "ui_dtuupdateWidget.h"
#include <QWidget>

class dtuupdateWidget : public QWidget
{
	Q_OBJECT

public:
	dtuupdateWidget(QWidget *parent = Q_NULLPTR);
	~dtuupdateWidget();
public slots:
	void enable_update();
	void select_dsp_program();
	void select_system_program();
	void select_arm_program();
	void select_goose_program();
	void select_config_program();
	void execute_update_program();
	bool translateFile(const std::string& src, const std::string destfile);
public:
	QString _title;
	QString _paramFile;
private:
	QString _path_arm;
	QString _path_goose;
	QStringList _path_dsp;
	std::vector<std::tuple<QString, QString>> _path_dsp_list;
	QStringList _path_system;
	std::vector<std::tuple<QString, QString>> _path_system_list;
	QStringList _path_config;
	// 文件全路径  文件名
	std::vector<std::tuple<QString, QString>> _path_config_list;

	Ui::dtuupdateWidget ui;
};
