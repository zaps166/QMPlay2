// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Notifies.hpp>

#include <QDateTime>
#include <QObject>

class OrgFreedesktopNotificationsInterface;
class QDBusPendingCallWatcher;

class QMPLAY2SHAREDLIB_EXPORT NotifiesFreedesktop final : public QObject, public Notifies
{
    Q_OBJECT

public:
    NotifiesFreedesktop();
    ~NotifiesFreedesktop();

private:
    bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId = 0) override;
    bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId = 0) override;

private slots:
    void callFinished(QDBusPendingCallWatcher *watcher);

private:
    OrgFreedesktopNotificationsInterface *m_interface;
    QDateTime m_lastNotificationTime;
    quint32 m_notificationId;
    bool m_error;
};
