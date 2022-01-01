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

#pragma once

#include <QMPlay2Extensions.hpp>

#include <memory>

class NotifyService final : public QObject
{
    Q_OBJECT

public:
    NotifyService(Settings &settings);
    ~NotifyService();

private slots:
    void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int, bool, const QString &fileName);
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
