#include "serial_transfer_impl.h"
#include <iostream>
#include <mutex>
#include <ctime>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

std::mutex mtx;

SerialTransferImpl::SerialTransferImpl()
{
    uart_fd = -1;
    dest_uart_fd = -1;
}

SerialTransferImpl::~SerialTransferImpl()
{
}

int SerialTransferImpl::set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch (nBits)
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch (nEvent)
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    switch (nSpeed)
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if (nStop == 1)
        newtio.c_cflag &= ~CSTOPB;
    else if (nStop == 2)
        newtio.c_cflag |= CSTOPB;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        perror("com set error");
        return -1;
    }
    return 0;
}

int SerialTransferImpl::uart_init(string &comName, int nSpeed, int nBits, char nEvent, int nStop, int dest_fd)
{
    int fd;

    fd = open(comName.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1)
    {
        printf("open %s fail\n", comName.c_str());
        return -1;
    }
    else
    {
        printf("open %s sucsses fd = %d\n", comName.c_str(), fd);
    }

    set_opt(fd, nSpeed, nBits, nEvent, nStop);

    uart_fd = fd;
    dest_uart_fd = dest_fd;
    return 0;
}

void SerialTransferImpl::uart_deinit(int uart_fd)
{
    if (uart_fd > 0)
        close(uart_fd);

    uart_fd = -1;
}

int SerialTransferImpl::getFd()
{
    return uart_fd;
}

bool SerialTransferImpl::readAddHeader(int comNo)
{
    int bytes_read = 0;
    int bytes_write = 0;
    int totalBytes = 4;
    char read_buff[1024] = {0};
    int tmpBytesRead = 0;

    // 读串口1,加头尾发给串口3
    bytes_read = read(uart_fd, read_buff, 1024);
    tmpBytesRead += bytes_read;
    if (bytes_read > 0)
    {
        printf("read %d Bytes\n", bytes_read);
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%d:%02x ", comNo, read_buff[i]);
        }
        cout << endl;
        if (read_buff[0] == 0x10)
        { // 固定长度帧，10开头
            if (comAddrBytes == 2)
            {
                totalBytes = 6;
            }
            else
            {
                totalBytes = 5;
            }
        }
        else
        { // 可变长度帧，68开头
            if (bytes_read >= 3)
            {
                int16_t dataLength = (read_buff[1] > read_buff[2]) ? read_buff[2] : read_buff[1];
                // totalBytes = 9 + dataLength;
                totalBytes = 6 + dataLength;
            }
        }

        time_t beginTime, curTime;
        time(&beginTime);
        while (tmpBytesRead < totalBytes)
        {
            usleep(2000);
            time(&curTime);
            double diffSeconds = curTime - beginTime;
            if (diffSeconds >= 5)
            {
                cout << comNo << ":"
                     << "5秒超时时间到" << endl;
                return false;
            }
            bytes_read = read(uart_fd, &read_buff[tmpBytesRead], 1024);
            if (bytes_read > 0)
            {
                if (read_buff[0] != 0x10 && tmpBytesRead < 3 && tmpBytesRead + bytes_read >= 3)
                {
                    int16_t dataLength = (read_buff[1] > read_buff[2]) ? read_buff[2] : read_buff[1];
                    totalBytes = 9 + dataLength;
                }

                printf("read %d Bytes\n", bytes_read);
                for (int i = 0; i < bytes_read; i++)
                {
                    printf("%d:%02x ", comNo, read_buff[tmpBytesRead + i]);
                }
                cout << endl;
                tmpBytesRead += bytes_read;
            }
        }

        // 加头尾
        int write_buff_size = 6 + totalBytes;
        uint8_t *write_buff = new uint8_t(write_buff_size);
        write_buff[0] = 0xAA;
        write_buff[1] = tmpBytesRead & 0xff;
        write_buff[2] = tmpBytesRead >> 8 & 0xff;
        write_buff[3] = comNo;
        for (int i = 0; i < tmpBytesRead; i++)
        {
            write_buff[4 + i] = read_buff[i];
        }
        write_buff[1 + 2 + 1 + tmpBytesRead] = 0; // 将校验位先设成0
        for (int i = 0; i < tmpBytesRead + 3; i++)
        {
            write_buff[1 + 2 + 1 + tmpBytesRead] += write_buff[i + 1]; // 校验取校验前所有字节的累加和
        }
        write_buff[1 + 2 + 1 + tmpBytesRead + 1] = 0x55;

        cout << comNo << ":"
             << "读取到的数据已处理完成，即将写入目的串口" << endl;
        for (int i = 0; i < write_buff_size; i++)
        {
            printf("%02x ", write_buff[i]);
        }
        cout << endl;
        if (dest_uart_fd != -1)
        {
            if (!writeToOtherCom(dest_uart_fd, (char *)write_buff, write_buff_size))
            {
                delete[] write_buff;
                cout << comNo << ":"
                     << "目的串口写失败" << endl;
                return false;
            }
        }
        else
        {
            delete[] write_buff;
            cout << comNo << ":"
                 << "目的串口未打开" << endl;
            return false;
        }
        delete[] write_buff;
    }
    return true;
}

bool SerialTransferImpl::writeToOtherCom(int dest_fd, char *buff, int buf_size)
{
    if (dest_fd == -1)
    {
        cout << "目的串口未打开" << endl;
        return false;
    }
    mtx.lock();
    int bytes_write = 0;
    bytes_write = write(dest_fd, buff, buf_size);
    mtx.unlock();
    printf("write %d Bytes\n\n", bytes_write);
    return true;
}

bool SerialTransferImpl::readDelHeader()
{
    int bytes_read = 0;
    int bytes_write = 0;
    int totalBytes = 4;
    char read_buff[1024] = {0};
    int tmpBytesRead = 0;
    // 读串口3,去头尾发给对应串口
    bytes_read = read(uart_fd, read_buff, 1024);
    tmpBytesRead += bytes_read;
    if (bytes_read > 0)
    {
        printf("read %d Bytes\n", bytes_read);
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%02x ", read_buff[i]);
        }
        cout << endl;
        if (bytes_read >= 3)
        {
            int16_t dataLength = read_buff[2] << 8; // 高八位
            dataLength |= read_buff[1];             // 低八位
            cout << "数据长度是" << dataLength << endl;
            totalBytes = 1 + 2 + 1 + dataLength + 1 + 1;
        }
        time_t beginTime, curTime;
        time(&beginTime);
        while (tmpBytesRead < totalBytes)
        {
            usleep(2000);
            time(&curTime);
            double diffSeconds = curTime - beginTime;
            if (diffSeconds >= 5)
            {
                cout << "5秒超时时间到" << endl;
                return false;
            }
            bytes_read = read(uart_fd, &read_buff[tmpBytesRead], 1024);
            if (bytes_read > 0)
            {
                if (tmpBytesRead < 3 && tmpBytesRead + bytes_read >= 3)
                {
                    int16_t dataLength = read_buff[2] << 8; // 高八位
                    dataLength |= read_buff[1];             // 低八位
                    totalBytes = 1 + 2 + 1 + dataLength + 1 + 1;
                }
                printf("read %d Bytes\n", bytes_read);
                for (int i = 0; i < bytes_read; i++)
                {
                    printf("%02x ", read_buff[tmpBytesRead + i]);
                }
                cout << endl;
                tmpBytesRead += bytes_read;
            }
        }

        // 去头尾
        int write_buff_size = totalBytes - 1 - 2 - 1 - 1 - 1;
        uint8_t *write_buff = new uint8_t(write_buff_size);
        for (int i = 0; i < write_buff_size; i++)
        {
            write_buff[i] = read_buff[4 + i];
        }

        // 解出中心编号
        int curComNo = read_buff[3];
        cout << "发送到中心编号为" << curComNo << "的串口" << endl;
        int dest_fd = -1;
        if (comMap.find(curComNo) != comMap.end())
        {
            dest_fd = comMap[curComNo];
            cout << "找到对应的串口fd为" << dest_fd << endl;
        }

        cout << "读取到的数据已处理完成，即将写入目的串口" << endl;
        for (int i = 0; i < write_buff_size; i++)
        {
            printf("%02x ", write_buff[i]);
        }
        cout << endl;
        if (dest_fd != -1)
        {
            if (!writeToOtherCom(dest_fd, (char *)write_buff, write_buff_size))
            {
                delete[] write_buff;
                cout << "目的串口写失败" << endl;
                return false;
            }
        }
        else
        {
            delete[] write_buff;
            cout << "目的串口未打开" << endl;
            return false;
        }
        delete[] write_buff;
    }
    return true;
}

void SerialTransferImpl::setComMap(map<int, int> &comInfoMap)
{
    comMap = comInfoMap;
}

void SerialTransferImpl::setAddrBytes(int addrBytes)
{
    cout << "setAddrBytes" << addrBytes << endl;
    comAddrBytes = addrBytes;
}
