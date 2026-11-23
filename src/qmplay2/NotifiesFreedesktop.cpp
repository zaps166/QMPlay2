// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <NotifiesFreedesktop.hpp>

#include <notifications_interface.h>

#include <QMPlay2Core.hpp>

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
    arg << QByteArray::fromRawData((const char *)scaledImage.constBits(), scaledImage.sizeInBytes());
    arg.endStructure();

    return arg;
}
const QDBusArgument &operator >>(const QDBusArgument &arg, QImage &image)
{
    Q_UNUSED(image)
    Q_ASSERT(!"This is needed to link but shouldn't be called");
    return arg;
}

/**/

NotifiesFreedesktop::NotifiesFreedesktop() :
    m_interface(new OrgFreedesktopNotificationsInterface(OrgFreedesktopNotificationsInterface::staticInterfaceName(), "/org/freedesktop/Notifications", QDBusConnection::sessionBus())),
    m_notificationId(0),
    m_error(false)
{
    static const auto metaTypeId = qDBusRegisterMetaType<QImage>();
    Q_UNUSED(metaTypeId);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(m_interface->GetCapabilities(), this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), SLOT(callFinished(QDBusPendingCallWatcher *)));
}
NotifiesFreedesktop::~NotifiesFreedesktop()
{
    delete m_interface;
}

bool NotifiesFreedesktop::doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId)
{
    return doNotify(title, message, ms, pixmap.toImage(), iconId);
}
bool NotifiesFreedesktop::doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId)
{
    Q_UNUSED(iconId)

    if (m_error)
        return false;

    QVariantMap hints;

    // "image_data" is deprecated, "image-data" should be used for version >= 1.2

    if (!image.isNull())
        hints["image_data"] = image;
    else if (QIcon::fromTheme("QMPlay2").isNull())
    {
        // Set QMPlay2 icon explicitly when it can't be found in system icons.
        // Otherwise it will be set by the Freedesktop notifications automatically.
        hints["image_data"] = QMPlay2Core.getQMPlay2Icon().pixmap(100, 100).toImage();
    }

    int id = 0;
    if (m_lastNotificationTime.msecsTo(QDateTime::currentDateTime()) < ms)
    {
        // Reuse the existing popup if it's still open.  The reason we don't always
        // reuse the popup is because the notification daemon on KDE4 won't re-show
        // the bubble if it's already gone to the tray.
        id = m_notificationId;
        m_notificationId = 0;
    }

    const QDBusPendingReply<quint32> reply = m_interface->Notify(QCoreApplication::applicationName(), id, QCoreApplication::applicationName(), title, message, {}, hints, ms);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), SLOT(callFinished(QDBusPendingCallWatcher *)));

    return true;
}

void NotifiesFreedesktop::callFinished(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError())
        m_error = true;
    else
    {
        const QDBusPendingReply<quint32> reply = *watcher;
        if (reply.isValid())
        {
            const quint32 id = reply.value();
            if (id != 0)
            {
                m_lastNotificationTime = QDateTime::currentDateTime();
                m_notificationId = id;
            }
        }
    }
    watcher->deleteLater();
}
