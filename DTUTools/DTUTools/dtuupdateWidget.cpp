#include "dtuupdateWidget.h"

#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QProgressDialog>
#include <complex>
#include <tuple>

#include <dtulog.h>
#include "dtutask.h"
#include "DTUCmdCode.h"

#include <windows.h>

dtuupdateWidget::dtuupdateWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	enable_update();
	this->setWindowModality(Qt::ApplicationModal);
	ui.progressBar->setValue(0);
	ui.progressBar->hide();
	ui.lineEdit_dsp->setPlaceholderText(" taeho-8101-9200b.img taeho-8100-9200a.img");
	ui.lineEdit_system->setPlaceholderText(" u-boot-860.img u-boot-spl.brn linux-dtu.itb");
	ui.lineEdit_config->setPlaceholderText(" Json XML DB");
	ui.lineEdit_arm->setPlaceholderText(" SDL9200VXXRXXCXXBXX.out");
	ui.lineEdit_goose->setPlaceholderText(" SDL9200GOOSESService");
}

dtuupdateWidget::~dtuupdateWidget()
{
	
}

//
void dtuupdateWidget::select_system_program()
{
	_path_system = QFileDialog::getOpenFileNames(this, "选择系统程序文件", "./update/","系统文件(u-boot-860.img u-boot-spl.brn linux-dtu.itb)");
	if (_path_system.isEmpty()) {
		return;
	}

	QString fileName;

	for (auto item : _path_system)
	{
		auto fullname = item.right(item.size() - (item.lastIndexOf('/') + 1));
		_path_dsp_list.emplace_back(std::make_tuple(item, fullname));
		fileName = fileName + " .../" + fullname;
	}
	ui.lineEdit_system->setText(fileName);
}

void dtuupdateWidget::select_arm_program()
{
	_path_arm = QFileDialog::getOpenFileName(this, "选择SDL9200程序文件", "./update/", "bin文件(*.out)");
	if (_path_arm.isEmpty()) {
		return;
	}
	ui.lineEdit_arm->setText(_path_arm);
}

void dtuupdateWidget::select_goose_program()
{
	_path_goose = QFileDialog::getOpenFileName(this, "选择SDL9200G程序文件", "./update/");
	if (_path_goose.isEmpty())
		return;
	ui.lineEdit_goose->setText(_path_goose);
}

void dtuupdateWidget::select_dsp_program()
{
	_path_dsp = QFileDialog::getOpenFileNames(this, "选择DSP程序文件(可多选)", "./update/", "img文件(taeho-8101-9200b.img taeho-8100-9200a.img)");
	if (_path_dsp.isEmpty()) {
		return;
	}

	QString fileName;

	for (auto item : _path_dsp)
	{
		auto fullname = item.right(item.size() - (item.lastIndexOf('/') + 1));
		_path_dsp_list.emplace_back(std::make_tuple(item, fullname));
		fileName = fileName + " .../" + fullname;
	}
	ui.lineEdit_dsp->setText(fileName);
}

void dtuupdateWidget::select_config_program()
{
	_path_config = QFileDialog::getOpenFileNames(this, "选择配置更新文件(可多选)", "./config/", "配置文件(*.xml *.json *.db)");
	if (_path_config.isEmpty()) {
		return;
	}

	QString fileName;
	
	for (auto item : _path_config)
	{
		auto fullname =  item.right(item.size() - (item.lastIndexOf('/') +1));
		_path_config_list.emplace_back(std::make_tuple(item,fullname));
		fileName = fileName + " .../" + fullname;
	}
	ui.lineEdit_config->setText(fileName);
}
//
void dtuupdateWidget::execute_update_program()
{
	ui.Ltxt->setText("升级中...  请勿进行其他操作");
	ui.btn_update->setEnabled(false);
	uint16_t tag = 0;
	bool bExecute = true;
	ui.progressBar->show();
	// 下发文件

	// 系统文件
	if (ui.checkBox_system->isChecked() && (!_path_system_list.empty())) {
		for (auto item : _path_system_list)
		{
			bExecute = translateFile(std::get<0>(item).toStdString(), "/update/system/" + std::get<1>(item).toStdString());
		}
		tag |= 0x001;
	}
	ui.progressBar->setValue(25);
	// DSP文件
	if (ui.checkBox_dsp->isChecked() && (!_path_dsp_list.empty())) {
		for (auto item:_path_dsp_list)
		{
			bExecute = translateFile(std::get<0>(item).toStdString(), "/update/" + std::get<1>(item).toStdString());
		}
		tag |= 0x010;
	}
	ui.progressBar->setValue(50);
	// ARM文件
	if (ui.checkBox_arm->isChecked() && (!_path_arm.isEmpty())) {
		bExecute = translateFile(_path_arm.toStdString(), "/update/arm");
		tag |= 0x100;
	}
	ui.progressBar->setValue(75);
	// 配置文件
	if (ui.checkBox_config->isChecked() && (!_path_config_list.empty())) {
		for (auto item : _path_config_list)
		{
			bExecute = translateFile(std::get<0>(item).toStdString(), "/update/config/" + std::get<1>(item).toStdString());
		}

		tag |= 0x1000;
	}
	// GOOSE文件
	if (ui.checkBox_goose->isChecked() && (!_path_goose.isEmpty())) {
		bExecute = translateFile(_path_goose.toStdString(), "/update/goose");
		tag |= 0x02; // Tag的数必须是偶数
	}
	ui.progressBar->setValue(85);
	// 下发控制指令
	if (bExecute)
	{
		execute_updateprogram(tag);
		QtDTULOG(DTU_INFO, "后台正在执行升级, 请稍后...");
	}
	ui.progressBar->setValue(100);
	ui.Ltxt->setText("升级完成");
	ui.btn_update->setEnabled(true);
}

void dtuupdateWidget::enable_update()
{
	//ARM
	if (ui.checkBox_arm->isChecked()) {
		ui.lineEdit_arm->setEnabled(true);
		ui.btn_arm->setEnabled(true);
	}
	else {
		ui.lineEdit_arm->setEnabled(false);
		ui.btn_arm->setEnabled(false);
	}
	//GOOSE
	if (ui.checkBox_goose->isChecked()) {
		ui.lineEdit_goose->setEnabled(true);
		ui.btn_goose->setEnabled(true);
	}
	else {
		ui.lineEdit_goose->setEnabled(false);
		ui.btn_goose->setEnabled(false);
	}
	//DSP
	if (ui.checkBox_dsp->isChecked()) {
		ui.lineEdit_dsp->setEnabled(true);
		ui.btn_dsp->setEnabled(true);
	}
	else {
		ui.lineEdit_dsp->setEnabled(false);
		ui.btn_dsp->setEnabled(false);
	}
	//系统升级
	if (ui.checkBox_system->isChecked()) {
		ui.lineEdit_system->setEnabled(true);
		ui.btn_system->setEnabled(true);
	}
	else {
		ui.lineEdit_system->setEnabled(false);
		ui.btn_system->setEnabled(false);
	}
	//Config
	if (ui.checkBox_config->isChecked()) {
		ui.lineEdit_config->setEnabled(true);
		ui.btn_config->setEnabled(true);
	}
	else {
		ui.lineEdit_config->setEnabled(false);
		ui.btn_config->setEnabled(false);
	}
}

bool dtuupdateWidget::translateFile(const std::string& src, const std::string destfile)
{
	try
	{
		std::string execPath;
		if (DTU_FAILED(execute_get_filepath(execPath)))
		{
			return false;
		}
		if (DTU_FAILED(execute_set_file(src, execPath + destfile)))
		{
			return false;
		}
	}
	catch (std::exception& e)
	{
		QtDTULOG(DTU_ERROR, "传送文件%s异常, %s", src.c_str(),e.what());
		return false;
	}
	return true;
}
