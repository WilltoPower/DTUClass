#pragma once

#include <QDialog>
#include "ui_dtuversiondlg.h"

class dtuversiondlg : public QDialog
{
	Q_OBJECT

public:
	dtuversiondlg(QWidget *parent = Q_NULLPTR);
	~dtuversiondlg();
public slots:
	void update_version();
private:
	Ui::dtuversiondlg ui;
};
