
#include "dtunetconfig.h"
#include <dtulog.h>

#include <json/json.h>
#include <dtucommon.h>
#include <dtucmdcode.h>
#include <fstream>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace DTUCFG;
using namespace DTU;
using namespace std;

void netconfigure::load(const std::string& file)
{
    DTULOG(DTU_INFO, (char*)"加载网卡配置参数:%s...",file.c_str());
    std::string newfile = file;
#ifdef _WIN32
    std::string filename("\\");
#else
    std::string filename("/");
#endif    
    newfile.erase(newfile.find_last_of(filename)+1);
    newfile = newfile + std::string("netcfg.json");

    DTU_USER();

    std::ifstream ifs;
	ifs.open(newfile);
	
    Json::Value info_root;
    Json::CharReaderBuilder readerbuilder;
    JSONCPP_STRING errs;

    if (!Json::parseFromStream(readerbuilder, ifs, &info_root, &errs)){
        DTU_THROW((char*)"加载网络配置文件错误 %s", errs.c_str());
    }

    ifs.close();

    NetParam.clear();
    _NETnum = 0;
    /*************************** 读取网络配置 ***************************/
    auto Netinfo = info_root["NET"];

    _net_info = Netinfo;

    for(auto item : Netinfo)
    {
        NetParam.push_back( make_tuple( item["USE"].asInt(), item["IP"].asString(), item["NAME"].asString(), 
                    item["NETMASK"].asString(), item["GATEWAY"].asString()) );
        _NETnum++;
    }
    /*******************************************************************/
}

int netconfigure::save_net(DTU::buffer context)
{
    std::string fullpath = get_exec_dir() + "/config/netcfg.json";
    save_file(fullpath,context);
    // 重新加载参数
    load(fullpath);
    // 执行DTUnetconfig.sh脚本
    std::string shPath = get_exec_dir()+"/config/DTUnetconfig.sh";
    std::string netcmd = "chmod +x "+shPath;
    DTULOG(DTU_INFO, (char*)"正在设置网络参数...");
    system(netcmd.c_str());

    system(shPath.c_str());//执行脚本文件

    return DTU_SUCCESS;
}

//读取配置
DTU::buffer netconfigure::get_net_param()
{
    try
    {
        DTU::buffer result;
        if(NetParam.size() <= 0)
        {
            DTULOG(DTU_ERROR, (char*)"获取网络配置错误");
            return result;
        }

        for(auto & item : NetParam)
        {
            uint32_t Ins = 0; 
            Ins = IPToInt(get<1>(item));
            result.append((char*)&Ins,sizeof(uint32_t));
            Ins = IPToInt(get<3>(item));
            result.append((char*)&Ins,sizeof(uint32_t));
            Ins = IPToInt(get<4>(item));
            result.append((char*)&Ins,sizeof(uint32_t));
        }
        return result;
    }
    catch (std::exception &e)
    {
        DTULOG(DTU_ERROR, (char*)"get_net_param() 错误%s",e.what());
        return DTU::buffer();
    }
}

//设置网络参数 从proto写入到文件中
int netconfigure::set_net_param(const DTU::buffer& data, int net_use)//完整路径
{
    DTULOG(DTU_INFO, (char*)"设置网络参数,网卡数量[%d]...", _NETnum);

    // 清空原网卡数据准备写入
    NetParam.clear();
    uint16_t    _USE = 0;       //是否使用
    std::string CH;             //是否注释(跟随_USE)
    std::string _IP = "";       //IP地址
    std::string _NETMASK = "";  //子网掩码
    std::string _GATEWAY = "";  //网关
    
    //修改sh脚本的内容
    std::string fullPath = get_exec_dir() + "/config/DTUnetconfig.sh";
    FILE* fp = fopen(fullPath.c_str(),(char*)"w+");
    if(!fp)
    {
        DTULOG(DTU_ERROR, (char*)"无法打开网络配置脚本DTUnetconfig.sh");
        return DTU_SERIAL_ERROR;
    }

    //发过来的参数,修改netcfg.json文件
    uint32_t pos = 0;

    for(auto &item : _net_info)
    {
        // 设置IP
        _IP = IntToIP(data.get(pos,sizeof(uint32_t)).value<uint32_t>()); pos = pos+4;
        item["IP"] = _IP;
        
        // 设置子网掩码
        _NETMASK = IntToIP(data.get(pos,sizeof(uint32_t)).value<uint32_t>()); pos = pos+4;
        item["NETMASK"] = _NETMASK;
        
        // 设置网关
        _GATEWAY = IntToIP(data.get(pos,sizeof(uint32_t)).value<uint32_t>()); pos = pos+4;
        item["GATEWAY"] = _GATEWAY;

        //保存到DTUnetconfig.sh中
        if(item["USE"].asBool())
            CH = "";
        else
            CH = "# ";

        // fprintf(fp,(char*)"%sifconfig %s %s netmask %s\n",CH.c_str(), item["NAME"].asString().c_str(), _IP.c_str(), _NETMASK.c_str());
        // fprintf(fp,(char*)"#%sroute add default gw %s\n\n",CH.c_str(), _GATEWAY.c_str());

        fprintf(fp,(char*)"%sifconfig %s %s up\n", CH.c_str(), item["NAME"].asString().c_str(), _IP.c_str());

        //保存网卡参数
        NetParam.push_back( make_tuple(item["USE"].asInt(),_IP,item["NAME"].asString(),_NETMASK,_GATEWAY) );
    }
    
    fclose(fp); //关闭DTUnetconfig.sh脚本文件

    //写入到netcfg.json文件中
    Json::Value root;
    root["NET"] = _net_info;

    Json::StreamWriterBuilder WriterBuilder;
	std::ostringstream os;
	std::unique_ptr<Json::StreamWriter> JsonWriter(WriterBuilder.newStreamWriter());
	JsonWriter->write(root, &os);
	std::string str = os.str();

    DTU::buffer result(str.c_str(),str.size());

    save_net(result);

    return DTU_SUCCESS;
}

DTU::buffer netconfigure::get_net_param_from_file()
{
    DTU::buffer result;
    std::string fullpath = get_exec_dir() + "/config/netcfg.json";
    get_file(fullpath,result);
    return result;
}