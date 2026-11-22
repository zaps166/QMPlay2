// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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

    void getAlbumCover(const QString &title, const QString &artist, const QString &album, const QString &origTitle, bool titleAsAlbum = false);

    Q_SLOT void login();
    void logout(bool canClear = true);

    void updateNowPlayingAndScrobble(const Scrobble &scrobble);

    void clear();

    void updatePlaying(bool play, QString title, QString artist, const QString &album, int length, bool needCover, const QString &fileName, const QString &url);

private slots:
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
