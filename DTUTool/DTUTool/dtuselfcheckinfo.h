#pragma once

#include <QWidget>
#include "ui_dtuselfcheckinfo.h"

#include <bitset>

QT_BEGIN_NAMESPACE
namespace Ui { class dtuselfcheckinfoClass; };
QT_END_NAMESPACE

class dtuselfcheckinfo : public QWidget
{
	Q_OBJECT

public:
	dtuselfcheckinfo(QWidget *parent = nullptr);
	~dtuselfcheckinfo();

public:
	void setinfo(const std::bitset<192>& data);

private:
	void load_ui();

private:
	Ui::dtuselfcheckinfoClass *ui;
	std::bitset<192> info;
};
