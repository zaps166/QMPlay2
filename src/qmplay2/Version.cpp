// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Version.hpp>

#include <QMPlay2Core.hpp>

#include <QFile>

#ifndef QMPlay2GitHEAD
    #define QMPlay2GitHEAD
#endif
#define QMPlay2Version "26.08.02" QMPlay2GitHEAD

QByteArray Version::get()
{
    static const QByteArray ver = QMPlay2Version + (isPortable() ? "-portable" : QByteArray());
    return ver;
}
QByteArray Version::userAgent()
{
    static const QByteArray agent = "QMPlay2/" + get();
    return agent;
}
QByteArray Version::userAgentWithMozilla()
{
    static const QByteArray agent = "Mozilla/5.0 " + userAgent();
    return agent;
}
bool Version::isPortable()
{
    static bool portable = QFile::exists(QMPlay2Core.getShareDir() + "portable");
    return portable;
}
