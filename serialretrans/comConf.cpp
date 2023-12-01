#include "serial_transfer_impl.h"
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <map>
using namespace std;

struct ComInfo
{
    string comName; // 串口名称
    int comNo;      // 串口中心编号
    int nSpeed;     // 波特率
    int nBits;      // 数据位
    char nEvent;    // 校验位
    int nStop;      // 停止位
    int addrBytes;  // 地址域字节数
    int fd;
};

string clearHeadTailSpace(string str)
{
    if (str.empty())
    {
        return str;
    }
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

vector<string> splitStr(const string &str, char tag)
{
    vector<string> strList;
    string subStr = "";
    for (size_t i = 0; i < str.length(); i++)
    {
        if (tag == str[i])
        {
            if (!subStr.empty())
            {
                strList.push_back(subStr);
                subStr.clear();
            }
        }
        else
        {
            subStr.push_back(str[i]);
        }
    }
    if (!subStr.empty())
    {
        strList.push_back(subStr);
    }
    return strList;
}

void ReadWriteComData(SerialTransferImpl serial, int comNo)
{
    cout << "enter to ReadWriteComData" << endl;
    while (1)
    {
        serial.readAddHeader(comNo);
        usleep(2000);
    }
}

void ReadWriteCom3Data(SerialTransferImpl serial)
{
    cout << "enter to ReadWriteCom3Data" << endl;
    while (1)
    {
        serial.readDelHeader();
        usleep(2000);
    }
}

int main()
{
    ComInfo com1;
    ComInfo com2;
    ComInfo com3;

    map<int, int> comMap;

    ///////////////////从com_config_file里读出串口名称和中心编号////////////////////////
    ifstream ifs;
    ifs.open("com_config_file", ios::in);
    if (!ifs.is_open())
    {
        cout << "读取串口配置文件失败" << endl;
        return 0;
    }
    string buf;
    int i = 1, j = 1;
    while (getline(ifs, buf))
    {
        vector<string> strList = splitStr(buf, ',');
        cout << strList.size();
        if (strList.size() == 7)
        {
            if (i == 1)
            {
                com1.comName = strList[0];
                com1.comNo = atoi(strList[1].c_str());
                com1.nSpeed = atoi(strList[2].c_str());
                com1.nBits = atoi(strList[3].c_str());
                string tmpStr = clearHeadTailSpace(strList[4]);
                if (tmpStr.length() != 3)
                {
                    cout << "串口配置文件格式错误，请检查后重试!" << endl;
                    return 0;
                }
                com1.nEvent = tmpStr[1]; // 取出'N'中的N
                com1.nStop = atoi(strList[5].c_str());
                com1.addrBytes = atoi(strList[6].c_str());
                i++;
            }
            else if (i == 2)
            {
                com2.comName = strList[0];
                com2.comNo = atoi(strList[1].c_str());
                com2.nSpeed = atoi(strList[2].c_str());
                com2.nBits = atoi(strList[3].c_str());
                string tmpStr = clearHeadTailSpace(strList[4]);
                if (tmpStr.length() != 3)
                {
                    cout << "串口配置文件格式错误，请检查后重试!!" << endl;
                    return 0;
                }
                com2.nEvent = tmpStr[1]; // 取出'N'中的N
                com2.nStop = atoi(strList[5].c_str());
                com2.addrBytes = atoi(strList[6].c_str());
                i++;
            }
            else if (i == 3)
            {
                com3.comName = strList[0];
                if (strList[1] != "0")
                {
                    cout << "串口配置文件格式错误，请检查后重试!!!" << endl;
                    return 0;
                }
                com3.comNo = 0;
                com3.nSpeed = atoi(strList[2].c_str());
                com3.nBits = atoi(strList[3].c_str());
                string tmpStr = clearHeadTailSpace(strList[4]);
                if (tmpStr.length() != 3)
                {
                    cout << "串口配置文件格式错误，请检查后重试." << endl;
                    return 0;
                }
                com3.nEvent = tmpStr[1]; // 取出'N'中的N
                com3.nStop = atoi(strList[5].c_str());
                com3.addrBytes = atoi(strList[6].c_str());
                i++;
            }
        }
        else
        {
            cout << "串口配置文件格式错误，请检查后重试," << endl;
            return 0;
        }
    }
    cout << "串口1名称：" << com1.comName << " 编号：" << com1.comNo << " 波特率:" << com1.nSpeed << " 数据位:" << com1.nBits << " 校验位:" << com1.nEvent << " 停止位:" << com1.nStop << "地址域字节数：" << com1.addrBytes << endl;
    cout << "串口2名称：" << com2.comName << " 编号：" << com2.comNo << " 波特率:" << com2.nSpeed << " 数据位:" << com2.nBits << " 校验位:" << com2.nEvent << " 停止位:" << com2.nStop << "地址域字节数：" << com2.addrBytes << endl;
    cout << "串口3名称：" << com3.comName << " 波特率:" << com3.nSpeed << " 数据位:" << com3.nBits << " 校验位:" << com3.nEvent << " 停止位:" << com3.nStop << "地址域字节数：" << com3.addrBytes << endl;

    ifs.close();
    //////////////////////////////////////////////////////////////////////////////////////////

    // 操作串口

    SerialTransferImpl serial3;
    serial3.setAddrBytes(com3.addrBytes);
    if (serial3.uart_init(com3.comName, com3.nSpeed, com3.nBits, com3.nEvent, com3.nStop, -1) != 0)
    {
        return 0;
    }

    SerialTransferImpl serial1;
    serial1.setAddrBytes(com1.addrBytes);
    if (serial1.uart_init(com1.comName, com1.nSpeed, com1.nBits, com1.nEvent, com1.nStop, serial3.getFd()) != 0)
    {
        return 0;
    }

    SerialTransferImpl serial2;
    serial2.setAddrBytes(com2.addrBytes);
    if (serial2.uart_init(com2.comName, com2.nSpeed, com2.nBits, com2.nEvent, com2.nStop, serial3.getFd()) != 0)
    {
        return 0;
    }

    com1.fd = serial1.getFd();
    com2.fd = serial2.getFd();
    com3.fd = serial3.getFd();

    comMap[com1.comNo] = com1.fd;
    comMap[com2.comNo] = com2.fd;
    comMap[com3.comNo] = com3.fd;
    cout << "comMap 1 key:" << com1.comNo << " fd:" << com1.fd << endl;
    cout << "comMap 2 key:" << com2.comNo << " fd:" << com2.fd << endl;
    cout << "comMap 3 key:" << com3.comNo << " fd:" << com3.fd << endl;

    serial3.setComMap(comMap);

    if (com1.fd == -1)
    {
        cout << "串口1未打开" << endl;
        return 0;
    }
    if (com2.fd == -1)
    {
        cout << "串口2未打开" << endl;
        return 0;
    }
    if (com3.fd == -1)
    {
        cout << "串口3未打开" << endl;
        return 0;
    }
    cout << "线程1启动" << endl;
    thread t1(ReadWriteComData, serial1, com1.comNo);

    cout << "线程2启动" << endl;
    thread t2(ReadWriteComData, serial2, com2.comNo);

    cout << "线程3启动" << endl;
    thread t3(ReadWriteCom3Data, serial3);
    t3.join();

    serial1.uart_deinit(com1.fd);
    serial2.uart_deinit(com2.fd);
    serial3.uart_deinit(com3.fd);

    return 0;
}
