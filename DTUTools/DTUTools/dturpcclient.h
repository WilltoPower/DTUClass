/*
1，负责更新链接状态
2，重新链接后订阅频道
*/
#pragma once
#include <QObject>
#include <string>
#include <rest_rpc/rest_rpc.hpp>
class dturpcclient : public QObject
{
	Q_OBJECT

private:
	dturpcclient() {};
public:
	static dturpcclient& instance() {
		static dturpcclient client;
		return client;
	}
	void init(const std::string& ip, uint16_t port, bool reinit = false);
	void setInformationID(uint16_t infoID,bool isrun = true);
	void run();
signals:
	void update_status(bool);
signals:
	void update_report(unsigned short, QByteArray);
signals:
	void update_information(unsigned short, QByteArray);
signals:
	void update_CSLOG(QByteArray);
signals:
	void update_CSMSG(QByteArray);
signals:
	void update_GOOSE(QByteArray);
public slots:
	void change_net_param();
private:
	rest_rpc::rpc_client _client;
	std::string _ip;
	uint16_t _port;
	std::atomic<uint16_t> thisInfoID;
	std::atomic_bool runInfomation;
};

