#pragma once

#include <QObject>
#include <map>
#include <dtuprotocol.h>
class dtutask_notify : public QObject
{
	Q_OBJECT

public:
	static dtutask_notify& instance() {
		static dtutask_notify notify;
		return notify;
	}

	void addMessage(uint16_t cmd, QString callback, QWidget* pWidget);

	void notify(const DTU::dtuprotocol& proto);

	~dtutask_notify();
private:
	dtutask_notify();

private:
	std::map<uint16_t, std::tuple<QString, QWidget*>> _messageMap;
};
