// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Notifies.hpp>

class QMPLAY2SHAREDLIB_EXPORT NotifiesTray final : public Notifies
{
public:
    NotifiesTray(QSystemTrayIcon *tray);
    ~NotifiesTray() final = default;

private:
    bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId) override;
    bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId) override;
    bool doNotify(const QString &title, const QString &message, const int ms, const int iconId) override;

    QSystemTrayIcon *m_tray;
};
