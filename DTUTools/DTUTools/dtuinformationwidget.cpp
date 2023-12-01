#include "dtuinformationwidget.h"
#include <QTableWidget>
#include <QLabel>

#include <bitset>

#include "create_control.h"
#include "dtutask.h"
#include "dtulog.h"

using namespace DTU;

dtuinformation::dtuinformation(QWidget *parent): QWidget(parent)
	, ui(new Ui::dtuinformationClass())
{
	ui->setupUi(this);
}

dtuinformation::~dtuinformation()
{
	delete ui;
}

// 创建TableWidget控件
QTableWidget *dtuinformation::createTabWidget(QTableWidget *itable, int row, QStringList header)
{
	// 设置行数
	itable->setRowCount(row);
	// 设置列数
	itable->setColumnCount(header.size());
	// 设置表头
	itable->setHorizontalHeaderLabels(header);

	// ui修改
	itable->horizontalHeader()->setStretchLastSection(true);// 自动延展
	itable->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置为不可修改
	//ptable->horizontalHeader()->setVisible(false);// 隐藏水平表头
	itable->verticalHeader()->setVisible(false);    // 隐藏垂直表头
	itable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);			//先自适应宽度
	itable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);//然后设置要根据内容使用宽度的列
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	return itable;
}

void dtuinformation::setCurWidgetInfoID(uint16_t infoid, DTU::dtuInfoTable &table)
{
	infoID = infoid;
	infoindex = table;
	load_ui();
}

uint16_t dtuinformation::curInfoID()
{
	return infoID;
}

void dtuinformation::load_ui()
{
	// 加载标题
	QStringList header;
	header << "序号" << "点表值" << "描述" << "值";

	auto iinfo = infoindex[infoID];
	QTableWidget *itable = createTabWidget(ui->tableWidget,iinfo.info.size(), header);
	int index = 1;
	for (const auto &item : iinfo.info)
	{
		itable->setCellWidget(index - 1, 0, createStrLabel(QString::number(index)));//序号
		itable->setCellWidget(index - 1, 1, createStrLabel("0x" + QString("%1").arg(item.second.fixid, 2, 16, QLatin1Char('0')).toUpper()));// 点表
		itable->setCellWidget(index - 1, 2, createStrLabel(QString::fromStdString(item.second.desc)));// 描述
		itable->setCellWidget(index - 1, 3, createControl(item.second.qtctrl, {
			item.second.desc,0,0,item.second.defaul,1,std::string("")
			}, false));// 值
		index++;
	}
}

bool dtuinformation::checkInfoDateSize(uint16_t infoID, int size)
{
	return (infoindex[infoID].size == size) ? true : false;
}

void dtuinformation::updateinfo(unsigned short infoID, QByteArray data)
{
	if (infoID != this->infoID)
		return;
	// 检查长度 长度错误直接返回
	if (!checkInfoDateSize(infoID, data.size()))
		return;
	DTU::buffer buff(data.data(), data.size());
	switch (infoID)
	{
		// 硬件遥信
	case 0: {
		uint32_t hyx = buff.value<uint32_t>();
		std::bitset<32> hyxbits(hyx);
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!hyxbits[item.second.offset])
				str = "0";
			setControlValue(item.second.qtctrl, str.c_str(), str.size(),
				ui->tableWidget->cellWidget(item.second.offset, 3));
		}
	}break;
		// 遥测值
	case 1: {
		int index = 0;
		for (const auto &item : infoindex[infoID].info)
		{
			float value = buff.get(item.second.offset, item.second.size).value<float>();
			std::string svalue = QString::number(value, 'f', 3).toStdString();
			setControlValue(item.second.qtctrl, svalue.c_str(), svalue.size(),
				ui->tableWidget->cellWidget(index, 3));
			index++;
		}
	}break;
		// 自检状态
	case 2: {
		std::string bin_str;
		uint8_t bitNum = 0;
		for (int i = 0; i < buff.size(); i++)
		{
			bitNum = buff.get(i, sizeof(bitNum)).value<uint8_t>();
			for (int j = 0; j < 8; j++)
			{
				if ((0x01 << j) & bitNum)
					bin_str = "1" + bin_str;
				else
					bin_str = "0" + bin_str;
			}
		}
		// 这里大于自检信息的位数即可
		std::bitset<272> selfcheckbits(bin_str);
		int widgetIndex = 0;
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!selfcheckbits[item.second.offset])
				str = "0";
			setControlValue(item.second.qtctrl, str.c_str(), str.size(),
				ui->tableWidget->cellWidget(widgetIndex, 3));
			widgetIndex++;
		}
	}break;
		// 状态信息
	case 3: {
		uint32_t hyx = buff.value<uint32_t>();
		std::bitset<32> statebits(hyx);
		for (const auto &item : infoindex[infoID].info)
		{
			std::string str = "1";
			if (!statebits[item.second.offset])
				str = "0";
			setControlValue(item.second.qtctrl, str.c_str(), str.size(),
				ui->tableWidget->cellWidget(item.second.offset, 3));
		}
	}break;
	default: {
		QtDTULOG(DTU_ERROR, "未知的信息查看编号[%04d]", infoID);
	}
	}
}