/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <FreedesktopNotify.hpp>
#include <QMPlay2Core.hpp>
#include <Module.hpp>

#include "notifications_interface.h"

#include <QDBusArgument>

#define FreedesktopNotifyName "Freedesktop Notify"

#if 0 //For future use
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
	// ABGR -> ARGB
	const QImage i = scaled.rgbSwapped();

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
#endif

FreedesktopNotify::FreedesktopNotify(qint32 timeout) :
	Notify(timeout),
	m_interface(new OrgFreedesktopNotificationsInterface(OrgFreedesktopNotificationsInterface::staticInterfaceName(), "/org/freedesktop/Notifications", QDBusConnection::sessionBus())),
	m_notificationId(0)
{}
FreedesktopNotify::~FreedesktopNotify()
{
	delete m_interface;
}

bool FreedesktopNotify::showMessage(const QString &summary, const QString &message, const QString &icon, const QImage &image)
{
	QVariantMap hints;
	if (!image.isNull())
		hints["image_data"] = QVariant(image);

	int id = 0;
	if (m_lastNotificationTime.msecsTo(QDateTime::currentDateTime()) < m_timeout)
	{
		// Reuse the existing popup if it's still open.  The reason we don't always
		// reuse the popup is because the notification daemon on KDE4 won't re-show
		// the bubble if it's already gone to the tray.
		id = m_notificationId;
	}

	const QDBusPendingReply<quint32> reply = m_interface->Notify(QCoreApplication::applicationName(), id, icon, summary, message, QStringList(), hints, m_timeout);
	QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
	connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), SLOT(callFinished(QDBusPendingCallWatcher *)));
	return true;
}

void FreedesktopNotify::callFinished(QDBusPendingCallWatcher *watcher)
{
	const QDBusPendingReply<quint32> reply = *watcher;
	if (reply.isError())
	{
		QMPlay2Core.logError(FreedesktopNotifyName " :: " + tr("Error sending notification") + ": " + reply.error().name());
		return;
	}

	const quint32 id = reply.value();
	if (id != 0)
	{
		m_notificationId = id;
		m_lastNotificationTime = QDateTime::currentDateTime();
	}

	watcher->deleteLater();
}
