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

#include <NotifiesAndroid.hpp>

#include <QtAndroidExtras/QtAndroid>
#include <QImage>

enum Duration
{
    SHORT = 0,
    LONG  = 1,
};

/**/

bool NotifiesAndroid::doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId)
{
    Q_UNUSED(pixmap)
    Q_UNUSED(iconId)
    return doNotify(title, message, ms, QImage());
}
bool NotifiesAndroid::doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId)
{
    Q_UNUSED(image)
    Q_UNUSED(iconId)

    QString fullMessage;
    if (!title.isEmpty())
        fullMessage += "[" + title + "]";
    if (!message.isEmpty())
    {
        if (!fullMessage.isEmpty())
            fullMessage += "\n";
        fullMessage += message;
    }

    QtAndroid::runOnAndroidThread([=] {
        QAndroidJniObject toast = QAndroidJniObject::callStaticObjectMethod
        (
            "android/widget/Toast",
            "makeText",
            "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;",
            QtAndroid::androidActivity().object(),
            QAndroidJniObject::fromString(fullMessage).object(),
            jint((ms > 0 && ms < 1500) ? SHORT : LONG)
        );
        toast.callMethod<void>("show");
    });

    return true;
}
