#pragma once

#include <QWidget>
#include "ui_dturulefilewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class dturulefilewidgetClass; };
QT_END_NAMESPACE

class dturulefilewidget : public QWidget
{
	Q_OBJECT

public:
	dturulefilewidget(QWidget *parent = nullptr);
	~dturulefilewidget();

private:
	Ui::dturulefilewidgetClass *ui;

public:
	void load_first();
private:
	void load_ui();
	QStringList load_header();
	std::string indextoLinuxPath();
	std::string indextoWindowsPath();
private slots:
	void updatelist();
	void showfile(int row, int column);
	void get_arm_file_lists(std::string path);
	void get_arm_file(std::string filename);
private:
	uint16_t historyID = 0xFF;
};
