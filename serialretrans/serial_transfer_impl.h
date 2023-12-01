#include <thread>
#include <string>
#include <map>
using namespace std;
using std::string;

class SerialTransferImpl
{
public:
   SerialTransferImpl();
   virtual ~SerialTransferImpl();

   int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
   int uart_init(string &comName, int nSpeed, int nBits, char nEvent, int nStop, int dest_uart_fd);
   void uart_deinit(int uart_fd);
   bool readAddHeader(int comNo); // 读过来的数据增加头尾
   bool readDelHeader();          // 读过来的数据去掉头尾
   bool writeToOtherCom(int dest_fd, char *buff, int buf_size);
   int getFd();
   void setComMap(map<int, int> &comInfoMap);
   void setAddrBytes(int addrBytes);

public:
   int uart_fd;
   int dest_uart_fd; // 数据加头尾后写入的串口fd
   int comAddrBytes; // 地址域字节数
   map<int, int> comMap;
};
