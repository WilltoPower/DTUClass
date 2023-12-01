#pragma once

#include <QDialog>
#include <vector>
#include "ui_dtuparamchannelDlg.h"
#include "dtusysconfig.h"

class dtuparamchannelDlg : public QDialog
{
	Q_OBJECT

public:
	dtuparamchannelDlg(QWidget *parent = Q_NULLPTR);
	~dtuparamchannelDlg();

	void load_channellist(std::vector<CHN_ANALOG>& selectedChannel, std::vector<CHN_ANALOG>& totleChannel);

	void update_channellist();

	void get_channellist(std::vector<CHN_ANALOG>& selectedChannel);
public slots:
	void select_one_channel();
	void delete_one_channel();
	void select_all_channel();
	void delete_all_channel();
	void select_ok();
	void select_cancel();
private:
	std::vector<CHN_ANALOG> m_vSelected;
	std::vector<CHN_ANALOG> m_vTempSelected;
	std::vector<CHN_ANALOG> m_totleChannel;
	bool firstLoad = true;
	Ui::sdlpct_select_channel_dlg ui;
};
