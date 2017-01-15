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

#include "notifications_interface.h"

#define FreedesktopNotifyName "Freedesktop Notify"

QDBusArgument &operator<<(QDBusArgument &arg, const QImage &image)
{
	QImage scaledImage;
	if (!image.isNull())
	{
		scaledImage = image.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		if (scaledImage.format() != QImage::Format_ARGB32)
			scaledImage = scaledImage.convertToFormat(QImage::Format_ARGB32);
		scaledImage = scaledImage.rgbSwapped();
	}

	const int channels = 4; // ARGB32 has 4 channels

	arg.beginStructure();
	arg << scaledImage.width();
	arg << scaledImage.height();
	arg << scaledImage.bytesPerLine();
	arg << scaledImage.hasAlphaChannel(); // Should be always "true" for ARGB32
	arg << scaledImage.depth() / channels;
	arg << channels;
#if QT_VERSION < 0x050000
	arg << QByteArray::fromRawData((const char *)scaledImage.constBits(), scaledImage.numBytes());
#else
	arg << QByteArray::fromRawData((const char *)scaledImage.constBits(), scaledImage.byteCount());
#endif
	arg.endStructure();

	return arg;
}
const QDBusArgument &operator >>(const QDBusArgument &arg, QImage &image)
{
	Q_UNUSED(image)
	Q_ASSERT(!"This is needed to link but shouldn't be called");
	return arg;
}

FreedesktopNotify::FreedesktopNotify(qint32 timeout) :
	Notify(timeout),
	m_interface(new OrgFreedesktopNotificationsInterface(OrgFreedesktopNotificationsInterface::staticInterfaceName(), "/org/freedesktop/Notifications", QDBusConnection::sessionBus())),
	m_notificationId(0),
	m_notificationImg(false)
{
	static const int metaTypeId = qDBusRegisterMetaType<QImage>();
	Q_UNUSED(metaTypeId);
}
FreedesktopNotify::~FreedesktopNotify()
{
	delete m_interface;
}

bool FreedesktopNotify::showMessage(const QString &summary, const QString &message, const QString &icon, const QImage &image)
{
	QVariantMap hints;

	//"image_data" is deprecated, "image-data" should be used for version >= 1.2

	if (!image.isNull())
	{
		hints["image_data"] = image;
		m_notificationImg = true;
	}

	int id = 0;
	if (m_lastNotificationTime.msecsTo(QDateTime::currentDateTime()) < m_timeout)
	{
		// Reuse the existing popup if it's still open.  The reason we don't always
		// reuse the popup is because the notification daemon on KDE4 won't re-show
		// the bubble if it's already gone to the tray.
		id = m_notificationId;

		if (m_notificationImg && image.isNull())
		{
			// Discard the image from the previous message.
			QImage image(1, 1, QImage::Format_ARGB32);
			image.fill(Qt::transparent);
			hints["image_data"] = image;
			m_notificationImg = false;
		}

		m_notificationId = 0;
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
		m_lastNotificationTime = QDateTime::currentDateTime();
		m_notificationId = id;
	}

	watcher->deleteLater();
}
