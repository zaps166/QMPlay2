#ifndef PORTAUDIOCOMMON_HPP
#define PORTAUDIOCOMMON_HPP

#include <QStringList>
#include <QPair>

namespace ALSACommon
{
	typedef QPair< QStringList, QStringList > DevicesList;

	DevicesList getDevices();

	QString getDeviceName(const DevicesList &devicesList, const QString &deviceName);
}

#endif
