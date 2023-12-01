#pragma once

#include <QWidget>
#include "ui_dtuexportwidget.h"

#include <QTableWidget>

#include "pugixml/pugixml.hpp"
#include "dtudbmanager.h"

using namespace DTU;

QT_BEGIN_NAMESPACE
namespace Ui { class dtuexportwidgetClass; };
QT_END_NAMESPACE

class dtuexportwidget : public QWidget
{
	Q_OBJECT

public:
	dtuexportwidget(uint16_t type, QString desc, int max, uint16_t exporttype, QWidget *parent = nullptr);
	~dtuexportwidget();

private:
	Ui::dtuexportwidgetClass *ui;

public:
	enum EXPORTER {
		EXPREPORT,
		EXPREPORTFILE,
		EXPCSPROTO,
	};

	enum FileType {
		FilePro,
		FileCFG,
	};

private:
	EXPORTER type;
	uint16_t exporttype;

private slots:
	void export_all();
	void export_report(int min,int max, int type,std::string desc,QString path);
	void export_report_file(int min, int max, int type, std::string desc, QString path);
	void export_csproto(int min,int max,int type, std::string desc,QString path);

private:
	// 添加SOE记录
	void addtoSOE(pugi::xml_node &node,DTU::buffer &data,int no);
	// 添加操作记录
	void addtoOPT(pugi::xml_node &node, DTU::buffer &data, int no);
	// 添加告警记录
	void addtoWAR(pugi::xml_node &node, DTU::buffer &data, int no);

private:
	void get_file(DTU::buffer &data, std::string armpath, std::vector<std::string> &suffix, std::string path);
	bool get_file_from_arm(std::string armPath, std::string localPath, std::string fileName, std::vector<std::string> suffix, QString &FullPath);
	QString SelestSavePath(int type);
	bool create_dir(QString fullpath);

public:
	void setWidgetPointer(QTableWidget *csprotoWidget);
	void addtoCSProto(pugi::xml_node &node, QTableWidget *table, int row);

private:
	// 规约窗口指针
	QTableWidget *csprotoWidget = nullptr;
};
