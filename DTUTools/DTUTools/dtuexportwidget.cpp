#include "dtuexportwidget.h"

#include <QFileDialog>
#include <QTime>
#include <QDate>
#include <QMessageBox>

#include <fstream>
#include <filesystem>

#include "dtutask.h"
#include "dtulog.h"
#include "dtucommon.h"

dtuexportwidget::dtuexportwidget(uint16_t type, QString desc, int max, uint16_t exporttype,QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::dtuexportwidgetClass())
{
	ui->setupUi(this);
	this->type = static_cast<EXPORTER>(type);
	this->exporttype = exporttype;
	ui->spin_max->setMaximum(max);
	ui->spin_max->setValue(max);
	ui->spin_min->setMaximum(max);
	ui->lab_count->setText(QString::number(max));
	ui->lab_what->setText(desc);
}

dtuexportwidget::~dtuexportwidget()
{
	delete ui;
}

void dtuexportwidget::export_all()
{
	switch (type)
	{
	case EXPREPORT: {
		QString path = SelestSavePath(1);
		if (path.isEmpty()) {
			return;
		}
		export_report(ui->spin_min->value(), ui->spin_max->value(), exporttype, 
						DBManager::instance().GetReportInfoByID(exporttype).desc, path);
	}; break;
	case EXPREPORTFILE: {
		QString path = SelestSavePath(2);
		if (path.isEmpty()) {
			return;
		}
		export_report_file(ui->spin_min->value(), ui->spin_max->value(), exporttype,
						DBManager::instance().GetReportInfoByID(exporttype).desc, path);
	}; break;
	case EXPCSPROTO: {
		QString path = SelestSavePath(1);
		if (path.isEmpty()) {
			return;
		}
		export_csproto(ui->spin_min->value() - 1, ui->spin_max->value() - 1, exporttype, "规约", path);
	}; break;
	}
	
}

void dtuexportwidget::export_report(int min, int max, int type, std::string desc, QString path)
{
	ui->progressBar->setValue(0);

	int no = min;
	int maxcounter = max - min + 1;
	int counter = 0;
	float add = 100.0 / maxcounter;	// 进度条每次增加的量

	ReportBufferAttr result;
	if (DTU_SUCCESS != execute_get_report(exporttype, min, max, result)) {
		return;
	}

	pugi::xml_document doc;
	doc.reset();
	pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
	xml_pre_node.append_attribute("version") = "1.0";
	xml_pre_node.append_attribute("encoding") = "UTF-8";

	pugi::xml_node xml_root = doc.append_child("Export");
	xml_root.append_attribute("description") = desc.c_str();
	xml_root.append_attribute("firstno") = min;
	xml_root.append_attribute("lastno") = max;
	xml_root.append_attribute("num") = max - min + 1;

	switch (static_cast<ReportID>(type))
	{
	case ReportSOE: {
		for (auto &item : result)
		{
			addtoSOE(xml_root,std::get<2>(item.second),no);
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	case ReportOPT: {
		for (auto &item : result)
		{
			addtoOPT(xml_root, std::get<2>(item.second), no);
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	case ReportWAR: {
		for (auto &item : result)
		{
			addtoWAR(xml_root, std::get<2>(item.second), no);
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	}
	doc.save_file(path.toStdString().c_str());
	QMessageBox::information(this, "导出文件", "导出文件完成", QMessageBox::Ok);
}

void dtuexportwidget::export_report_file(int min, int max, int type, std::string desc, QString path)
{
	ui->progressBar->setValue(0);

	int no = min;
	int maxcounter = max - min + 1;
	int counter = 0;
	float add = 100.0 / maxcounter;	// 进度条每次增加的量

	ReportBufferAttr result;
	if (DTU_SUCCESS != execute_get_report(exporttype, min, max, result)) {
		return;
	}

	switch (type)
	{
	case ReportPro: {
		static std::vector<std::string> suffix = {""};
		for (auto &item : result)
		{
			get_file(std::get<2>(item.second), "/protect/protect/", suffix, path.toStdString());
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	case ReportTransRcd: {
		static std::vector<std::string> suffix = { ".cfg",".dat" };
		for (auto &item : result)
		{
			get_file(std::get<2>(item.second), "/protect/comtrade/", suffix, path.toStdString());
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	case ReportWorkRcd: {
		static std::vector<std::string> suffix = { ".cfg",".dat" };
		for (auto &item : result)
		{
			get_file(std::get<2>(item.second), "/COMTRADE/", suffix, path.toStdString());
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
	}; break;
	}
	QMessageBox::information(this, "导出文件", "导出文件完成", QMessageBox::Ok);
}

void dtuexportwidget::export_csproto(int min, int max, int type, std::string desc, QString path)
{
	if (csprotoWidget != nullptr) {
		ui->progressBar->setValue(0);

		if (csprotoWidget == nullptr) {
			return;
		}

		int no = min;
		int maxcounter = max - min + 1;
		int counter = 0;
		float add = 100.0 / maxcounter;	// 进度条每次增加的量

		pugi::xml_document doc;
		doc.reset();
		pugi::xml_node xml_pre_node = doc.prepend_child(pugi::node_declaration);
		xml_pre_node.append_attribute("version") = "1.0";
		xml_pre_node.append_attribute("encoding") = "UTF-8";

		pugi::xml_node xml_root = doc.append_child("Export");
		std::string desc = std::to_string(type) + "规约报文";
		xml_root.append_attribute("description") = desc.c_str();
		xml_root.append_attribute("firstno") = min;
		xml_root.append_attribute("lastno") = max;
		xml_root.append_attribute("num") = max - min + 1;

		for (int i=min; i<=max; i++)
		{
			addtoCSProto(xml_root, csprotoWidget, i);
			counter += static_cast<int>(add);
			if (no == max) {
				counter = 100;
			}
			no++;
			ui->progressBar->setValue(counter);
		}
		doc.save_file(path.toStdString().c_str());
		QMessageBox::information(this, "导出文件", "导出文件完成", QMessageBox::Ok);
	}
}

void dtuexportwidget::addtoSOE(pugi::xml_node &node, DTU::buffer &data,int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	// 
	uint16_t type = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t fix = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	uint16_t statuschange = data.get(14, sizeof(uint16_t)).value<uint16_t>();
	QString state = QString("%1→%2").arg(LOBYTE(statuschange)).arg(HIBYTE(statuschange));

	std::string soename;
	if (fix == 0) {
		soename = "SOE值为0 TYPE=" + std::to_string(type);
	}
	else {
		auto ret = DTU::DBManager::instance().GetSOEIndex();
		auto ita = ret.find(fix);
		if (ita != ret.end()) {
			soename = ita->second.desc;
		}
		else {
			soename = "未知SOE=" + std::to_string(fix) + " TYPE=" + std::to_string(type);
		}
	}

	pugi::xml_node xml_add = node.append_child("data");
	xml_add.append_attribute("no") = no;
	xml_add.append_attribute("time") = strtime.c_str();

	if (type == 0x0001) {
		xml_add.append_attribute("type") = "hard";// 硬遥信
	}
	else if (type == 0x0002) {
		xml_add.append_attribute("type") = "soft";// 软遥信
	}
	xml_add.append_attribute("info") = soename.c_str();
	xml_add.append_attribute("change") = state.toStdString().c_str();
}

void dtuexportwidget::addtoOPT(pugi::xml_node &node, DTU::buffer &data, int no)
{ 
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	//
	uint16_t optsrc = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t optobj = data.get(12, sizeof(uint16_t)).value<uint16_t>();

	static std::map<uint16_t, QString> srcmap = {
		{1, "本地"},{2, "远方"}
	};
	static std::map<uint16_t, QString> objmap = {
		{1, "遥控分闸"},{2, "遥控合闸"},
		{3, "活化启动"},{4, "活化退出"},
		{5, "信号复归"},{6, "公共定值"},
		{7, "保护定值"},{8, "保护控制字"},
		{9, "就地FA定值"},{10, "就地FA控制字"}
	};
	QString strsrc;
	auto srcita = srcmap.find(optsrc);
	if (srcita == srcmap.end()) {
		strsrc = "未知操作源";
	}
	else {
		strsrc = srcita->second;
	}
	QString strobj;
	auto objita = objmap.find(optobj);
	if (objita == objmap.end()) {
		strobj = "未知操作";
	}
	else {
		strobj = objita->second;
	}
	
	pugi::xml_node xml_add = node.append_child("data");
	xml_add.append_attribute("no") = no;
	xml_add.append_attribute("time") = strtime.c_str();
	xml_add.append_attribute("operate") = strobj.toStdString().c_str();
	xml_add.append_attribute("from") = strsrc.toStdString().c_str();
}

void dtuexportwidget::addtoWAR(pugi::xml_node &node, DTU::buffer &data, int no)
{
	uint32_t seconds = data.get(0, sizeof(uint32_t)).value<uint32_t>();
	uint32_t micosec = data.get(sizeof(uint32_t), sizeof(uint32_t)).value<uint32_t>();
	uint64_t totlemircoseconds = (uint64_t)seconds * 1000000 + (uint64_t)micosec;
	std::string strtime = create_time_from_mirco(totlemircoseconds);
	uint16_t warnno = data.get(10, sizeof(uint16_t)).value<uint16_t>();
	uint16_t statuschange = data.get(12, sizeof(uint16_t)).value<uint16_t>();
	QString state = QString("%1→%2").arg(LOBYTE(statuschange)).arg(HIBYTE(statuschange));

	QString content;

	switch (warnno)
	{
	case 1:content = "PT断线"; break;
	case 2:content = "控制回路断线"; break;
	case 3:content = "线路断线"; break;
	case 4:content = "保护功能投入冲突告警"; break;
	default:
		content = "未知告警";
		break;
	}

	pugi::xml_node xml_add = node.append_child("data");
	xml_add.append_attribute("no") = no;
	xml_add.append_attribute("time") = strtime.c_str();
	xml_add.append_attribute("operate") = content.toStdString().c_str();
	xml_add.append_attribute("from") = state.toStdString().c_str();
}

void dtuexportwidget::addtoCSProto(pugi::xml_node &node, QTableWidget *table,int row)
{
	pugi::xml_node xml_add = node.append_child("data");
	xml_add.append_attribute("no") = row + 1;
	xml_add.append_attribute("time") = table->item(row, 2)->text().toStdString().c_str();
	xml_add.append_attribute("dir") = table->item(row, 0)->text().toStdString().c_str();
	xml_add.append_attribute("data") = table->item(row, 4)->text().toStdString().c_str();
}

void dtuexportwidget::get_file(DTU::buffer &data,std::string armpath, std::vector<std::string> &suffix, std::string path)
{
	QString fullPath;
	std::string fileName(data.const_data(), data.size());
	get_file_from_arm(armpath, path, fileName, suffix, fullPath);
}

bool dtuexportwidget::get_file_from_arm(std::string armPath, std::string localPath, std::string fileName, std::vector<std::string> suffix, QString &FullPath)
{
	if (suffix.empty())
		return false;

	// 查找文件是否存在
	std::string file = localPath + "/" + fileName + suffix[0];

	FullPath = QString::fromStdString(file);

	if (execute_test_arm_connect())
	{
		// ARM已经连接无论是否存在文件都刷新 因为可能产生同名文件无法进行覆盖
		for (const auto& item : suffix)
		{
			std::string fullName = fileName + item;
			DTU::buffer result;
			if (DTU_SUCCESS != execute_get_file(armPath + fullName, result))
			{
				QMessageBox::information(this, "错误", "无法获取文件", QMessageBox::Ok);
				return false;
			}
			// 保存文件
			fullName = localPath + "/" + fullName;
			std::ofstream ofs;
			ofs.open(fullName, std::ios::binary | std::ios::app);
			ofs.write(result.const_data(), result.size());
			ofs.close();
		}
		return true;
	}
	else
	{
		return false;
	}
}

QString dtuexportwidget::SelestSavePath(int type)
{
	QString ret;
	QTime CurTime = QTime::currentTime();
	QDate CurDate = QDate::currentDate();
	switch (type)
	{
	case 1: {
		char buf[128] = {};
		sprintf(buf, "OutPuter%04d%02d%02d_%02d_%02d_%02d_%03d.xml",
			CurDate.year(), CurDate.month(), CurDate.day(),
			CurTime.hour(), CurTime.minute(), CurTime.second(), CurTime.msec());

		QString filename = "./output/" + QString::fromLatin1(buf);
		QString path = QFileDialog::getSaveFileName(this, "选择保存文件位置", filename, "XML (*.xml)");
		ret = path;
	}; break;
	case 2: {
		char buf[128] = {};
		sprintf(buf, "OutPuter%04d%02d%02d_%02d_%02d_%02d_%03d",
			CurDate.year(), CurDate.month(), CurDate.day(),
			CurTime.hour(), CurTime.minute(), CurTime.second(), CurTime.msec());
		QString filedir = "./output/" + QString::fromLatin1(buf);
		QString dir = QFileDialog::getExistingDirectory(this, "选择一个文件夹",
			filedir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (!dir.isEmpty()) {
			if (create_dir(dir + "/" + QString::fromLatin1(buf))) {
				dir = dir + +"/" + QString::fromLatin1(buf);
			}
		}
		ret = dir;
	}; break;
	}
	return ret;
}

bool dtuexportwidget::create_dir(QString fullpath)
{
	QDir dir(fullpath);
	if (!dir.exists()) {
		return dir.mkdir(fullpath);
	}
	return false;
}

void dtuexportwidget::setWidgetPointer(QTableWidget *csprotoWidget)
{
	this->csprotoWidget = csprotoWidget;
}