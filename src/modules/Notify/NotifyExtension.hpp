// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Extensions.hpp>

#include <memory>

class NotifyService final : public QObject
{
    Q_OBJECT

public:
    NotifyService(Settings &settings);
    ~NotifyService();

private:
    void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int, bool, const QString &fileName, const QString &url);

private slots:
    void coverDataFromMediaFile(const QByteArray &cover);
    void coverFile(const QString &fileName);

    void playStateChanged(const QString &playState);
    void volumeChanged(double v);

private:
    QString m_summaryFormat, m_bodyFormat, m_lastPlayState;
    QByteArray m_cover;
    qint32 m_timeout;
};

/**/

class NotifyExtension final : public QMPlay2Extensions
{
public:
    NotifyExtension(Module &module);
    ~NotifyExtension();

private:
    bool set() override;

    std::unique_ptr<NotifyService> m_notifyService;
};

#define NotifyExtensionName "Additional Notifications"
