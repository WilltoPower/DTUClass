#pragma once

#include <QWidget>
#include <QLabel>
#include <thread>
#include <bitset>

#include "ui_dtuselfcheckprase.h"
#include "dtuselfcheckinfo.h"

#include "pugixml/pugixml.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class dtuselfcheckpraseClass; };
QT_END_NAMESPACE

class dtuselfcheckprase : public QWidget
{
	Q_OBJECT

public:
	dtuselfcheckprase(QWidget *parent = nullptr);
	~dtuselfcheckprase();

public:
	void load_ui();
	void load_file(const std::string& fullPath);

signals:
	void updateprocessvalue(int value);
	void setLabelColor(int row, int column, bool state);
	void selfcheckpraseover();

private slots:
	void updateProcess(int value);
	void updatePage(int pageno);
	std::string strHexToBin(const std::string& hexstr);
	void setColor(int row, int column, bool state);
	void initover();
	void doubleclick(QModelIndex index);

private:
	struct allselfcheckinfo {
		allselfcheckinfo() {}
		allselfcheckinfo(const std::string& time, const std::string& bin) : tm(time), state(bin) {}
		std::string tm;
		std::bitset<192> state;
	};

	std::map<int, allselfcheckinfo> info;

private:
	Ui::dtuselfcheckpraseClass *ui;
	pugi::xml_document doc;
	pugi::xml_node root; 
	std::thread *findThread = nullptr;
	std::map<int, std::map<int, QLabel*>> labelmap;
	dtuselfcheckinfo *prasewidget = nullptr;
};
