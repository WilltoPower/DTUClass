#include "dtutask_notify.h"
#include <dtulog.h>
dtutask_notify::dtutask_notify()
{
}



dtutask_notify::~dtutask_notify()
{
}

void dtutask_notify::addMessage(uint16_t cmd, QString callback, QWidget* pWidget)
{
	_messageMap.insert(std::pair<uint16_t, std::tuple<QString, QWidget*>>(cmd, { callback, pWidget }));
}

void dtutask_notify::notify(const DTU::dtuprotocol& proto)
{
	auto ita = _messageMap.find(proto._cmd);
	if (ita != _messageMap.end())
	{
		QWidget* pWidget = std::get<1>(ita->second);
		std::string callBack = std::get<0>(ita->second).toStdString();
		if (pWidget)
		{
			QMetaObject::invokeMethod((QObject*)pWidget, callBack.c_str(), Qt::QueuedConnection,
				Q_ARG(unsigned short, proto._cmd), Q_ARG(QByteArray, QByteArray(proto._data.const_data(), proto._data.size())));
		}
	}
	else {
		DTULOG(DTU_ERROR,(char*)"未知的通知命令0x%04x", proto._cmd);
	}
}
