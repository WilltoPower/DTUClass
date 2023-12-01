/*********************************************************************************
  *Copyright(C),2021-2025,sddl
  *FileName:  dtulog.h
  *Description: 
    通用功能定义
  *History: 
    1, 创建, wangjs, 2021-8-10
**********************************************************************************/
#include "dtucommon.h"
#ifndef _WIN32
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>  
#include <string.h>
#include <iconv.h>
#include <sys/stat.h>
#include <json/json.h>
#define MAX_PATH 260
#else
#include <windows.h>
#include <filesystem>
#endif
#include "dtulog.h"
#include <chrono>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <regex>

#include <time.h>
#include <stdint.h>

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#define EPOCHFILETIME   (116444736000000000UL)
#endif

typedef struct _sdlSYSTEMTIME
{
	uint16_t	wYear;
	uint16_t	wMonth;
	uint16_t	wDay;
	uint16_t	wHour;
	uint16_t	wMinute;
	uint16_t	wSecond;
	uint16_t	wMilliseconds;
	uint16_t	wMicroseconds;
	uint16_t	wNanoseconds;

	char	clock_leap_second_known;
	char	clock_failure;
	char	clock_not_synchronized;
	char	clock_precision;

}SDLSYSTEMTIME, *LPSDLSYSTEMTIME;
void ComputeTime(uint64_t llMicrosecond, LPSDLSYSTEMTIME lpst)
{
	if(llMicrosecond/1000000 > 36*365*24*60*60)//2036 year
	{
		llMicrosecond = 36*365*24*60*60;
		llMicrosecond *= 1000000;
	}

	struct tm tm1;
	tm1.tm_sec = 0;
	tm1.tm_min = 0;
	tm1.tm_hour = 0;
	tm1.tm_mday = 1;
	tm1.tm_mon = 0;
	tm1.tm_year = 100;
	tm1.tm_isdst = 0;
	time_t tt = mktime(&tm1);
	
	tt += llMicrosecond/1000000;	
#ifdef _WIN32
	localtime_s(&tm1, &tt);
#else
	localtime_r(&tt, &tm1);	//涉及到时区的.
#endif
	lpst->wYear = tm1.tm_year+1900;
	lpst->wMonth = tm1.tm_mon+1;
	lpst->wDay = tm1.tm_mday;
	lpst->wHour = tm1.tm_hour;
	lpst->wMinute = tm1.tm_min;
	lpst->wSecond = tm1.tm_sec;
	lpst->wMilliseconds = (uint16_t)((llMicrosecond%1000000)/1000);
	lpst->wMicroseconds = (uint16_t)((llMicrosecond%1000000)%1000);
	lpst->wNanoseconds = 0;
}

std::string get_exec_dir()
{
    char exeFullPath[MAX_PATH] = {};
#ifdef _WIN32
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::string strFullPath = (std::string)(exeFullPath);
	int nStart = strFullPath.find_last_of(TEXT("\\"));
	std::string strexepath = strFullPath.substr(0, nStart);
#else
	readlink("/proc/self/exe", exeFullPath, MAX_PATH);
	//getcwd(exeFullPath, MAX_PATH);
	std::string strexepath = dirname(exeFullPath);
	//std::string strexepath = std::string(exeFullPath);

	//strexepath = strexepath.substr(0, strexepath.find_last_not_of("/"));
#endif
    return strexepath;
}
bool create_dir_in_exec(std::string dir)
{
    std::string exepath = get_exec_dir();
#ifdef _WIN32
    std::string fullpath = exepath + "\\" + dir;
    return std::experimental::filesystem::create_directories(fullpath);
#else
    std::string command;
    command = "mkdir -p " + exepath+"/"+dir;  
    system(command.c_str());
    return true;
#endif
}
std::string create_filename_from_mirco(uint64_t llMicrosecond)
{
    SDLSYSTEMTIME st;
	ComputeTime(llMicrosecond, &st);
	char chTime[128] = {};
	sprintf(chTime, "%04u%02u%02u_%02u%02u%02u_%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	return chTime;
}

std::string create_time_from_mirco(uint64_t llMicrosecond)
{
	SDLSYSTEMTIME st;
	ComputeTime(llMicrosecond, &st);
	char chTime[128] = {};
	sprintf(chTime, "%04u-%02u-%02u %02u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	return chTime;
}
std::string create_comtradetime_from_mirco(uint64_t llMicosecond)
{
	SDLSYSTEMTIME st;
	ComputeTime(llMicosecond, &st);
	char chTime[128] = {};
	sprintf(chTime, "%02u/%02u/%04u,%02u:%02u:%02u.%06u", st.wDay, st.wMonth, 
            st.wYear, st.wHour, st.wMinute, st.wSecond, (uint32_t)st.wMilliseconds*1000+st.wMicroseconds);
	return chTime;
}
std::string create_time_from_format(uint64_t llMicoseond, std::string format)
{
	SDLSYSTEMTIME st;
	ComputeTime(llMicoseond, &st);
	char chTime[128] = {};
	sprintf(chTime, format.c_str(), 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return chTime;
}
uint64_t get_current_mirco(bool bUTC)
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = 
		time_point_now.time_since_epoch();

	uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();

	if (bUTC){
		return micros;
	}
	struct tm tm1;
	tm1.tm_sec = 0;
	tm1.tm_min = 0;
	tm1.tm_hour = 0;
	tm1.tm_mday = 1;
	tm1.tm_mon = 0;
	tm1.tm_year = 100;
	tm1.tm_isdst = 0;
	time_t tt1 = mktime(&tm1);//2000.1.1 0:0:0以前的秒计数

	return (micros - tt1*1000*1000);
}
uint64_t get_current_seconds(bool bUTC)
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = 
		time_point_now.time_since_epoch();

	uint64_t secs =  std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
	if (bUTC){
		return secs;
	}
	struct tm tm1;
	tm1.tm_sec = 0;
	tm1.tm_min = 0;
	tm1.tm_hour = 0;
	tm1.tm_mday = 1;
	tm1.tm_mon = 0;
	tm1.tm_year = 100;
	tm1.tm_isdst = 0;
	time_t tt1 = mktime(&tm1);//2000.1.1 0:0:0以前的秒计数
	 
	return (secs-tt1);
}
uint64_t get_current_mills()
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = 
		time_point_now.time_since_epoch();

	uint64_t mill =  std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();

	return mill;
}

bool sortMethod(FILEINFO file1,FILEINFO file2)
{
	#if 1
	// 降序排序
	return (std::get<0>(file1) > std::get<0>(file2));
	#else
	// 升序排序
	return (std::get<0>(file1) < std::get<0>(file2));
	#endif
}

void get_dir_files(const std::string& cate_dir, FILELIST& files)
{
	files.clear();
#ifdef _WIN32
	std::filesystem::path str(cate_dir);
	if (!std::filesystem::exists(str)) {
		return;
	}
	std::filesystem::directory_iterator filelist(str);
	for (auto& ita : filelist)
	{
		auto elapse = 
			std::chrono::duration_cast<std::chrono::seconds>(
				std::experimental::filesystem::file_time_type::clock::now().time_since_epoch()
				- std::chrono::system_clock::now().time_since_epoch()).count();
		auto systemTime = std::chrono::duration_cast<std::chrono::seconds>(ita.last_write_time().time_since_epoch()).count() - elapse;

		files.push_back({ ita.path().filename().string(), ita.file_size(), systemTime, (uint16_t)ita.is_directory()});
	}
#else  
    DIR *dir;  
    struct dirent *ptr;  
    char base[1000];
   
    if ((dir=opendir(cate_dir.c_str())) == NULL)  
	{  
		return;
	}  
   
    while ((ptr=readdir(dir)) != NULL)  
    {
        if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
        {
			// 当前目录或上级目录
			continue;
		}
		else if(ptr->d_type == 4)
        {
			// 目录文件 返回结果例:HISTORY
            files.push_back({ptr->d_name, 0, 0, 1});
        }
        else if(ptr->d_type == 8)
		{
			// 常规文件 返回结果例:SDL9200.log
			std::string fullfilePath = cate_dir;
			int strLength = fullfilePath.size();
			// 现假设cate_dir的路径形式为/HISTORY/SOE/
			std::string fileName = std::string(ptr->d_name);
			fullfilePath = fullfilePath + fileName;
			auto retf = get_file_info(fullfilePath);
			std::get<0>(retf).erase(0,strLength);
			files.push_back(retf);
		}
        else
		{
			// 其他类型文件,停止本次操作
			continue;
		}
    }
    closedir(dir);
#endif
	// 文件名排序
	std::sort(files.begin(),files.end(),sortMethod);
}

FILEINFO get_file_info(const std::string &fileName) 
{
    FILEINFO sf;
    struct stat info;
    static uint64_t ms_time = 1000;
    stat(fileName.c_str(), &info);

    return {fileName, info.st_size, (info.st_mtime) * ms_time, 0};
}

// 获取文件的大小
uint64_t get_file_size(const std::string& fileName)
{
	struct stat statBuff;
	stat(fileName.c_str(), &statBuff);
	return statBuff.st_size;
}

void get_file(const std::string& fileName, DTU::buffer& data)
{
    std::ifstream input;
	// 必须要用二进制打开文件,否则会出很多问题
	input.open(fileName, std::fstream::in | std::fstream::binary);
    data.remove();
    if (input.good())
    {
		std::stringstream buffer;
		buffer << input.rdbuf();

		data.append(buffer.str().c_str(), buffer.str().size());

		input.close();
    }
	else
	{
		DTULOG(DTU_ERROR, "get_file 无法打开文件%s", fileName.c_str());
		input.close();
	}
}

void get_file(const std::string& fileName, DTU::buffer& data, uint64_t offset, uint64_t size)
{
	static std::map<std::string, DTU::buffer> StaticReadFileStore;
	data.remove();
	// 查找是否有文件索引
	auto ita = StaticReadFileStore.find(fileName);
	if(ita != StaticReadFileStore.end()) {
		data.append(ita->second.const_data()+offset, size);
		if(ita->second.size() <= offset + size) {
			DTULOG(DTU_INFO, "文件[%s]发送完成,清空缓存区(多传输)...", fileName.c_str());
			StaticReadFileStore.erase(ita);
		}
	}
	else {
		DTU::buffer fileBuffer;
		get_file(fileName, fileBuffer);
		if(fileBuffer.size() == 0) {
			return;
		}
		StaticReadFileStore.insert({fileName, fileBuffer});
		data.append(fileBuffer.const_data()+offset, size);
		if(fileBuffer.size() <= offset + size) {
			DTULOG(DTU_INFO, "文件[%s]发送完成,清空缓存区(单传输)...", fileName.c_str());
			StaticReadFileStore.erase(fileName);
		}
    }
}

int save_file(const std::string& fileName, const DTU::buffer& content)
{
	std::string fullName = fileName;
	
    FILE* fp = fopen(fullName.c_str(),"wb");
    if (!fp)
    {
        DTULOG(DTU_ERROR, (char*)"save_file 无法保存文件:[%s],错误原因:[%s]", fullName.c_str(),strerror(errno));
        return 1;
    }
    auto ret = fwrite(content.const_data(), 1, content.size(), fp);
    fclose(fp);
    if (ret != content.size()){
        return 3;
    }
    return 0;
}

int save_file(const std::string& fileName, const DTU::buffer& data, bool transOK)
{
	int result = DTU_SUCCESS;
	static std::map<std::string, DTU::buffer> StaticSaveFileStore;
	auto ita = StaticSaveFileStore.find(fileName);
	if(ita != StaticSaveFileStore.end()) {
		ita->second.append(data);
	}
	else {
		StaticSaveFileStore.insert({fileName, data});
	}
	if(transOK) {
		// 传输完成
		result = save_file(fileName, StaticSaveFileStore[fileName]);
		DTULOG(DTU_INFO, "文件[%s]传输保存完成, 清空缓存区",fileName.c_str());
		StaticSaveFileStore.erase(fileName);
	}
	return result;
}

void read_fix_table(uint16_t paramid)
{
#ifndef _WIN32
    // std::cout << "+--------------------------------------------------------------------+" << std::endl;
    // std::cout << "|------------------------------- Fix Value Table --------------------|" << std::endl;
    // std::cout << "+--------------------------------------------------------------------+" << std::endl;
    // std::cout << "|    fix   |         Description         |    size    |    offset    |" << std::endl;
    // std::cout << "+--------------------------------------------------------------------+" << std::endl;
	// //SETTINGVALUETABLE attributeTable
	// auto& attributeTable = dtudataconfig::instance().get_setting_table();
	// if (paramid == 0)
	// {
	// 	for(auto& item : attributeTable)
	// 	{
	// 		for(auto& item1 : std::get<PARAM_TABLE>(item.second))
	// 		{
	// 			auto desc = std::get<VALUE_DESC>(item1.second);// 描述
	// 			auto fix = std::get<VALUE_FIX>(item1.second); // 点表
	// 			auto size = std::get<VALUE_SIZE>(item1.second);
	// 			auto offset = std::get<VALUE_OFFSET>(item1.second);
	// 			//char desc_utf8[32];
	// 			//GBKToUTF8((char*)desc.c_str(),desc.size(), desc_utf8, 128);
	// 			//if(fix == 0)continue;
	// 			printf("|          |                             |            |              |\r");
	// 			printf("|          |                             |            |   %u\r", offset);
	// 			printf("|          |                             |   %u\r", size);
	// 			printf("|  0x%04X  |   %s\n", fix, desc.c_str());
				
				
	// 			std::cout << "+--------------------------------------------------------------------+" << std::endl;

	// 		}
	// 	}
	// }
	// else{
	// 	auto ita = attributeTable.find(paramid);
	// 	if (ita != attributeTable.end())
	// 	{
	// 		for(auto& item1 : std::get<PARAM_TABLE>(ita->second))
	// 		{
	// 			auto desc = std::get<VALUE_DESC>(item1.second);// 描述
	// 			auto fix = std::get<VALUE_FIX>(item1.second); // 点表
	// 			auto size = std::get<VALUE_SIZE>(item1.second);
	// 			auto offset = std::get<VALUE_OFFSET>(item1.second);
	// 			//char desc_utf8[32];
	// 			//GBKToUTF8((char*)desc.c_str(),desc.size(), desc_utf8, 128);
	// 			//if(fix == 0)continue;
	// 			printf("|          |                             |            |              |\r");
	// 			printf("|          |                             |            |   %u\r", offset);
	// 			printf("|          |                             |   %u\r", size);
	// 			printf("|  0x%04X  |   %s\n", fix, desc.c_str());
				
				
	// 			std::cout << "+--------------------------------------------------------------------+" << std::endl;

	// 		}
	// 	}
	// }
#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////
const double TOLERANCE = 1E-6;
#define DTU_PI (3.14159265358979323846)
bool IsEqual(double A, double B)
{
	return fabs(A - B) < TOLERANCE;
}
std::vector<double> LeastSquare(int PointNumber, const double X[], const double Y[])
{
	// now, we use rank 1 polynomial only
	//assert(Rank == 1);

	double sumX = 0.0;
	double sumX2 = 0.0;
	double sumY = 0.0;
	double sumXY = 0.0;
	for (int i = 0; i < PointNumber; i++) {
		sumX += X[i];			// t2
		sumX2 += X[i] * X[i];	// t1
		sumY += Y[i];			// t4
		sumXY += X[i] * Y[i];	// t3
	}

	double denominator = PointNumber * sumX2 - sumX * sumX;
	if (IsEqual(denominator, 0))
		throw std::runtime_error("invalid values, denominator error");
	//printf("t1:%lf t2:%lf t3:%lf t4:%lf\n", sumX2, sumX, sumXY, sumY);
	std::vector<double> v(2);
	v[0] = (sumX2 * sumY - sumXY * sumX) / denominator;
	v[1] = (PointNumber * sumXY - sumX * sumY) / denominator;
	return v;
}
uint8_t set_bit(uint8_t data, int index, bool flag)
{
    if (index > 8 || index < 1)
        return data;
    int v = index < 2 ? index : (2  << (index - 2));
    return flag ? (uint8_t)(data | v) : (uint8_t)(data & ~v);
}
double Argument(const std::complex<double>& Value)
{
	if (std::abs(Value) >= TOLERANCE)
		return std::arg(Value);
	else
		return 0;
}

std::complex<double> Dft(int PointNumber, const double X[], int Rank)
{
	// Rank是有几个周波
	// assert(PointNumber > 0);//去除小于零提示

	//PointNumber = 600;
	//PointNumber = 200;
	// for m points, only the harmonic whose rank is less than m / 2 can be calculated
	std::complex<double> sum_point;

	std::vector<std::complex<double>> one_point;
	one_point.resize(PointNumber);


	double an = 0;
	double bn = 0;
	for (int i = 0; i < PointNumber; i++) 
	{
		an += X[i] * cos(2 * DTU_PI * i * Rank / PointNumber);
		bn += -X[i] * sin(2 * DTU_PI * i * Rank / PointNumber);
	}

	an *= 2.0 / PointNumber;
	bn *= 2.0 / PointNumber;
	return std::complex<double>(bn, an);
}
double NormalizeArgument(double Argument)
{
	while (Argument > DTU_PI)
		Argument -= 2 * DTU_PI;
	while (Argument <= -DTU_PI)
		Argument += 2 * DTU_PI;
	return Argument;
}
///////////////////////////////////////////////////////////////////////////////////////////////


uint32_t GBKToUTF8(char* chGBK, size_t lenGBK, char* chUTF8, size_t lenUTF8)
{
#ifndef _WIN32
	size_t lenUTF8_old = lenUTF8;

	memset(chUTF8, 0, lenUTF8);

	// 不能转换成gb18030,
	// iconv可选项 //TRANSLIT 遇到无法转换的转成相近的字符
	// iconv可选项 //IGNORE 遇到无法转换的就跳过
	iconv_t cd = iconv_open("UTF-8", "GBK//IGNORE");
	if (cd == (iconv_t)(-1))
		return 0;
	if (iconv(cd, &chGBK, &lenGBK, &chUTF8, &lenUTF8) == (size_t)(-1))
	{
		iconv_close(cd);
		return 0;
	}

	iconv_close(cd);
	return (uint32_t)(lenUTF8_old - lenUTF8);
#else 
	int len = MultiByteToWideChar(936, 0, chGBK, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(936, 0, chGBK, -1, wstr, len);

	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, chUTF8, len, NULL, NULL);

	delete[] wstr;
	return len;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// UTF8ToGBK()
uint32_t UTF8ToGBK(char* chUTF8, size_t lenUTF8, char* chGBK, size_t lenGBK)
{
#ifndef _WIN32
	size_t lenGBK_old = lenGBK;

	memset(chGBK, 0, lenGBK);
	iconv_t cd = iconv_open("GBK//IGNORE", "UTF-8");
	if (cd == (iconv_t)(-1))
		//printf("打开 GBK//IGNORE,UTF-8,失败\n");
		return 0;
	if (iconv(cd, &chUTF8, &lenUTF8, &chGBK, &lenGBK) == (size_t)(-1))
	{
		iconv_close(cd);
		return 0;
	}

	iconv_close(cd);
	return (uint32_t)(lenGBK_old - lenGBK);
#else
	int len = MultiByteToWideChar(CP_UTF8, 0, chUTF8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, chUTF8, -1, wstr, len);

	len = WideCharToMultiByte(936, 0, wstr, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(936, 0, wstr, -1, chGBK, len, NULL, NULL);

	delete[] wstr;
	return len;
#endif
}
uint8_t get_mod(uint8_t* data, uint32_t size)
{
	if (!data){
		return 0;
	}
	uint8_t chs = 0;
	for(uint32_t i=0;i<size;i++)
	{
		chs+=data[i];
	}
	return chs;
}

//计算CRC32校验
uint32_t crc32(uint8_t *buf, uint32_t len)
{
	uint32_t  crc_result=0;
	uint32_t  i;

	static uint32_t crc32_tab[] =
	{
		0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9,
		0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005,
		0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,
		0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD,
		0x4C11DB70,0x48D0C6C7,0x4593E01E,0x4152FDA9,
		0x5F15ADAC,0x5BD4B01B,0x569796C2,0x52568B75,
		0x6A1936C8,0x6ED82B7F,0x639B0DA6,0x675A1011,
		0x791D4014,0x7DDC5DA3,0x709F7B7A,0x745E66CD,
		0x9823B6E0,0x9CE2AB57,0x91A18D8E,0x95609039,
		0x8B27C03C,0x8FE6DD8B,0x82A5FB52,0x8664E6E5,
		0xBE2B5B58,0xBAEA46EF,0xB7A96036,0xB3687D81,
		0xAD2F2D84,0xA9EE3033,0xA4AD16EA,0xA06C0B5D,
		0xD4326D90,0xD0F37027,0xDDB056FE,0xD9714B49,
		0xC7361B4C,0xC3F706FB,0xCEB42022,0xCA753D95,
		0xF23A8028,0xF6FB9D9F,0xFBB8BB46,0xFF79A6F1,
		0xE13EF6F4,0xE5FFEB43,0xE8BCCD9A,0xEC7DD02D,
		0x34867077,0x30476DC0,0x3D044B19,0x39C556AE,
		0x278206AB,0x23431B1C,0x2E003DC5,0x2AC12072,
		0x128E9DCF,0x164F8078,0x1B0CA6A1,0x1FCDBB16,
		0x018AEB13,0x054BF6A4,0x0808D07D,0x0CC9CDCA,
		0x7897AB07,0x7C56B6B0,0x71159069,0x75D48DDE,
		0x6B93DDDB,0x6F52C06C,0x6211E6B5,0x66D0FB02,
		0x5E9F46BF,0x5A5E5B08,0x571D7DD1,0x53DC6066,
		0x4D9B3063,0x495A2DD4,0x44190B0D,0x40D816BA,
		0xACA5C697,0xA864DB20,0xA527FDF9,0xA1E6E04E,
		0xBFA1B04B,0xBB60ADFC,0xB6238B25,0xB2E29692,
		0x8AAD2B2F,0x8E6C3698,0x832F1041,0x87EE0DF6,
		0x99A95DF3,0x9D684044,0x902B669D,0x94EA7B2A,
		0xE0B41DE7,0xE4750050,0xE9362689,0xEDF73B3E,
		0xF3B06B3B,0xF771768C,0xFA325055,0xFEF34DE2,
		0xC6BCF05F,0xC27DEDE8,0xCF3ECB31,0xCBFFD686,
		0xD5B88683,0xD1799B34,0xDC3ABDED,0xD8FBA05A,
		0x690CE0EE,0x6DCDFD59,0x608EDB80,0x644FC637,
		0x7A089632,0x7EC98B85,0x738AAD5C,0x774BB0EB,
		0x4F040D56,0x4BC510E1,0x46863638,0x42472B8F,
		0x5C007B8A,0x58C1663D,0x558240E4,0x51435D53,
		0x251D3B9E,0x21DC2629,0x2C9F00F0,0x285E1D47,
		0x36194D42,0x32D850F5,0x3F9B762C,0x3B5A6B9B,
		0x0315D626,0x07D4CB91,0x0A97ED48,0x0E56F0FF,
		0x1011A0FA,0x14D0BD4D,0x19939B94,0x1D528623,
		0xF12F560E,0xF5EE4BB9,0xF8AD6D60,0xFC6C70D7,
		0xE22B20D2,0xE6EA3D65,0xEBA91BBC,0xEF68060B,
		0xD727BBB6,0xD3E6A601,0xDEA580D8,0xDA649D6F,
		0xC423CD6A,0xC0E2D0DD,0xCDA1F604,0xC960EBB3,
		0xBD3E8D7E,0xB9FF90C9,0xB4BCB610,0xB07DABA7,
		0xAE3AFBA2,0xAAFBE615,0xA7B8C0CC,0xA379DD7B,
		0x9B3660C6,0x9FF77D71,0x92B45BA8,0x9675461F,
		0x8832161A,0x8CF30BAD,0x81B02D74,0x857130C3,
		0x5D8A9099,0x594B8D2E,0x5408ABF7,0x50C9B640,
		0x4E8EE645,0x4A4FFBF2,0x470CDD2B,0x43CDC09C,
		0x7B827D21,0x7F436096,0x7200464F,0x76C15BF8,
		0x68860BFD,0x6C47164A,0x61043093,0x65C52D24,
		0x119B4BE9,0x155A565E,0x18197087,0x1CD86D30,
		0x029F3D35,0x065E2082,0x0B1D065B,0x0FDC1BEC,
		0x3793A651,0x3352BBE6,0x3E119D3F,0x3AD08088,
		0x2497D08D,0x2056CD3A,0x2D15EBE3,0x29D4F654,
		0xC5A92679,0xC1683BCE,0xCC2B1D17,0xC8EA00A0,
		0xD6AD50A5,0xD26C4D12,0xDF2F6BCB,0xDBEE767C,
		0xE3A1CBC1,0xE760D676,0xEA23F0AF,0xEEE2ED18,
		0xF0A5BD1D,0xF464A0AA,0xF9278673,0xFDE69BC4,
		0x89B8FD09,0x8D79E0BE,0x803AC667,0x84FBDBD0,
		0x9ABC8BD5,0x9E7D9662,0x933EB0BB,0x97FFAD0C,
		0xAFB010B1,0xAB710D06,0xA6322BDF,0xA2F33668,
		0xBCB4666D,0xB8757BDA,0xB5365D03,0xB1F740B4
	};

	while(len--)
	{
		crc_result = ((crc_result<<8) | *buf++) ^ crc32_tab[(crc_result>>24) & 0xFF];
	}
	for(i=0;i<4;i++)
	{
		crc_result = ((crc_result<<8))^ crc32_tab[(crc_result>>24) & 0xFF];
	}
	return crc_result;
}

// CRC16校验码
uint16_t crc16(uint8_t *data, uint16_t len)
{
    uint8_t ucCRCHi = 0xFF;
    uint8_t ucCRCLo = 0xFF;
    int iIndex;
    static const uint8_t aucCRCHi[] =
    {   0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40
    };
    static const uint8_t aucCRCLo[] =
    {   0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40
    };
 
    while(len--)
    {
        iIndex = ucCRCLo ^ *(data++);
        ucCRCLo = (uint8_t)(ucCRCHi ^ aucCRCHi[iIndex]);
        ucCRCHi = aucCRCLo[iIndex];
    }
    //DEBUG_INFO("AI_tempsend = 0x%x", (uint16_t)(ucCRCLo << 8 | ucCRCHi));
    return (uint16_t)(ucCRCLo << 8 | ucCRCHi);
}

std::vector<std::string> split_str(const std::string input, const std::string& regex)
{
	std::regex patten(regex);
	std::sregex_token_iterator first{ input.begin(),input.end(), patten, -1 }, last;
	return { first, last };
}

void transMacStr(const std::string str,const std::string& cspi,uint8_t result[])
{
    std::vector<std::string> ret = split_str(str,cspi);
    int i = 0;
    for(auto &item : ret)
    {
        if( i >= ret.size() )
            return;
        result[i] = (uint8_t)strtol(item.c_str(), NULL, 16);
        i++;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
int64_t GetSystemTime()
{
    int64_t time = 0;
#ifndef _WIN32
    timeval tv;
    gettimeofday(&tv, 0);
    time = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
#else
    FILETIME ft;
    LARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    // 从1970年1月1日0:0:0:000到现在的微秒数(UTC时间)
    time = (li.QuadPart - EPOCHFILETIME) / 10;
#endif
    return time;
}

#ifndef _WIN32
std::string ExecCmd(std::string cmd)
{
    std::string ret;
    if(cmd.empty())
    {
        return ret;
    }
    FILE *fp = nullptr;
    fp = popen(cmd.c_str(), "r");
    if(fp == nullptr)
    {
        return ret;
    }
    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        ret = ret + buf;
    }
    pclose(fp);
    return ret;
}


Disk_info GetDiskUsage()
{
	Disk_info info;
	try
	{
		std::string cmd = "df -h | grep /dev/mmcblk0p3 | awk '{print $5}'";
		auto ret = ExecCmd(cmd);
		ret.erase(ret.find("%"));
		info._used = atoi(ret.c_str());
		
		cmd = "df -h | grep /dev/mmcblk0p3 | awk '{print $2}'";
		info._size_s = ExecCmd(cmd);
		info._size_s.erase(info._size_s.length() - 1);

		cmd = "df -h | grep /dev/mmcblk0p3 | awk '{print $3}'";
		info._used_s = ExecCmd(cmd);
		info._used_s.erase(info._used_s.length() - 1);

		cmd = "df -h | grep /dev/mmcblk0p3 | awk '{print $4}'";
		info._available_s = ExecCmd(cmd);
		info._available_s.erase(info._available_s.length() - 1);
		return std::move(info);
	}
	catch(const std::exception& e)
	{
		DTULOG(DTU_ERROR, (char *)"GetDiskUsage() 发生错误",e.what());
	}
}

DTU::buffer GetSysconfig(uint16_t cmd)
{
	DTU::buffer result;

	std::string path = get_exec_dir();

	switch(cmd)
	{
		case DTU_GET_SYS_CONFIG : path = path + "/config/syscfg.json";break;
		case DTU_GET_PROTO_CONFIG : path = path + "/config/csprotocol.json";break;
	}

	get_file(path,result);

	return result;
}

#endif

// 是否包含子串
static bool isincStr(const std::string& str1, const std::string& str2)
{
	return (strstr(str1.c_str(), str2.c_str()) != nullptr);
}

FILELIST get_file_list(const std::string& dir)
{
    DTULOG(DTU_INFO, (char *)"规约请求目录:%s", dir.c_str());

    // 规约支持的目录
    static std::vector<std::string> vecdir = {
        "/", "/COMTRADE/", "/HISTORY/", "/HISTORY/SOE/", "/HISTORY/CO/", "/HISTORY/EXV/",
         "/HISTORY/FIXPT/", "/HISTORY/ULOG/", "/HISTORY/FLOWREV/", "/HISTORY/FRZ/"};

	std::string truePath;

	if ((dir.find("COMTRADE") != std::string::npos)) {
		truePath = "/COMTRADE/";
	} else if ((dir.find("HISTORY/SOE") != std::string::npos) || (dir.find("SOE") != std::string::npos)) {
		truePath = "/HISTORY/SOE/";
	} else if ((dir.find("HISTORY/CO") != std::string::npos) || (dir.find("CO") != std::string::npos)) {
		truePath = "/HISTORY/CO/";
	} else if ((dir.find("HISTORY/EXV") != std::string::npos) || (dir.find("EXV") != std::string::npos)) {
		truePath = "/HISTORY/EXV/";
	} else if ((dir.find("HISTORY/FIXPT") != std::string::npos) || (dir.find("FIXPT") != std::string::npos)) {
		truePath = "/HISTORY/FIXPT/";
	} else if ((dir.find("HISTORY/ULOG") != std::string::npos) || (dir.find("ULOG") != std::string::npos)) {
		truePath = "/HISTORY/ULOG/";
	} else if ((dir.find("HISTORY/FLOWREV") != std::string::npos) || (dir.find("FLOWREV") != std::string::npos)) {
		truePath = "/HISTORY/FLOWREV/";
	} else if ((dir.find("HISTORY/FRZ") != std::string::npos) || (dir.find("FRZ") != std::string::npos)) {
		truePath = "/HISTORY/FRZ/";
	} else if ((dir.find("HISTORY") != std::string::npos)) {
		truePath = "/HISTORY/";
	} else {
		truePath = "/";
	}

    // 文件/文件夹名 大小 时间戳  1:目录 0:文件
    FILELIST resultlist;
    auto ita = std::find(vecdir.begin(), vecdir.end(), truePath);
    if (ita == vecdir.end())
    {
        DTULOG(DTU_ERROR,(char*)"不支持读取目录:%s", truePath.c_str());
        return resultlist;
    }
	else {
		DTULOG(DTU_INFO, (char*)"真实目录:%s", truePath.c_str());
	}

    // 判断是否为读取根目录 根目录要特殊处理
    if(truePath == "/")
    {
        resultlist.push_back({"HISTORY",0,0,1});
        resultlist.push_back({"COMTRADE",0,0,1});
    }
    else
    {
        // 不为根目录
        std::string fullPath = get_exec_dir() + truePath;
        get_dir_files(fullPath, resultlist);
    }

    //使"HISTORY"格式变为"/HISTORY/"格式
    for(auto &item:resultlist)
    {
        auto oristr = std::get<0>(item);
        if(std::get<3>(item) == 0)
        {
            // 如果是文件则跳过
            continue;
        }
        std::get<0>(item) = "/" + oristr + "/";
    }

    return resultlist;
}

//点分十进制转无符号整型
uint32_t IPToInt(const std::string &strIP)
{
	int a[4];
	std::string ip = strIP;
	for (int i = 0; i < 4; ++i)
	{
		auto pos = ip.find('.');
		a[i] = stoi(ip.substr(0, pos));
		ip = ip.substr(pos+1);
	}
	uint32_t ans = (a[3] << 24) + (a[2] << 16) + (a[1] << 8) + a[0];
	return ans;
}

//无符号整型转点分十进制
std::string IntToIP(const uint32_t &value)
{
	std::vector<std::string> vec(4);
	// 16进制以“0x”开头
	vec[3] = std::to_string((value & 0xff000000)>>24);
	vec[2] = std::to_string((value & 0x00ff0000)>>16);
	vec[1] = std::to_string((value & 0x0000ff00)>>8);
	vec[0] = std::to_string(value & 0x000000ff);
	return vec[0] + "." + vec[1] + "." + vec[2] + "." + vec[3];
}

float strtofloat(std::string str)
{
	float retfloat = 0.0;
    sscanf(str.c_str(),"%f",&retfloat);
	return retfloat;
}