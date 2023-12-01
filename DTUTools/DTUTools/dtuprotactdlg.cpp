#include "dtuprotactdlg.h"
#include <map>
#include <vector>
#include <tuple>
#include <dtureporthelper.h>
#include <dtucommon.h>
#include "dtucmdcode.h"

#include "dtudbmanager.h"

// 保护动作事件(与保护动作报告保护类型对应)
static std::map<uint16_t, std::string> gActDesc = {
	{1, "保护启动"},{2, "空"},{3, "整组复归"},{4, "长启动"},
	{5, "跳闸失败"},{6, "过流I段"},{7, "过流II段"},{8, "过流III段"},
	{9, "后加速电流保护"},{10, "一次重合闸动作"},{11, "二次重合闸动作"},
	{12, "保护启动重合闸"},{13, "不对应启动重合闸"},{14, "重合闸检测超时"},
	{15, "重合闸充电满"},{16, "重合闸放电"},{17, "线路PT断线"},{18, "重合闸复归"},
	{19, "零序电压后加速"},{20, "大电流闭锁重合闸"},{21, "过电压"},{22, "高周解列"},
	{23, "零序过压"},{24, "零序过流I段"},{25, "零序过流II段"},{26, "零序过流加速段"},
	{27, "重合闸T2闭锁二次重合闸"},{28, "手合同期退出"},{29, "手合同期失败"},
	{30, "手合同期同频同期动作"},{31, "手合同期差频同期动作"},{32, "手合同期检无压动作"},
	{33, "跳闸成功"},{34, "重合闸T4复归"},{35, "空"},{36, "空"},{37, "空"},{38, "空"},
	{39, "电压越限启动"},{40, "低电压解列"},{41, "电压过低解列"},{42, "高电压解列"},
	{43, "电压过高解列"},{44, "频率启动"},{45, "低频解列"},{46, "频率过低解列"},
	{47, "高频解列"},{48, "过负荷"},{49, "重载"},{50, "空"},{51, "控制回路断线"},
	{52, "控制回路异常"},{53, "PT断线"},{54, "保留"},{55, "保留"},{56, "保留"},
	{57, "保留"},{58, "保留"},{59, "保留"},{60, "保留"},{61, "电源侧得电延时合闸"},
	{62, "负荷侧得电延时合闸"},{63, "双侧失电延时分闸"},{64, "电源侧正向闭锁合闸"},
	{65, "电源侧反向闭锁合闸"},{66, "负荷侧正向闭锁合闸"},{67, "负荷侧反向闭锁合闸"},
	{68, "电源侧残压闭锁"},{69, "负荷侧残压闭锁"},{70, "合到故障快速分闸"},
	{71, "合到零压闭锁合闸"},{72, "多次失电分闸闭锁合闸"},{73, "保留"},{74, "保留"},
	{75, "单侧失压延时合闸"},{76, "联络开关双侧失电延时分闸"},{77, "联络开关电源残压闭锁"},
	{78, "双侧有压闭锁合闸"},{79, "PT断线闭锁合闸"},{80, "联络开关负荷残压闭锁"},{200, "LCD清屏命令"},
};
std::string get_protect_act(uint16_t type)
{
	auto ita = gActDesc.find(type);
	if (ita != gActDesc.end()) {
		return ita->second;
	}
	return ("未知保护动作");
}

dtuprotactdlg::dtuprotactdlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.tableWidget_prot_event->verticalHeader()->hide();
	ui.tableWidget_prot_event->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	ui.tableWidget_prot_event->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.tableWidget_prot_event->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.tableWidget_prot_event->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.tableWidget_prot_event->horizontalHeader()->setStretchLastSection(true);

	ui.tableWidget_prot_detail->verticalHeader()->hide();
	ui.tableWidget_prot_detail->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	ui.tableWidget_prot_detail->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.tableWidget_prot_detail->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.tableWidget_prot_detail->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.tableWidget_prot_detail->horizontalHeader()->setStretchLastSection(true);

	this->setWindowTitle("动作报告解析");
}

dtuprotactdlg::~dtuprotactdlg(){}

void dtuprotactdlg::parse(const DTU::buffer& data)
{
	if (data.size() == 0) {
		return;
	}
	uint32_t counts = data.get(0, sizeof(counts)).value<uint32_t>();
	_prot_act.clear();
	//
	int reportsize = DTU::DBManager::instance().GetReportInfoByID(DTU::ReportPro).size;
	for (auto i = 0; i < counts; i++)
	{
		_prot_act.emplace_back(data.get(sizeof(counts) + i * reportsize, reportsize));
	}
	display_act();
}

void addOneItemToTable(QTableWidget *table, int index, int no, QString desc)
{
	table->setItem(index, no, new QTableWidgetItem(desc));
	table->item(index, no)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void dtuprotactdlg::display_act()
{
	ui.tableWidget_prot_event->clearContents();
	ui.tableWidget_prot_event->setRowCount(0);
	for (auto i = 0; i < _prot_act.size(); i++)
	{
		// 获取报告类型
		uint16_t type = (_prot_act[i]).get(sizeof(uint16_t), sizeof(uint16_t)).value<uint16_t>();
		ui.tableWidget_prot_event->insertRow(i);
		if (i == 0) {
			// 获取总秒数
			uint32_t seconds = (_prot_act[i]).get(2 * 6, 4).value<uint32_t>();
			uint32_t mircosec = (_prot_act[i]).get(2 * 6 + 4, 4).value<uint32_t>();
			addOneItemToTable(ui.tableWidget_prot_event, i, 0,
				QString::fromStdString(create_time_from_mirco((uint64_t)seconds * 1000000 + (uint64_t)mircosec)));
		}
		else {
			uint32_t toffset = (_prot_act[i]).get(2 * 6 + 8, 4).value<uint32_t>();
			addOneItemToTable(ui.tableWidget_prot_event, i, 0, QString("%1ms").arg(toffset));
		}

		addOneItemToTable(ui.tableWidget_prot_event, i, 1, 
			QString::fromStdString(get_protect_act(type)));
	}
}

void dtuprotactdlg::show_protect_detail(QTableWidgetItem* pItem)
{
	auto ret = DTU::DBManager::instance().GetReportInfoByID(DTU::ReportPro);

	int selectRow = pItem->row();
	if (selectRow >= _prot_act.size()) {
		return;
	}

	auto& data = _prot_act[selectRow];

	ui.tableWidget_prot_detail->clearContents();
	ui.tableWidget_prot_detail->setRowCount(0);
	int index = 0;

	int ID = 1;
	for (auto &item : ret.rinfo)
	{
		auto DESC = item.second.desc;
		auto SIZE = item.second.size;
		auto OFFSET = item.second.offset;

		// 如果是时间则返回
		if (ID == 6 || ID == 7) {
			ID++;
			continue;
		}
		ui.tableWidget_prot_detail->insertRow(index);

		addOneItemToTable(ui.tableWidget_prot_detail, index, 0,
			QString::fromStdString(DESC.c_str()));
		
		// 解属性
		if (ID == 3)
		{
			static std::map<uint16_t, std::string> protectval = {
				{1,"无效"},//无效
				{2,"动作"},
				{3,"告警"},
				{4,"告警返回"},
			};
			auto ret = protectval.find(data.get(OFFSET, SIZE).value<uint16_t>());

			if (ret != protectval.end()) {
				addOneItemToTable(ui.tableWidget_prot_detail, index, 1, 
					ret->second.c_str() + QString(" (%1)").arg(data.get(OFFSET, SIZE).value<uint16_t>()));
			}
			else {
				addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
					"未知属性" + QString(" (%1)").arg(data.get(OFFSET, SIZE).value<uint16_t>()));
			}

			index++;
			ID++;
			continue;
		}

		// 解二进制
		if (ID == 4 || ID == 5)
		{
			addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
				QString("%1(B)").arg(data.get(OFFSET, SIZE).value<uint16_t>(), 16, 2, QLatin1Char('0')));
			index++;
			ID++;
			continue;
		}

		// 电流模拟量  ID 在之间  需要数值除以100
		if (ID >= 10 && ID <= 15)
		{
			addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
				QString("%1 (A)").arg(data.get(OFFSET, SIZE).value<uint16_t>() / 100.0));

			index++;
			ID++;
			continue;
		}

		// 电压模拟量  ID 在之间 需要数值除以100
		if (ID >= 16 && ID <= 25)
		{
			addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
				QString("%1 (V)").arg(data.get(OFFSET, SIZE).value<uint16_t>() / 100.0));

			index++;
			ID++;
			continue;
		}

		if (ID == 26)
		{
			QString txtshow;
			switch (data.get(OFFSET, SIZE).value<uint16_t>())
			{
			case 191:txtshow = "A相"; break;
			case 192:txtshow = "B相"; break;
			case 193:txtshow = "C相"; break;
			case 194:txtshow = "AB相"; break;
			case 195:txtshow = "BC相"; break;
			case 196:txtshow = "CA相"; break;
			case 197:txtshow = "ABC相"; break;
			case 198:txtshow = "零序电流"; break;
			case 199:txtshow = "零序电压"; break;
			case 200:txtshow = "判相错误"; break;
			default:
				txtshow = "未知相别" + QString::number(data.get(OFFSET, SIZE).value<uint16_t>()); break;
			}

			addOneItemToTable(ui.tableWidget_prot_detail, index, 1, txtshow);

			index++;
			ID++;
			continue;
		}

		// 解析频率
		if (ID == 27 || ID == 28)
		{
			addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
				QString("%1 (Hz)").arg(data.get(OFFSET, SIZE).value<uint16_t>() / 100.0));

			index++;
			ID++;
			continue;
		}

		// 解析其他
		addOneItemToTable(ui.tableWidget_prot_detail, index, 1,
			QString("%1").arg(data.get(OFFSET, SIZE).value<uint16_t>()));

		index++;
		ID++;
	}

}
