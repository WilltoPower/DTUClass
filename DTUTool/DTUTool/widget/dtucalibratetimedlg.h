#ifndef DTUCALIBRATETIMEDLG_H
#define DTUCALIBRATETIMEDLG_H

#include <QDialog>

#include <atomic>
#include <thread>
#include <memory>

#include <time.h>
#include <windows.h>

#include <QCloseEvent>

#include "dtuprotocol.h"
#include "dtutask.h"

namespace Ui {
class dtuCalibrateTimeDlg;
}

class dtuCalibrateTimeDlg : public QDialog
{
    Q_OBJECT

public:
    explicit dtuCalibrateTimeDlg(QWidget *parent = 0);
    ~dtuCalibrateTimeDlg();

public:
	void load_ui();
	void showTime();

private:
    Ui::dtuCalibrateTimeDlg *ui;
private:
	void run();

public:
	void start();
	void stop();

private:
    // 定时读取数据多线程
    std::unique_ptr<std::thread> _query_local_thread;
	std::unique_ptr<std::thread> _query_device_thread;
	std::unique_ptr<std::thread> _time_skew;
    bool _run_query = true;
    void query_local_time();
	void query_device_time();
	void time_skew();
	std::atomic<uint32_t> deviceTime = 0;
	std::atomic<uint32_t> systemTime = 0;
	std::atomic_bool _run_auto_time = false;
	bool auto_modify = false;

private slots:
    void Auto_Calibrate_Time();
    void Manual_Calibrate_Time();

private:
	bool queryData = false;

};

#endif // DTUCALIBRATETIMEDLG_H
