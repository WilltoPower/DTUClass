#include "dtuadjustwidget.h"

#include "dtumodifyadjustdlg.h"

#include <QMessageBox>
#include <iostream>
#include <memory>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <complex>
#include <cassert>
#include <QTreeWidgetItem>

#include <QButtonGroup>
#include <QTablewidget>
#include <vector>
#include <dtulog.h>
#include <dtucmdcode.h>
#include <dtutask.h>
#include <dtuprotocol.h>
#include <dtuerror.h>
#include <dtucommon.h>
#include "dtustorage.h"
#include <map>

#define QSTR(str) QString::fromLocal8Bit(str)
#define SIGNALSEND(str1,start,end,str2,flag) emit send_status_info(QSTR(str1),start,end, 5000, QSTR(str2), 5000,flag)

//通道信息 通道号 对应通道号是电压还是电流通道
static std::map<uint32_t, uint32_t> CHN_List = {
	{  1, CHN_ANALOG::elVoltage},//Ua
	{  2, CHN_ANALOG::elVoltage},//Ub
	{  3, CHN_ANALOG::elVoltage},//Uc
	{  4, CHN_ANALOG::elVoltage},//U0
	{  5, CHN_ANALOG::elCurrent},//Ia
	{  6, CHN_ANALOG::elCurrent},//Ib
	{  7, CHN_ANALOG::elCurrent},//Ic
	{  8, CHN_ANALOG::elCurrent},//I0
	{  9, CHN_ANALOG::elCurrent},//Ix0
	{ 10, CHN_ANALOG::elVoltage},//Ua
	{ 11, CHN_ANALOG::elVoltage},//Ub
	{ 12, CHN_ANALOG::elVoltage},//Uc
	{ 13, CHN_ANALOG::elVoltage},//U0
	{ 14, CHN_ANALOG::elCurrent},//Ia
	{ 15, CHN_ANALOG::elCurrent},//Ib
	{ 16, CHN_ANALOG::elCurrent},//Ic
	{ 17, CHN_ANALOG::elCurrent},//I0
	{ 18, CHN_ANALOG::elVoltage},//直流通道1
	{ 19, CHN_ANALOG::elVoltage},//直流通道2
};

static std::vector<std::string> CHN_Desc = {
	{"第1路交流电压通道Ua"},
	{"第1路交流电压通道Ub"},
	{"第1路交流电压通道Uc"},
	{"第1路交流零序电压通道U0"},
	{"第1路交流电流通道Ia"},
	{"第1路交流电流通道Ib"},
	{"第1路交流电流通道Ic"},
	{"第1路交流零序电流通道I0"},
	{"交流零序电流I0x(小电流)"},
	{"第2路交流电压通道Ua"},
	{"第2路交流电压通道Ub"},
	{"第2路交流电压通道Uc"},
	{"第2路交流零序电压通道U0"},
	{"第2路交流电流通道Ia"},
	{"第2路交流电流通道Ib"},
	{"第2路交流电流通道Ic"},
	{"第2路交流零序电流通道I0"},
	{"直流电压通道1"},
	{"直流电压通道2"},
	{"备用"},
	{"备用"},
	{"备用"},
};

dtuadjustwidget::dtuadjustwidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.btn_cancel_last->setEnabled(false);
	ui.btn_restart->setEnabled(false);

	QButtonGroup* bGroup = new QButtonGroup(this);
	bGroup->addButton(ui.radio_amplitude, 0);
	bGroup->addButton(ui.radio_phase, 1);

	connect(bGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(bgGroup_toggled(int, bool)));
	for (auto i = 0; i < CHANNEL_NO_MAX; i++)
	{
		_analogList.emplace_back(std::move(CHN_ANALOG(i + 1, 0, CHN_Desc[i])));
	}

	ui.tableWidget_adjustresult->setSelectionBehavior(QAbstractItemView::SelectRows);//单行选择
	ui.tableWidget_adjustresult->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
	ui.tableWidget_adjustresult->verticalHeader()->setVisible(false);
	ui.tableWidget_adjustresult->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);

	//开始键锁住
	ui.btn_start->setEnabled(false);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(TimeOutFunc()));
}

dtuadjustwidget::~dtuadjustwidget()
{
}

void dtuadjustwidget::ChnACCalibrate()
{
	TransDataFromWindow();
	QString str, str1;
	str1 = "开始";
	str = ui.btn_start->text();
	if (str1 == str)
	{
		adjustFinish = false;
		// 检查参数是否保存

		// 检查整定通道
		if (m_selectAnalog.empty()) {
			QMessageBox::warning(this, "提示", "未选择通道！");
			return;
		}

		ui.listWidget_operate->clear();
		ui.btn_start->setText("继续");
		ui.btn_restart->setEnabled(true);

		ui.listWidget_operate->addItem("请在通道上加入交点处同相位电压电流，继续......");
		ui.btn_cancel_last->setEnabled(false);

		return;
	}

	SIGNALSEND("读取实时数据", 0, 0, "读取实时数据", true);

	timer->start(2800);

	if (ui.radio_amplitude)
	{
		ChnACCalibrateAmplitude();
	}
	else
	{
		ChnACCalibratePhase();
	}

	ui.btn_start->setEnabled(false);
}
void dtuadjustwidget::ChnDCCalibrate()
{

}
void dtuadjustwidget::ChnBoradCalibrate()
{

}

void dtuadjustwidget::ChnACCalibrateAmplitude()
{
	//// 与下层通信获取3个周波数据
	try
	{
		if (!ReadRuntimeData())
		{
			return;
		}

		double zero = 0;
		bool is_ok = false;
		for (size_t i = 0; i < m_selectAnalog.size(); i++)
		{
			uint16_t analog_id = m_selectAnalog[i]._chNo;

			if (m_nStep == 0) {
				m_selectAnalog[i].ZeroExcursion = 0.0;
			}
			uint16_t base_frequency = 50;
			zero = 0;
			if (ui.radio_amplitude->isChecked())
			{
				m_selectAnalog[i].rawValues[m_nStep] = GetPrimaryValue(m_selectAnalog[i]._chNo);
				if (m_selectAnalog[i]._curType == CHN_ANALOG::DC && m_nStep == 0)
				{
					// 直流整定 保存电压为零时零漂
					zero_code_storage[i] = m_selectAnalog[i].rawValues[m_nStep];
					m_selectAnalog[i].rawValues[m_nStep] = m_selectAnalog[i].rawValues[m_nStep] - zero_code_storage[i];
					//printf("零漂值 %f \n\n", zero_code_storage[i]);
				}
				else if (m_selectAnalog[i]._curType == CHN_ANALOG::DC && m_nStep > 0)
				{
					// 直流整定 每一步都去除零漂值
					//printf("i = %d 原值%f 零漂%f ",i, m_selectAnalog[i].rawValues[m_nStep], zero_code_storage[i]);
					m_selectAnalog[i].rawValues[m_nStep] = m_selectAnalog[i].rawValues[m_nStep] - zero_code_storage[i];
					//printf("修改值 %f\n", m_selectAnalog[i].rawValues[m_nStep]);
				}
			}
			else
			{
				m_selectAnalog[i].Phases[m_nStep] = GetPhase(m_selectAnalog[i]._chNo);
			}
			
			auto ret = CHN_List.find(m_selectAnalog[i]._chNo);
			if (ret == CHN_List.end())
			{
				return;
			}

			if (ret->second == CHN_ANALOG::elVoltage) {
				m_selectAnalog[i].Values[m_nStep] = ui.doubleSpinBox_input_vol->value();
			}
			else {
				m_selectAnalog[i].Values[m_nStep] = ui.doubleSpinBox_input_cur->value();
			}
		}
		if (m_nStep < CALIBRATION_STEP_NUMBER-1)
		{
			if (ui.doubleSpinBox_input_vol->text().isEmpty())
				ui.doubleSpinBox_input_vol->setValue(0.0);
			if (ui.doubleSpinBox_vol_step->text().isEmpty())
				ui.doubleSpinBox_vol_step->setValue(0.0);
			if (ui.doubleSpinBox_input_cur->text().isEmpty())
				ui.doubleSpinBox_input_cur->setValue(0.0);
			if (ui.doubleSpinBox_cur_step->text().isEmpty())
				ui.doubleSpinBox_cur_step->setValue(0.0);

			double voltage = 0, voltageIncrement = 0, current = 0, currentIncrement = 0;
			voltage = ui.doubleSpinBox_input_vol->value();
			voltageIncrement = ui.doubleSpinBox_vol_step->value();

			current = ui.doubleSpinBox_input_cur->value();
			currentIncrement = ui.doubleSpinBox_cur_step->value();

			if (ui.radio_amplitude->isChecked()) {
				voltage += voltageIncrement;
				current += currentIncrement;
			}

			ui.doubleSpinBox_input_vol->setValue(voltage);
			ui.doubleSpinBox_input_cur->setValue(current);

			ui.listWidget_operate->addItem(QString("请输入采样值(电压:%1,电流:%2),点击下一步,继续...").arg(voltage).arg(current));
		}
		else
		{
			std::vector<double> v;
			double baseKa;

			if (ui.radio_amplitude->isChecked())
			{
				// 幅值
				for (size_t i = 0; i < m_selectAnalog.size(); i++)
				{
					try {
						v = LeastSquare(CALIBRATION_STEP_NUMBER, m_selectAnalog[i].rawValues, m_selectAnalog[i].Values);
						m_selectAnalog[i].Kc = v[1];
						m_selectAnalog[i].X0 = v[0];
						if (m_selectAnalog[i]._curType == CHN_ANALOG::DC)
						{
							m_selectAnalog[i].ZeroExcursion = zero_code_storage[i];
						}
					}
					catch (...) {
						m_selectAnalog[i].Kc = 0;
						m_selectAnalog[i].X0 = 0;
					}

				}
			}
			else
			{
				for (size_t i = 0; i < m_selectAnalog.size(); i++)
				{
					double sum = 0;
					double diff;
					// The first point with index 0 is used to measure zero excursion,
					// its phase has great error.
					// So we use the first nonzero signal as the base
					double arg = m_selectAnalog[i].Phases[1];
					for (size_t j = 2; j < CALIBRATION_STEP_NUMBER; j++) {
						// normalization of difference helps to elimate the
						// great leap when angle is approximate to PI or -PI
						diff = NormalizeArgument(m_selectAnalog[i].Phases[j] - arg);
						sum += diff;
					}
					m_selectAnalog[i].Ka = arg + sum / (CALIBRATION_STEP_NUMBER - 1);
					if (i == 0) {
						baseKa = m_selectAnalog[i].Ka;
					}
					m_selectAnalog[i].Ka -= baseKa;
				}
			}

			ui.listWidget_operate->addItem(new QListWidgetItem("整定完成,点击保存按钮保存整定结果!"));
			finashFirst = true;
			adjustFinish = true;
			ui.btn_display_result->setEnabled(true);
		}

		m_nStep++;
		if (m_nStep == CALIBRATION_STEP_NUMBER) {
			ui.btn_cancel_last->setEnabled(false);
			ui.btn_start->setEnabled(false);
		}
		else {
			ui.btn_cancel_last->setEnabled(true);
			ui.btn_start->setEnabled(true);
		}
	}
	catch (std::exception& e)
	{
		SIGNALSEND("读取实时数据", 50, 100, "读取实时数据出错", false);
		DTULOG(DTU_ERROR, "%s", e.what());
	}
}

void dtuadjustwidget::ChnACCalibratePhase()
{
	
}

bool dtuadjustwidget::SaveACInOutDX0()
{
	return true;
}

void dtuadjustwidget::TransDataFromWindow()
{
	m_fIInput = ui.doubleSpinBox_input_cur->value();
	m_fVInput = ui.doubleSpinBox_input_vol->value();
	m_fVStep = ui.doubleSpinBox_vol_step->value();
	m_fIStep = ui.doubleSpinBox_cur_step->value();
}

void dtuadjustwidget::TransDataToWindow()
{
	ui.doubleSpinBox_input_cur->setValue(m_fIInput);
	ui.doubleSpinBox_input_vol->setValue(m_fVInput);
	ui.doubleSpinBox_vol_step->setValue(m_fVStep);
	ui.doubleSpinBox_cur_step->setValue(m_fIStep);
}

bool dtuadjustwidget::ReadRuntimeData()
{
	try
	{
		SIGNALSEND("读取实时数据", 0, 50, "读取实时数据", true);
		DTU::buffer result;
		if (DTU_SUCCESS == execute_query_data(PC_R_LO_ADJ_DATA, result))
		{
			int length = ((32 + DTU::DAdjustSamples::_samples_length * 50 * 4 / 2) * 3) * 2;
			if (result.size() != length)//单次周波数据长度
			{
				SIGNALSEND("读取实时数据", 50, 100, "读取实时数据错误", false);
				DTULOG(DTU_ERROR, (char*)"通道整定:接收低速采样数据长度错误!!当前长度%u,期望长度%u", (unsigned int)result.size(),length);
				return false;
			}
		}
		_lowSpeedWave.parse(result.const_data(), result.size());
	}
	catch (std::exception& e)
	{
		SIGNALSEND("读取实时数据", 50, 100, "读取实时数据错误", false);
		DTULOG(DTU_ERROR, "%s", e.what());
		return false;
	}
	SIGNALSEND("读取实时数据", 50, 100, "读取实时数据", true);
	return true;
}

double dtuadjustwidget::GetPrimaryValue(std::uint32_t chNo)
{
	auto rawValue = _lowSpeedWave.get_raw_value(chNo);
	// 计算原始值
	std::vector<double> x;
	for (auto i=0;i<rawValue.size();i++)
	{
		x.push_back(rawValue[i]);
	}

	for (auto& item : m_selectAnalog)
	{
		if (item._chNo == chNo)
		{
			if (item._curType == CHN_ANALOG::DC)
			{
				double ret = 0.0;
				for (auto& item : x)
				{
					ret += item;
				}
				return ret / x.size();
			}
		}
	}

	THROW_RUNTIME_ERROR_IF(x.size() == 0,"整定数据长度为0");
	std::complex<double> h = Dft(x.size(), x.data(), 3) / sqrt(2.0);
	std::complex<double> h1;

	static const double voltageCoefficients[] = { 1, 1, 1,
			1, 1, 1, 1 };
	static const double currentCoefficients[] = { 1, 1, 1,
			1, 1, 1, 1 };

	const double* coefs;
	coefs = voltageCoefficients;

	h1 = h * coefs[0];

	return abs(h1);
}

double dtuadjustwidget::GetPhase(std::uint32_t chNo)
{
	auto rawValue = _lowSpeedWave.get_raw_value(chNo);
	// 计算原始值
	std::vector<double> x;
	for (auto i = 0; i < rawValue.size(); i++)
	{
		x.push_back(rawValue[i]);
	}

	THROW_RUNTIME_ERROR_IF(x.size() == 0, "整定数据长度为0");

	std::complex<double> h = Dft(x.size(), x.data(),3) / sqrt(2.0);
	std::complex<double> h1;

	static const double voltageCoefficients[] = { 1, 1, 1,
			1, 1, 1, 1 };
	static const double currentCoefficients[] = { 1, 1, 1,
			1, 1, 1, 1 };
	const double* coefs;
	coefs = voltageCoefficients;
	h1 = h * coefs[0];


	double argument = Argument(h1);
	double firstChannel = 0;
	return NormalizeArgument(argument - firstChannel);
}

void dtuadjustwidget::loadCalibrateResult(bool bReload)
{
	if (bReload)
	{
		DTU::dtuprotocol proto;
		proto._header = 0xAA55;
		proto._cmd = PC_R_ADJ_FIX;//读取整定定值
		try {
			_adjParam.load();
		}
		catch (std::exception& e)
		{
			DTULOG(DTU_ERROR,(char*)"加载adj.xml文件失败");
			return;
		}
	}
	// 暂存目前的通道选择和数据
	auto cur_selectAnalog = m_selectAnalog;
	m_selectAnalog.clear();
	for (auto& item : _analogList)
	{
		CHN_ANALOG channel;
		channel._chNo = item._chNo;
		channel._chName = item._chName;
		channel.Ka = acos(_adjParam.GetValue(channel._chNo, ADJUST_ANGLE_COS));
		channel.Kc = _adjParam.GetValue(channel._chNo, ADJUST_RATIO);
		channel.ZeroExcursion = _adjParam.GetValue(channel._chNo, ADJUST_ZERO);
		channel.X0 = _adjParam.GetValue(channel._chNo, ADJUST_INTERCEPT);

		auto ret = CHN_List.find(item._chNo);
		if(ret != CHN_List.end())
		{
			channel._type = ret->second;
		}
		else
		{
			channel._type = CHN_ANALOG::elUnknown;
		}

		m_selectAnalog.emplace_back(std::move(channel));
	}
	showAdjust();
	// 还原通道选择和数据
	m_selectAnalog.clear();
	m_selectAnalog = cur_selectAnalog;
}

void dtuadjustwidget::SaveParam()
{
	for (auto& item : m_selectAnalog)
	{
		if (SAVE_FLAG == FLAG_Amplitude)
		{
			_adjParam.SetValue(item._chNo, item.Kc, ADJUST_RATIO);			//变比
			_adjParam.SetValue(item._chNo, item.X0, ADJUST_INTERCEPT);		//截距
			if (item._curType == CHN_ANALOG::DC)
			{
				_adjParam.SetValue(item._chNo, item.ZeroExcursion, ADJUST_ZERO);//零漂
			}
		}
		else if (SAVE_FLAG == FLAG_Phase)
		{
			_adjParam.SetValue(item._chNo, item.ZeroExcursion, ADJUST_ZERO);//零漂
			_adjParam.SetValue(item._chNo, cos(item.Ka), ADJUST_ANGLE_COS); //COS
			_adjParam.SetValue(item._chNo, sin(item.Ka), ADJUST_ANGLE_SIN);	//SIN
		}

		if (item.isModify == FLAG_Modify)
		{/* 手动修改值 */
			if (item.wModify & 0x1000)
			{
				_adjParam.SetValue(item._chNo, item.Kc, ADJUST_RATIO);//变比
			}

			if (item.wModify & 0x0100)
			{
				_adjParam.SetValue(item._chNo, item.X0, ADJUST_INTERCEPT);//截距
			}

			if (item.wModify & 0x0010)
			{
				_adjParam.SetValue(item._chNo, item.ZeroExcursion, ADJUST_ZERO);//零漂
			}

			if (item.wModify & 0x0001)
			{
				_adjParam.SetValue(item._chNo, cos(item.Ka), ADJUST_ANGLE_COS);//COS
				_adjParam.SetValue(item._chNo, sin(item.Ka), ADJUST_ANGLE_SIN);//SIN
			}
			item.isModify = FLAG_NoModify;
			item.wModify = 0;
		}
	}
	_adjParam.save();
	SendResult();
}

void dtuadjustwidget::SendResult()
{
	try
	{
		SIGNALSEND("下发整定参数", 0, 50, "下发整定参数", true);
		DTU::buffer adjResult;
		adjResult.append(_adjParam.Adjdata());
		// 下发整定参数
		int retCode = execute_write_data(PC_W_ADJ_FIX, adjResult);
		if (retCode == DTU_SUCCESS) 
		{
			DTULOG(DTU_INFO, "下发整定参数成功");
			SIGNALSEND("下发整定参数", 50, 100, "下发整定参数成功", true);
			try
			{
				// 更新本地整定参数
				//DTU::dtuprotocol proto;
				//proto._header = 0xAA55;
				//proto._cmd = PC_W_ADJ_FIX;//写取整定定值
				
				//DTU::DSTORE::instance().write_setting_data(DTU_GET_PARAM_ID(proto._cmd), _adjParam.data());
				//FILE  FILENAMAGER.WriteParamToFile<EPARAM::easyParamAdjust>(_adjParam);
			}
			catch (std::exception& e)
			{
				DTULOG(DTU_ERROR, const_cast<char*>(e.what()));
			}
		}
		else 
		{
			SIGNALSEND("下发整定参数", 50, 100, "下发整定参数成功", false);
			DTULOG(DTU_ERROR, "execute_write 下发整定参数失败");
		}
	}
	catch (std::exception& e)
	{
		SIGNALSEND("下发整定参数", 50, 100, "下发整定参数成功", false);
		DTULOG(DTU_ERROR, "下发整定参数失败:%s", e.what());
	}
}

void dtuadjustwidget::showAdjust()
{

	ui.tableWidget_adjustresult->setRowCount(0);
	for (auto i = 0; i < m_selectAnalog.size(); i++)
	{
		ui.tableWidget_adjustresult->insertRow(i);
		// 通道编号
		QTableWidgetItem *item = new QTableWidgetItem(QString("%1").arg(m_selectAnalog[i]._chNo));
		ui.tableWidget_adjustresult->setItem(i, 0, item);
		// 通道名称
		item = new QTableWidgetItem(QString(m_selectAnalog[i]._chName.c_str()));
		ui.tableWidget_adjustresult->setItem(i, 1, item);
		// 变比
		item = new QTableWidgetItem(QString("%1").arg(m_selectAnalog[i].Kc));
		ui.tableWidget_adjustresult->setItem(i, 2, item);
		// 截距
		item = new QTableWidgetItem(QString("%1").arg(m_selectAnalog[i].X0));
		ui.tableWidget_adjustresult->setItem(i, 3, item);
		// 零漂
		item = new QTableWidgetItem(QString("%1").arg(m_selectAnalog[i].ZeroExcursion));
		ui.tableWidget_adjustresult->setItem(i, 4, item);
		// 角度偏差
		item = new QTableWidgetItem(QString("%1").arg(m_selectAnalog[i].Ka));
		ui.tableWidget_adjustresult->setItem(i, 5, item);
	}
}

void dtuadjustwidget::start_calibrate()
{
	if (ui.radio_amplitude->isChecked())
	{
		SAVE_FLAG = FLAG_Amplitude;
	}
	else
	{
		SAVE_FLAG = FLAG_Phase;
	}
	ChnACCalibrate();
}

void dtuadjustwidget::select_channel()
{
	dtuparamchannelDlg dlg;
	dlg.setWindowTitle("通道选择[*]");
	dlg.setWindowModified(false);
	dlg.load_channellist(m_selectAnalog, _analogList);
	if (dlg.exec() == 1)
	{
		dlg.get_channellist(m_selectAnalog);
	}
}

void dtuadjustwidget::show_result()
{
	//// 显示整定结果
	if (m_selectAnalog.empty() || !finashFirst)
	{
		// 显示已有的整定参数值
		if (QMessageBox::Yes == QMessageBox::information(this, "提示",
			"没有当前整定结果,是否显示上次结果", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
			loadCalibrateResult();
		}
		else
		{
			return;
		}
	}
	else
	{
		curMenu = ADJUSTRESULT;
		showAdjust();
	}
}

void dtuadjustwidget::save_result()
{
	if (m_selectAnalog.empty())
	{
		if (QMessageBox::Yes == QMessageBox::information(this, "提示",
					"没有当前整定结果,是否下发上次结果", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
			SendResult();
		}
		else
		{
			return;
		}
	}
	else
	{
		SaveParam();
	}
}

void dtuadjustwidget::restart_calibrate()
{
	ui.btn_start->setText("开始");
	ui.btn_start->setEnabled(true);
	ui.btn_cancel_last->setEnabled(false);
	//ui.btn_display_result->setEnabled(false);

	ui.doubleSpinBox_input_cur->setValue(0.0);
	ui.doubleSpinBox_input_vol->setValue(0.0);
	ui.doubleSpinBox_vol_step->setValue(0.0);
	ui.doubleSpinBox_cur_step->setValue(0.0);

	ui.listWidget_operate->clear();
	m_nStep = 0;
	zero_code_storage.clear();
	//_cycdata.clear_data();
}

void dtuadjustwidget::bgGroup_toggled(int id, bool status)
{
	if (id == 0) {
		if (status) {
			ui.doubleSpinBox_cur_step->setEnabled(true);
			ui.doubleSpinBox_vol_step->setEnabled(true);
		}
		else
		{
			ui.doubleSpinBox_cur_step->setEnabled(false);
			ui.doubleSpinBox_vol_step->setEnabled(false);
		}
	}
	else
	{
		if (status) {
			ui.doubleSpinBox_cur_step->setEnabled(false);
			ui.doubleSpinBox_vol_step->setEnabled(false);
		}
		else
		{
			ui.doubleSpinBox_cur_step->setEnabled(true);
			ui.doubleSpinBox_vol_step->setEnabled(true);
		}
	}
}

void dtuadjustwidget::cancel_last_step()
{
	ui.listWidget_operate->removeItemWidget(ui.listWidget_operate->takeItem(ui.listWidget_operate->count()-1));
	if (ui.radio_amplitude->isChecked())
	{
		if (m_nStep != 0)
		{
			auto vCur = ui.doubleSpinBox_input_cur->value();
			vCur -= ui.doubleSpinBox_cur_step->value();
			ui.doubleSpinBox_input_cur->setValue(vCur);

			auto vVol = ui.doubleSpinBox_input_vol->value();
			vVol -= ui.doubleSpinBox_vol_step->value();
			ui.doubleSpinBox_input_vol->setValue(vVol);
		}
	}
	if (m_nStep > 0)
	{
		m_nStep--;
	}
	if (m_nStep == 0)
	{
		ui.btn_cancel_last->setEnabled(false);
	}
}

void dtuadjustwidget::get_adjust_result()
{
	SIGNALSEND("读取整定参数", 0, 50, "读取整定参数", true);
	DTU::buffer result;
	if (DTU_SUCCESS == execute_query_data(PC_R_ADJ_FIX, result))
	{
		DTULOG(DTU_INFO, (char*)"读取整定参数成功! 当前整定定值数据长度%u",result.size());
		_adjParam.Adjdata().remove();
		_adjParam.Adjdata().append(result);
		SIGNALSEND("读取整定参数", 50, 100, "读取整定参数成功", true);
		// 显示
		loadCalibrateResult(false);
		// 存入本地
		_adjParam.save();
		if (!firstGetValue) 
		{ 
			firstGetValue = true;
			ui.btn_start->setEnabled(true);
			ui.label_tip->hide();
		}
		curMenu = ALLRESULT;
	}
	else
	{
		SIGNALSEND("读取整定参数失败", 50, 100, "读取整定参数", true);
		DTULOG(DTU_ERROR, "读取整定参数失败!");
	}
}

void dtuadjustwidget::clear_tablewigdet_show()
{
	for (int row = ui.tableWidget_adjustresult->rowCount() - 1; row >= 0; row--)
	{
		ui.tableWidget_adjustresult->removeRow(row);
	}
}

void dtuadjustwidget::modify_adjust_result()
{
	auto row = ui.tableWidget_adjustresult->currentRow();
	auto column = ui.tableWidget_adjustresult->currentColumn();
	int ch_no = ui.tableWidget_adjustresult->item(row, 0)->text().toInt();
	dtumodifyadjustDlg *ModifyAdjustDlg = new dtumodifyadjustDlg(_adjParam, m_selectAnalog, curMenu, ch_no, column);
	ModifyAdjustDlg->exec();

	ModifyAdjustDlg->get_result(_adjParam,m_selectAnalog);

	if (curMenu)
	{
		// 显示全部结果
		loadCalibrateResult(false);
	}
	else
	{
		// 显示整定结果
		showAdjust();
	}

	delete ModifyAdjustDlg;
	ModifyAdjustDlg = nullptr;

}

void dtuadjustwidget::TimeOutFunc()
{
	if (!adjustFinish)
	{
		ui.btn_start->setEnabled(true);
	}
}