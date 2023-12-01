#pragma once

#include <QWidget>
#include "ui_dtucsprotocolwidget.h"
#include "dtubuffer.h"

#include "dtucsprotocolexplain.h"
#include "dtuexportwidget.h"

class dtucsprotocolwidget : public QWidget
{
	Q_OBJECT

public:
	dtucsprotocolwidget(QWidget *parent = Q_NULLPTR);
	~dtucsprotocolwidget();
private:
	Ui::dtucsprotocolwidget *ui;
public slots:
	void update_info(QByteArray data);
	void parase(DTU::buffer &data);
	void clear_101();
	void clear_104();
	void AutoScroll_101();
	void AutoScroll_104();
private:
	uint64_t CS101_index = 0;
	uint64_t CS104_index = 0;
	bool scroolflag_101 = false;
	bool scroolflag_104 = false;
	void load_ui(QTableWidget *widget);
	void init_widget(QTableWidget *widget);
	void update_log(uint64_t msec,uint8_t protofrom,std::string log);
	void update_msg(uint64_t msec, uint8_t protofrom,bool sent,uint8_t *msg,uint32_t msgsize);
	QString analysisMessage(uint8_t flag,uint8_t *msg,uint32_t msgsize);
	void tableWidgetInsert(QTableWidget *widget,bool scroll,uint64_t &index,QString dir, uint32_t size,QString time,QString data,QString desc);
	void setOneWidget(QTableWidget *widget,uint64_t index,int no,QString str, QColor color);
//-> 规约配置部分
private:
	void load_ui();
	void load_widget(QTableWidget *widget, int row);
	void addItemToTableWidget(QTableWidget *twidget, int row, int column, QWidget *widget);
private slots:
	void read_config();
	void save_config();
	void default_config();
	void updatecfg(int index);
private:
	int TransParityToindex(char parity);
	char TransIndexToParity(int index);
	template<typename _Type>
	_Type *GetItem(QTableWidget *table, int row)
	{
		return (_Type*)(table->cellWidget(row, 1));
	}

//-> 报文解析部分
private slots:
	void explain_101(int row,int column);
	void explain_104(int row,int column);
	void explain_csproto(int type,QString &text, QString time, QString proto, QString dir, QString size);

private:
	dtucsprotocolexplain *explainWidget = nullptr;

//-> 导出报文部分
private slots:
	void save_file_104();
	void save_file_101();
private:
	dtuexportwidget *exportWidget = nullptr;
};