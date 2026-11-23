// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <NotifiesAndroid.hpp>

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
