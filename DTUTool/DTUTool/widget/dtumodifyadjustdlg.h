#ifndef DTUMODIFYADJUSTDLG_H
#define DTUMODIFYADJUSTDLG_H

#include <QDialog>
#include "dtuadjust.h"
#include "dturecorder.h"
#include <dtustructs.h>
namespace Ui {
class dtumodifyadjustDlg;
}

class dtumodifyadjustDlg : public QDialog
{
    Q_OBJECT
public:
	typedef	enum
	{
		USE_SELECTANALOG = 0,
		USE_ADJPARAM = 1,
	}USE_WHICH_PARAM;
	typedef enum
	{
		MODIFY_FLAG_FLAG = 2,
		NOMODIFY_FLAG_FLAG = 3,
	}MODIFY_FLAG;
public:
    explicit dtumodifyadjustDlg(DTUParamAdjust &_adjParam ,std::vector<CHN_ANALOG> &m_selectAnalog, bool curMenu,int channel = CHANNEL_NO_MAX, int column = 2,QWidget *parent = 0);
    ~dtumodifyadjustDlg();
public:
	void get_result(DTUParamAdjust &_adjParam, std::vector<CHN_ANALOG> &m_selectAnalog);
	int _use;
private:
    Ui::dtumodifyadjustDlg *ui;
	void init(DTUParamAdjust &_adjParam, std::vector<CHN_ANALOG> &m_selectAnalog);
	int _channel;
	DTUParamAdjust _adjParam;
	std::vector<CHN_ANALOG> m_selectAnalog;
public slots:
    void modify_adjust();
	void show_infomation(int index);
};

#endif // DTUMODIFYADJUSTDLG_H
