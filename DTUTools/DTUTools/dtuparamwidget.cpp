﻿#include "dtuparamwidget.h"
#include <QTableWidget>
#include <QLabel>

#include "dtutask.h"
#include "create_control.h"
#include "dtulog.h"
#include "dtutask.h"

using namespace DTU;

static std::vector<std::tuple<int, int, int>> RGBVec = {
	{255, 255, 224},//  黄
	{240, 255, 255},// 	Azure	240 255 255
	{180, 205, 205},//  LightCyan3	180 205 205	
	{211, 211, 211},// 	LightGray	211 211 211
	{255, 228, 181},// 	Moccasin	255 228 181
	{248, 248, 255},//  GhostWhite	248 248 255
	{95,  158, 160},//  CadetBlue	95 158 160
	{255, 245, 238},//  Seashell1	255 245 238
	{255, 222, 173},//  NavajoWhite1	255 222 173
	{192, 226, 255},// 	SlateGray1	198 226 255
	{255, 248, 220},//  Cornsilk1	255 248 220
};

void setColor(bool state, QLabel *label)
{
	static int index = 0;
	if (state) {
		index++;
	}
	if (index + 1 > RGBVec.size()) {
		index = index % RGBVec.size();
	}
	label->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").
		arg(std::get<0>(RGBVec[index])).arg(std::get<1>(RGBVec[index])).arg(std::get<2>(RGBVec[index])));
}

dtuparam::dtuparam(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuparamClass())
{
	ui->setupUi(this);
}

dtuparam::~dtuparam()
{
	delete ui;
}

void dtuparam::SetParamID(uint16_t paramid)
{
	paramID = paramid;
	Paramindex = DTU::DBManager::instance().GetParamInfo();
	load_ui();
	if (paramID == ParamAdjust) {
		ui->btn_default->setEnabled(false);
		ui->btn_save->setEnabled(false);
	}
}

uint16_t dtuparam::GetCurWidgetParamID()
{
	return paramID;
}

QTableWidget *dtuparam::createTabWidget(QTableWidget *ptable,int row, QStringList header)
{
	// 设置行数
	ptable->setRowCount(row);
	// 设置列数
	ptable->setColumnCount(header.size());
	// 设置表头
	ptable->setHorizontalHeaderLabels(header);

	// ui修改
	// 自动延展
	ptable->horizontalHeader()->setStretchLastSection(true);
	//设置为不可修改
	ptable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	// 隐藏水平表头
	//ptable->horizontalHeader()->setVisible(false);
	// 隐藏垂直表头
	ptable->verticalHeader()->setVisible(false);
	// 先自适应宽度
	ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	//然后设置要根据内容使用宽度的列
	ptable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	//ptable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	return ptable;
}

void dtuparam::load_ui()
{
	// 加载标题
	QStringList header;
	header << "序号" << "点表值" << "参数类型" << "参数名称" << "参数值" << "类型" << "单位";

	auto paraminfo = Paramindex[paramID];
	QTableWidget *ptable = createTabWidget(ui->tableWidget,paraminfo.info.size(), header);
	int index = 1;
	QString lastText = "";
	bool state = false;
	for (const auto &item : paraminfo.info)
	{
		QString text = QString::fromStdString(item.second.paramtype);
		if (lastText != text) {
			// 改变颜色
			state = true;
			lastText = text;
		}
		else {
			state = false;
		}

		QLabel *label = createStrLabel(QString::number(index));
		ptable->setCellWidget(index - 1, 0, label);//序号
		setColor(state, label);

		label = createStrLabel("0x" + QString("%1").arg(item.second.fixid, 2, 16, QLatin1Char('0')).toUpper());
		ptable->setCellWidget(index - 1, 1, label);// 点表
		setColor(false, label);

		label = createStrLabel(QString::fromStdString(paraminfo.desc));
		ptable->setCellWidget(index - 1, 2, label);// 参数类型
		setColor(false, label);

		label = createStrLabel(QString::fromStdString(item.second.desc));
		ptable->setCellWidget(index - 1, 3, label);// 参数名称
		setColor(false, label);


		ptable->setCellWidget(index - 1, 4, createControl(item.second.QTctrl, {
			item.second.desc,item.second.min,item.second.max,item.second.defaultValue,item.second.step,item.second.list
			}));// 数值

		label = createStrLabel(QString::fromStdString(item.second.paramtype));
		ptable->setCellWidget(index - 1, 5, label);// 类型
		setColor(false, label);

		label = createStrLabel(QString::fromStdString(item.second.unit));
		ptable->setCellWidget(index - 1, 6, label);// 单位
		setColor(false, label);

		index++;
	}
}

void dtuparam::read_param()
{
	try
	{
		DTU::buffer data;
		if (execute_test_arm_connect())
		{
			uint32_t curgroup = 0;
			uint32_t maxgroup = 0;
			// 将后台与工具定值区统一
			execute_read_group(curgroup, maxgroup);
			if (DBManager::instance().GetCurrGroup() != curgroup)
				DBManager::instance().SetCurrGroup(curgroup);

			// 第一步:从后台读取定值
			if (DTU_SUCCESS == execute_read_setting(
				DBManager::instance().Get_R_CMD_By_ParamID(paramID), data,
				DBManager::instance().GetCurrGroup()))
			{
				// 本地数据库存储一份
				// 检查数据
				if (!checkData(paramID, data.size()))
					return;
				// 特殊定值处理
				switch (paramID)
				{
				case ParamGroupNo: {
					// 设置当前定值区
					DBManager::instance().SetCurrGroup(data.value<uint32_t>());
					QtDTULOG(DTU_INFO, "当前定值区[%u]", data.value<uint32_t>());
				}; break;
				case ParamCommunication: {
					// 通信定值 不进行保存(或者进行特殊保存) 直接退出switch
				}; break;
				case ParamAdjust: {
					// 整定定值 不进行保存 直接退出switch
				}; break;
				default: {
					dtuParam param(paramID);
					param.unpack(data);
					DBManager::instance().saveParamValue(param);
				}break;
				}
			}
			else
				QtDTULOG(DTU_ERROR, "读取定值错误");
		}
		else
		{
			// 未连接从数据库读取
			QtDTULOG(DTU_WARN, "后台程序未连接,从本地读取定值数据");
			if (paramID == static_cast<uint16_t>(ParamGroupNo))
			{
				uint32_t curno = DBManager::instance().GetCurrGroup();
				data.append((char*)&curno, sizeof(uint32_t));
				return;
			}
			else
			{
				dtuParam param(paramID);
				DBManager::instance().readParamValue(param);
				data = param.pack();
				// 检查数据
				if (!checkData(paramID, data.size()))
					return;
			}
		}

		int index = 0;
		for (const auto &item : Paramindex[paramID].info)
		{
			setControlValue(item.second.QTctrl, data.data() + item.second.offset, item.second.size,
				ui->tableWidget->cellWidget(index, 4));
			index++;
		}
	}
	catch (std::exception &e)
	{
		QtDTULOG(DTU_ERROR, "读取定值发生未知错误,定值ID[0x%04x]:%s", paramID, e.what());
		return;
	}
}

void dtuparam::save_param()
{
	try
	{
		if (execute_test_arm_connect())
		{
			// 第一步:建立buffer
			DTU::buffer data;
			data.resize(Paramindex[paramID].size);
			int index = 0;
			for (const auto &item : Paramindex[paramID].info)
			{
				getControlValue(item.second.QTctrl, data.data() + item.second.offset, item.second.size,
					ui->tableWidget->cellWidget(index, 4));
				index++;
			}

			if (DTU_SUCCESS != execute_write_setting(
				DBManager::instance().Get_W_CMD_By_ParamID(paramID), data,
				DBManager::instance().GetCurrGroup()))
			{
				// 失败
				QtDTULOG(DTU_ERROR, "保存定值发生错误");
			}
			else
			{
				// 本地数据库存储一份  特殊定值处理
				switch (paramID)
				{
				case ParamGroupNo: {
					// 设置当前定值区
					DBManager::instance().SetCurrGroup(data.value<uint32_t>());
				}; break;
				case ParamCommunication: {
					// 通信定值 直接跳过
				}; break;
				default: {
					dtuParam param(paramID);
					param.unpack(data);
					DBManager::instance().saveParamValue(param);
				}
				};
			}
		}
		else
		{
			// 未连接 不进行下发
			QtDTULOG(DTU_WARN, "后台程序未连接,取消");
		}
	}
	catch (std::exception &e)
	{
		QtDTULOG(DTU_ERROR, "保存定值发生未知错误,定值ID[0x%04x]:%s", paramID, e.what());
		return;
	}
}

void dtuparam::set_default()
{
	try
	{
		int index = 0;
		DTU::buffer data;
		dtuParam param(paramID);
		bool ok = true;

		for (const auto &item : Paramindex[paramID].info)
		{
			param.SetValueByFixid(static_cast<QLabel*>(ui->tableWidget->cellWidget(index, 1))->text().toInt(&ok, 16),
				item.second.defaultValue);
			index++;
		}

		index = 0;
		data = param.pack();

		for (const auto &item : Paramindex[paramID].info)
		{
			setControlValue(item.second.QTctrl, data.data() + item.second.offset, item.second.size,
				ui->tableWidget->cellWidget(index, 4));
			index++;
		}

	}
	catch (std::exception &e)
	{
		QtDTULOG(DTU_ERROR, "设置默认定值发生错误,参数ID[0x%04x]:%s", paramID, e.what());
		return;
	}
}

bool dtuparam::checkData(uint16_t paramid, int datasize)
{
	if (Paramindex[paramid].size == datasize)
		return true;
	else {
		QtDTULOG(DTU_ERROR, "参数ID[0x%04X]数据长度不匹配,当前数据长度[%d]期望长度[%d]",
			paramid, datasize, Paramindex[paramid].size);
		return false;
	}
}
