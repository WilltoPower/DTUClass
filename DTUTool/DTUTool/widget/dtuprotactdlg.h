#pragma once

#include <QDialog>
#include <dtubuffer.h>
#include "ui_dtuprotactdlg.h"

class dtuprotactdlg : public QDialog
{
	Q_OBJECT

public:
	dtuprotactdlg(QWidget *parent = Q_NULLPTR);
	~dtuprotactdlg();
	
	void parse(const DTU::buffer& data);

	void display_act();

public slots:
	void show_protect_detail(QTableWidgetItem*);
private:
	Ui::dtuprotactdlg ui;

	std::vector<DTU::buffer> _prot_act;

	uint32_t _select_item = 0;
};
