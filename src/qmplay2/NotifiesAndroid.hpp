// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Notifies.hpp>

#include <QObject>

class NotifiesAndroid final : public QObject, public Notifies
{
    Q_OBJECT

    bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId = 0) override;
    bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId = 0) override;
};
