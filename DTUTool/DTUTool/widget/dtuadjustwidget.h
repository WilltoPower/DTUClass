#pragma once

#include "ui_dtuadjustwidget.h"

#include <QWidget>
#include <vector>
#include <QTimer>

#include <dtubuffer.h>
#include "dtuadjust.h"
#include "dtuparamchannelDlg.h"
#include "dturecorder.h"
#include <dtustructs.h>


class dtuadjustwidget : public QWidget
{
	Q_OBJECT

public:
	typedef enum
	{
		FLAG_Amplitude = 0,//幅值整定
		FLAG_Phase	   = 1,//相位整定
		FLAG_Modify    = 2,//修改整定参数值
		FLAG_NoModify  = 3,//无修改
	}SAVE_FLAG_PARAM;
	dtuadjustwidget(QWidget *parent = Q_NULLPTR);
	~dtuadjustwidget();
	// 交流通道整定
	void ChnACCalibrate();
	// 直流通道整定
	void ChnDCCalibrate();
	// 板卡整定
	void ChnBoradCalibrate();
	//载入整定结果
	void loadCalibrateResult(bool bReload = true);
protected:

	void ChnACCalibrateAmplitude();
	void ChnACCalibratePhase();
	
	bool SaveACInOutDX0();

	void TransDataFromWindow();
	void TransDataToWindow();

	bool ReadRuntimeData();

	double GetPrimaryValue(std::uint32_t chNo);

	double GetPhase(std::uint32_t chNo);

	
	void SaveParam();
	void SendResult();
	void showAdjust();
public slots:
	void start_calibrate();
	void select_channel();
	void show_result();
	void save_result();
	void restart_calibrate();
	void bgGroup_toggled(int, bool);
	void cancel_last_step();
	void get_adjust_result();
	void clear_tablewigdet_show();
	void modify_adjust_result();
private:
	Ui::sdlpct_settings_adjust_widget ui;

	dtuparamchannelDlg m_selectDlg;//新窗口
	std::vector<CHN_ANALOG> _analogList;
	std::vector<CHN_ANALOG> m_selectAnalog;

	std::vector<std::tuple<uint16_t, uint32_t, QString>> _digitalChannel;

	double m_fIInput;		// 当前输入电流
	double m_fVInput; 		// 当前输入电压
	double m_fIStep;  		// 输入电流间隔
	double m_fVStep; 		// 输入电压间隔

	double m_fInDXOV;		// 交点电压值
	double m_fInDXOI;		// 交点电流值

	DTU::buffer m_calibrationData;
	// 整定参数
	DTUParamAdjust _adjParam;
	DTU::DAdjustRcd _lowSpeedWave;
	int m_nStep = 0;
	bool firstGetValue = false;
	int SAVE_FLAG = -1;//见SAVE_FLAG
	std::map<int, double> zero_code_storage;
#define ALLRESULT true
#define ADJUSTRESULT false
	// 当前显示的是所有整定值还是整定结果
	bool curMenu = ALLRESULT;
	// 首次完成整定
	bool finashFirst = false;
private:
	QTimer *timer = nullptr;
	bool adjustFinish = false;
private slots:
	void TimeOutFunc();
signals:
	void send_status_info(QString, int, int, int, QString, int, bool);
};
