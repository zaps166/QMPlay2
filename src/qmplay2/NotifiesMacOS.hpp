// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Notifies.hpp>

#include <QList>

class NotifiesMacOS final : public Notifies
{
public:
    NotifiesMacOS();
    ~NotifiesMacOS();

private:
    bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId) override;
    bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId) override;

    void *m_notificationItem;
    QList<void *> m_notifications;
};
