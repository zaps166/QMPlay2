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

#include <NetworkAccess.hpp>

#include <QTimer>
#include <QQueue>

#include <time.h>

class QImage;

class LastFM final : public QObject, public QMPlay2Extensions
{
    Q_OBJECT
public:
    class Scrobble
    {
    public:
        inline bool operator ==(const Scrobble &other)
        {
            return title == other.title && artist == other.artist && album == other.album && duration == other.duration;
        }

        QString title, artist, album;
        time_t startTime;
        int duration;
        bool first;
    };

    LastFM(Module &module);
private:
    bool set() override;

    void getAlbumCover(const QString &title, const QString &artist, const QString &album, bool titleAsAlbum = false);

    Q_SLOT void login();
    void logout(bool canClear = true);

    void updateNowPlayingAndScrobble(const Scrobble &scrobble);

    void clear();
private slots:
    void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName);

    void albumFinished();
    void loginFinished();
    void scrobbleFinished();

    void processScrobbleQueue();
private:
    NetworkReply *coverReply, *loginReply;
    QList<NetworkReply *> m_scrobbleReplies;
    bool downloadCovers, dontShowLoginError, firstTime;
    QString user, md5pass, session_key;
    QQueue<Scrobble> scrobbleQueue;
    QTimer updateTim, loginTimer;
    NetworkAccess net;
    QStringList imageSizes;
};

#define LastFMName "LastFM"
