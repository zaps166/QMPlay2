// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <NotifiesTray.hpp>

NotifiesTray::NotifiesTray(QSystemTrayIcon *tray) :
    m_tray(tray)
{}

bool NotifiesTray::doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId)
{
    Q_UNUSED(pixmap)
    return doNotify(title, message, ms, iconId);
}
bool NotifiesTray::doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId)
{
    Q_UNUSED(image)
    return doNotify(title, message, ms, iconId);
}
bool NotifiesTray::doNotify(const QString &title, const QString &message, const int ms, const int iconId)
{
    if (m_tray && m_tray->isVisible() && QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages())
    {
        m_tray->showMessage(title, message, (QSystemTrayIcon::MessageIcon)iconId, ms);
        return true;
    }
    return false;
}
