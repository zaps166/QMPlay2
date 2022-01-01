/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <Notifies.hpp>

#include <NotifiesTray.hpp>
#ifdef NOTIFIES_FREEDESKTOP
    #include <NotifiesFreedesktop.hpp>
#endif
#ifdef NOTIFIES_MACOS
    #include <NotifiesMacOS.hpp>
#endif
#ifdef NOTIFIES_ANDROID
    #include <NotifiesAndroid.hpp>
#endif

#include <QApplication>
#include <QStyle>
#include <QIcon>

Notifies *Notifies::s_notifies[2];
bool Notifies::s_nativeFirst;

void Notifies::initialize(QSystemTrayIcon *tray)
{
    if (!s_notifies[0])
    {
#ifdef NOTIFIES_FREEDESKTOP
        s_notifies[0] = new NotifiesFreedesktop;
#endif
#ifdef NOTIFIES_MACOS
        s_notifies[0] = new NotifiesMacOS;
#endif
#ifdef NOTIFIES_ANDROID
        s_notifies[0] = new NotifiesAndroid;
#endif
    }
    if (!s_notifies[1] && tray)
        s_notifies[1] = new NotifiesTray(tray);
    s_nativeFirst = true;

}
bool Notifies::hasBoth()
{
    return (s_notifies[0] && s_notifies[1]);
}
void Notifies::setNativeFirst(bool f)
{
    if (s_nativeFirst != f)
    {
        qSwap(s_notifies[0], s_notifies[1]);
        s_nativeFirst = f;
    }
}
void Notifies::finalize()
{
    for (auto &notifies : s_notifies)
    {
        delete notifies;
        notifies = nullptr;
    }
}

bool Notifies::notify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId)
{
    for (Notifies *notifies : s_notifies)
    {
        if (notifies && notifies->doNotify(title, message, ms, pixmap, iconId))
            return true;
    }
    return false;
}
bool Notifies::notify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId)
{
    for (Notifies *notifies : s_notifies)
    {
        if (notifies && notifies->doNotify(title, message, ms, image, iconId))
            return true;
    }
    return false;
}
bool Notifies::notify(const QString &title, const QString &message, const int ms, const int iconId)
{
    for (Notifies *notifies : s_notifies)
    {
        if (notifies && notifies->doNotify(title, message, ms, iconId))
            return true;
    }
    return false;
}

bool Notifies::doNotify(const QString &title, const QString &message, const int ms, const int iconId)
{
    QPixmap pixmap;
    if (iconId > 0)
    {
        const QIcon icon = QApplication::style()->standardIcon((QStyle::StandardPixmap)((int)QStyle::SP_MessageBoxInformation + iconId - 1));
        const QList<QSize> iconSizes = icon.availableSizes();
        if (!iconSizes.isEmpty())
            pixmap = icon.pixmap(iconSizes.last());
    }
    return doNotify(title, message, ms, pixmap, iconId);
}
