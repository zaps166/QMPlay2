/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <X11Notify.hpp>
#include <QMPlay2Core.hpp>
#include <Module.hpp>

#include "dbus/notification.h"

#include <QImage>

#include <QDBusArgument>

#define X11NotifyName "X11 Notify"

QDBusArgument &operator<<(QDBusArgument &arg, const QImage &image)
{
	if (image.isNull())
	{
		// Sometimes this gets called with a null QImage for no obvious reason.
		arg.beginStructure();
		arg << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
		arg.endStructure();
		return arg;
	}
	const QImage scaled = image.scaledToHeight(100, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	// ABGR -> ARGB
	const QImage i = scaled.rgbSwapped();
#else
	// ABGR -> GBAR
	QImage i(scaled.size(), scaled.format());
	for (int y = 0; y < i.height(); ++y)
	{
		QRgb *p = (QRgb *)scaled.scanLine(y);
		QRgb *q = (QRgb *)i.scanLine(y);
		QRgb *end = p + scaled.width();
		while (p < end)
		{
			*q = qRgba(qGreen(*p), qBlue(*p), qAlpha(*p), qRed(*p));
			p++;
			q++;
		}
	}
#endif

	arg.beginStructure();
	arg << i.width();
	arg << i.height();
	arg << i.bytesPerLine();
	arg << i.hasAlphaChannel();
	int channels = i.isGrayscale() ? 1 : (i.hasAlphaChannel() ? 4 : 3);
	arg << i.depth() / channels;
	arg << channels;
#if QT_VERSION < 0x050000
	arg << QByteArray((const char *)(i.bits()), i.numBytes());
#else
	arg << QByteArray((const char *)(i.bits()), i.byteCount());
#endif
	arg.endStructure();
	return arg;
}


const QDBusArgument &operator >>(const QDBusArgument &arg, QImage &)
{
	Q_ASSERT(0); // This is needed to link but shouldn't be called.
	return arg;
}


X11Notify::X11Notify(qint32 timeout) :
	Notify(timeout),
	m_notificationId(0)
{
	m_interface = new OrgFreedesktopNotificationsInterface(
		OrgFreedesktopNotificationsInterface::staticInterfaceName(),
		"/org/freedesktop/Notifications", QDBusConnection::sessionBus());
	if (!m_interface->isValid()) {
		QMPlay2Core.logError(X11NotifyName " :: " + tr("Error connecting to notifications service"));
	}
}

bool X11Notify::showMessage(const QString &summary, const QString &message, const QString &icon, const QImage &image)
{
	if (!m_interface) return false;

	QVariantMap hints;
	if (!image.isNull())
	{
		hints["image_data"] = QVariant(image);
	}

	int id = 0;
	if (m_lastNotificationTime.secsTo(QDateTime::currentDateTime()) * 1000 < m_timeout)
	{
		// Reuse the existing popup if it's still open.  The reason we don't always
		// reuse the popup is because the notification daemon on KDE4 won't re-show
		// the bubble if it's already gone to the tray.
		id = m_notificationId;
	}

	QDBusPendingReply<uint> reply = m_interface->Notify(
				QCoreApplication::applicationName(), id, icon, summary,
				message, QStringList(), hints, m_timeout);
	QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
	connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
		SLOT(CallFinished(QDBusPendingCallWatcher*)));
	return true;
}

X11Notify::~X11Notify()
{
	delete m_interface;
}

void X11Notify::CallFinished(QDBusPendingCallWatcher *watcher)
{
	QDBusPendingReply<uint> reply = *watcher;
	if (reply.isError())
	{
		QMPlay2Core.logError(X11NotifyName " :: " + tr("Error sending notification") + ": " + reply.error().name());
		return;
	}

	uint id = reply.value();
	if (id != 0)
	{
		m_notificationId = id;
		m_lastNotificationTime = QDateTime::currentDateTime();
	}
}
