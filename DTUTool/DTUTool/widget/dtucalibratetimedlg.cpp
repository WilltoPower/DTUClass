#pragma execution_character_set("utf-8")

#include "dtucalibratetimedlg.h"
#include "ui_dtucalibratetimedlg.h"
#include <QDebug>

#include "dtulog.h"
#include "DTUCmdCode.h"
#include <QTime>
#include <dtucommon.h>
#include <iostream>

#define TimeBeginTimeStamp 946656000

#define QSTR(str) QString::fromLocal8Bit(str)

dtuCalibrateTimeDlg::dtuCalibrateTimeDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dtuCalibrateTimeDlg)
{
    ui->setupUi(this);

	this->load_ui();

 //   _query_local_thread = std::make_unique<std::thread>(&dtuCalibrateTimeDlg::query_local_time, this);
	//_query_device_thread = std::make_unique<std::thread>(&dtuCalibrateTimeDlg::query_device_time, this);
	//_time_skew = std::make_unique<std::thread>(&dtuCalibrateTimeDlg::time_skew, this);
}

void dtuCalibrateTimeDlg::load_ui()
{
	this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
	this->setWindowTitle("时间校准");
	ui->lineEdit_auto_time->setEnabled(false);
	ui->lineEdit_device_time->setEnabled(false);

	ui->btn_auto_time->hide();
}

void dtuCalibrateTimeDlg::showTime()
{
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	ui->spin_manu_year->setValue(systime.wYear);
	ui->spin_manu_month->setValue(systime.wMonth);
	ui->spin_manu_day->setValue(systime.wDay);
	ui->spin_manu_hour->setValue(systime.wHour);
	ui->spin_manu_min->setValue(systime.wMinute);
	ui->spin_manu_sec->setValue(systime.wSecond);
}

dtuCalibrateTimeDlg::~dtuCalibrateTimeDlg()
{
	//在退出窗口之前，实现希望做的操作
	_run_query = false;
	this->stop();
    delete ui;
}

void dtuCalibrateTimeDlg::query_local_time()
{
	while (_run_query)
	{
		if (queryData) {
			SYSTEMTIME systime;
			GetLocalTime(&systime);
			char buf[32];
			sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
			QString time(buf);
			ui->lineEdit_auto_time->setText(time);
			QDateTime time_t = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
			uint32_t cur_time = time_t.toTime_t() - TimeBeginTimeStamp;
			systemTime = cur_time;
		}
		//休眠
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void dtuCalibrateTimeDlg::query_device_time()
{
	while (_run_query)
	{
		if (queryData) {
			printf("qury device time\n");
			try
			{
				static int count = 0;
				DTU::buffer result;
				if (execute_query_data(PC_R_CLK, result) != DTU_SUCCESS)
				{
					count++;
					if (count % 10 == 0)
					{
						DTULOG(DTU_ERROR, "查询设备时间出错");
					}
					return;
				}

				uint16_t year = result.get(0, sizeof(uint16_t)).value<uint16_t>();
				uint8_t mon = result.get(sizeof(uint16_t) * 1, sizeof(uint8_t)).value<uint8_t>();
				uint8_t day = result.get(sizeof(uint8_t) * 3, sizeof(uint8_t)).value<uint8_t>();
				uint8_t hh = result.get(sizeof(uint8_t) * 4, sizeof(uint8_t)).value<uint8_t>();
				uint8_t mm = result.get(sizeof(uint8_t) * 5, sizeof(uint8_t)).value<uint8_t>();
				uint8_t ss = result.get(sizeof(uint8_t) * 6, sizeof(uint8_t)).value<uint8_t>();
				uint16_t ms = result.get(sizeof(uint8_t) * 7, sizeof(uint16_t)).value<uint16_t>();
				char buf[32];
				sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hh, mm, ss);
				//QString time = QString::number(year) + "-" + QString::number(mon) + "-" + QString::number(day) + " " + QString::number(hh) + ":" + QString::number(mm) + ":" + QString::number(ss);
				QString time(buf);

				QDateTime qtime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
				deviceTime = qtime.toTime_t() - TimeBeginTimeStamp;

				ui->lineEdit_device_time->setText(time);
			}
			catch (std::exception& e)
			{
				DTULOG(DTU_ERROR, "query_device_time() %s", e.what());
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(900));
	}
}

void dtuCalibrateTimeDlg::time_skew()
{
	while (_run_query)
	{
		if (queryData) {
			uint32_t device_time = deviceTime;
			uint32_t system_time = systemTime;
			uint32_t retTime;
			static int count = 10;//减少对时次数
			if (device_time >= system_time)
			{
				retTime = device_time - system_time;
				ui->clock_skew->setText(QString::number(retTime));
			}
			else
			{
				retTime = system_time - device_time;
				ui->clock_skew->setText(QString::number(retTime));
			}

			if (retTime <= 3)//误差小于3秒不用对时
			{
				continue;
			}

			if (_run_auto_time)
			{
				count--;
				if (count < 0)
				{
					SYSTEMTIME systime;
					GetLocalTime(&systime);
					char buf[32];
					sprintf_s(buf, "%d-%02d-%02d %02d:%02d:%02d", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
					QString time(buf);
					QDateTime time_t = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
					uint32_t cur_time = time_t.toTime_t() - TimeBeginTimeStamp;
					DTU::buffer data;
					data.append((char*)&cur_time, sizeof(uint32_t));
					if (DTU_SUCCESS != execute_write_data(PC_W_CLK, data))
					{
						DTULOG(DTU_ERROR, "自动写入时间参数错误");
						return;
					}
					DTULOG(DTU_INFO, "自动写入时间参数");
					count = 10;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void dtuCalibrateTimeDlg::Auto_Calibrate_Time()
{
	auto_modify = !auto_modify;
	if (auto_modify)
	{
		ui->btn_auto_time->setText(QSTR("自动校时(执行)"));
		ui->btn_manual_time->setEnabled(false);
		_run_auto_time = true;
	}
	else
	{
		ui->btn_auto_time->setText(QSTR("自动校时"));
		ui->btn_manual_time->setEnabled(true);
		_run_auto_time = false;
	}
}

void dtuCalibrateTimeDlg::Manual_Calibrate_Time()
{
	char buf[64];
	sprintf_s(buf, "%u-%02u-%02u %02u:%02u:%02u", ui->spin_manu_year->text().toUInt(),
		ui->spin_manu_month->text().toUInt(),
		ui->spin_manu_day->text().toUInt(),
		ui->spin_manu_hour->text().toUInt(),
		ui->spin_manu_min->text().toUInt(),
		ui->spin_manu_sec->text().toUInt());
	QString time(buf);
	QDateTime time_t = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
	uint32_t cur_time = time_t.toTime_t() - TimeBeginTimeStamp;
	DTU::buffer data;
	// 放入总秒数
	data.append((char*)&cur_time, sizeof(uint32_t));
	cur_time = 0;
	// 放入微妙数
	data.append((char*)&cur_time, sizeof(uint32_t));
	if (DTU_SUCCESS != execute_write_data(PC_W_CLK, data))
	{
		DTULOG(DTU_ERROR, "手动写入时间参数错误");
		return;
	}
	DTULOG(DTU_INFO, "手动写入时间参数");
}

void dtuCalibrateTimeDlg::run()
{
	std::thread Thread_quryDeviceTime([&]() {
		while (_run_query)
		{
			if (queryData) {
				try
				{
					static int count = 0;
					DTU::buffer result;
					if (execute_query_data(PC_R_CLK, result) != DTU_SUCCESS)
					{
						count++;
						if (count % 10 == 0)
						{
							DTULOG(DTU_ERROR, "查询设备时间出错");
						}
						return;
					}

					uint16_t year = result.get(0, sizeof(uint16_t)).value<uint16_t>();
					uint8_t mon = result.get(sizeof(uint16_t) * 1, sizeof(uint8_t)).value<uint8_t>();
					uint8_t day = result.get(sizeof(uint8_t) * 3, sizeof(uint8_t)).value<uint8_t>();
					uint8_t hh = result.get(sizeof(uint8_t) * 4, sizeof(uint8_t)).value<uint8_t>();
					uint8_t mm = result.get(sizeof(uint8_t) * 5, sizeof(uint8_t)).value<uint8_t>();
					uint8_t ss = result.get(sizeof(uint8_t) * 6, sizeof(uint8_t)).value<uint8_t>();
					uint16_t ms = result.get(sizeof(uint8_t) * 7, sizeof(uint16_t)).value<uint16_t>();
					char buf[32];
					sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hh, mm, ss);
					//QString time = QString::number(year) + "-" + QString::number(mon) + "-" + QString::number(day) + " " + QString::number(hh) + ":" + QString::number(mm) + ":" + QString::number(ss);
					QString time(buf);

					QDateTime qtime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
					deviceTime = qtime.toTime_t() - TimeBeginTimeStamp;

					ui->lineEdit_device_time->setText(time);
				}
				catch (std::exception& e)
				{
					DTULOG(DTU_ERROR, "query_device_time() %s", e.what());
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(900));
		}
	});
	Thread_quryDeviceTime.detach();

	std::thread Thread_quryLocalTime([&]() {
		try
		{
			while (_run_query) {
				if (queryData) {
					SYSTEMTIME systime;
					GetLocalTime(&systime);
					char buf[32];
					sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
					QString time(buf);
					ui->lineEdit_auto_time->setText(time);
					QDateTime time_t = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");
					uint32_t cur_time = time_t.toTime_t() - TimeBeginTimeStamp;
					systemTime = cur_time;
				}
				//休眠
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}
		catch (std::exception &e)
		{
			DTULOG(DTU_ERROR, "发生未知错误%s", e.what());
		}
	});
	Thread_quryLocalTime.detach();

	std::thread Thread_timeSkew([&]() {
		try 
		{
			while (_run_query) {
				if (queryData) {
					uint32_t device_time = deviceTime;
					uint32_t system_time = systemTime;
					uint32_t retTime;
					if (device_time >= system_time) {
						retTime = device_time - system_time;
						ui->clock_skew->setText(QString::number(retTime));
					}
					else {
						retTime = system_time - device_time;
						ui->clock_skew->setText(QString::number(retTime));
					}
				}
			}
		}
		catch (std::exception &e)
		{
			DTULOG(DTU_ERROR,"发生未知错误%s",e.what());
		}
	});
	Thread_timeSkew.detach();
}

void dtuCalibrateTimeDlg::start()
{
	if (!queryData) {
		this->run();
		queryData = true;
	}
}

void dtuCalibrateTimeDlg::stop()
{
	if (queryData) {
		queryData = false;
	}
}