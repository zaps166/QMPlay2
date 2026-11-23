// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class QMPLAY2SHAREDLIB_EXPORT Notifies
{
public:
    static void initialize(QSystemTrayIcon *tray);
    static bool hasBoth();
    static void setNativeFirst(bool f);
    static void finalize();

    static bool notify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId = 0);
    static bool notify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId = 0);
    static bool notify(const QString &title, const QString &message, const int ms, const int iconId = 0);

protected:
    virtual ~Notifies() = default;

    virtual bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId) = 0;
    virtual bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId) = 0;
    virtual bool doNotify(const QString &title, const QString &message, const int ms, const int iconId);

private:
    static Notifies *s_notifies[2];
    static bool s_nativeFirst;
};
