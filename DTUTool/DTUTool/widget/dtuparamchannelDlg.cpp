#pragma execution_character_set("utf-8")

#include "dtuparamchannelDlg.h"

#include <QTableWidgetItem>
#include <QComboBox>

#include "QCreateHelper.h"
#include <algorithm>


dtuparamchannelDlg::dtuparamchannelDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.tableWidget_selected->verticalHeader()->hide();
	ui.tableWidget_selected->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
	ui.tableWidget_selected->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
	ui.tableWidget_selected->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::Stretch);
	ui.tableWidget_selected->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.tableWidget_selected->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.tableWidget_selected->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

dtuparamchannelDlg::~dtuparamchannelDlg()
{
}

void dtuparamchannelDlg::load_channellist(std::vector<CHN_ANALOG>& selectedChannel, std::vector<CHN_ANALOG>& totleChannel)
{
	m_vTempSelected.clear();
	m_vTempSelected.reserve(selectedChannel.size());
	m_vTempSelected.insert(m_vTempSelected.end(), selectedChannel.begin(), selectedChannel.end());

	m_totleChannel = totleChannel;

	update_channellist();
}
void dtuparamchannelDlg::update_channellist()
{
	if (firstLoad)
	{
		firstLoad = false;
	}
	else
	{
		this->setWindowModified(true);
	}
	ui.tableWidget_selected->clearContents();
	ui.tableWidget_selected->setRowCount(0);
	ui.listWidget_unselected->clear();

	// 加载已选择
	int nRow = 0;
	for (auto& item : m_totleChannel)
	{
		auto ita = find_if(m_vTempSelected.begin(), m_vTempSelected.end(), [&, item](CHN_ANALOG& item1) {
			return item1._chNo == item._chNo;
		});
		auto listitem = new QListWidgetItem;
		QString str = QString("%1.%2").arg(item._chNo, 2, 10, QLatin1Char('0')).arg(item._chName.c_str());
		listitem->setText(str);

		if (ita == m_vTempSelected.end())
		{
			// 未找到则加入未选择列表
			ui.listWidget_unselected->addItem(listitem);
		}
		else
		{
			ui.tableWidget_selected->insertRow(nRow);
			ui.tableWidget_selected->setItem(nRow, 0, new QTableWidgetItem(QString("%1").arg(item._chNo)));
			ui.tableWidget_selected->setItem(nRow, 1, new QTableWidgetItem(item._chName.c_str()));
			
			QStringList list;
			list << "交流" << "直流";
			QComboBox* pComb = QCreateHelper::createComboBox(list);
			pComb->setCurrentIndex(ita->_curType);
			ui.tableWidget_selected->setCellWidget(nRow, 2,(QWidget*)pComb);
			nRow++;
		}
	}
}
void dtuparamchannelDlg::select_one_channel()
{
	auto selitem = ui.listWidget_unselected->currentItem();
	if (!selitem) {
		return;
	}
	auto str = selitem->text().split(".");

	auto sNo = str[0].toUInt();

	auto ita = std::find_if(m_totleChannel.begin(), m_totleChannel.end(), [&, sNo](CHN_ANALOG& item) {
		return sNo == item._chNo;
	});
	if (ita != m_totleChannel.end())
	{
		m_vTempSelected.push_back(*ita);
	}
	update_channellist();
}
void dtuparamchannelDlg::delete_one_channel()
{
	auto selitem = ui.tableWidget_selected->currentRow();

	auto selItem = ui.tableWidget_selected->item(selitem, 0);
	if (!selItem) {
		return;
	}
	auto str = selItem->text();

	auto sNo = str.toUInt();

	auto ita = find_if(m_vTempSelected.begin(), m_vTempSelected.end(), [&, sNo](CHN_ANALOG& item) {
		return sNo == item._chNo;
	});
	if (ita != m_vTempSelected.end())
	{
		m_vTempSelected.erase(ita);
	}
	update_channellist();
}
void dtuparamchannelDlg::select_all_channel()
{
	ui.listWidget_unselected->clear();
	m_vTempSelected.clear();

	m_vTempSelected.reserve(m_totleChannel.size());
	m_vTempSelected.insert(m_vTempSelected.end(), m_totleChannel.begin(), m_totleChannel.end());

	update_channellist();
}
void dtuparamchannelDlg::delete_all_channel()
{
	m_vTempSelected.clear();
	update_channellist();
}
void dtuparamchannelDlg::select_ok()
{
	m_vSelected.clear();
	m_vSelected.reserve(m_vTempSelected.size());
	m_vSelected.insert(m_vSelected.end(), m_vTempSelected.begin(), m_vTempSelected.end());
	// 确定直流交流

	for (auto& item : m_vSelected)
	{
		for (int i = 0; i < ui.tableWidget_selected->rowCount(); i++)
		{
			if (item._chNo == ui.tableWidget_selected->item(i, 0)->text().toUInt())
			{
				item._curType = ((QComboBox*)(ui.tableWidget_selected->cellWidget(i, 2)))->currentIndex();
			}
		}
	}
	emit done(1);
}
void dtuparamchannelDlg::select_cancel()
{
	emit done(0);
}
void dtuparamchannelDlg::get_channellist(std::vector<CHN_ANALOG>& selectedChannel)
{
	selectedChannel.clear();
	selectedChannel.reserve(m_vSelected.size());
	selectedChannel.insert(selectedChannel.end(), m_vSelected.begin(), m_vSelected.end());
}