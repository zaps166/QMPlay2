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

#include <LastFM.hpp>

constexpr const char *audioScrobbler2URL = "https://ws.audioscrobbler.com/2.0";
constexpr const char *api_key = "b1165c9688c2fcf29fc74c2ab62ffd90";
constexpr const char *secret = "e36ce24a59f45514daad8fccec294c34";
constexpr int scrobbleSec = 5;

Q_DECLARE_METATYPE(LastFM::Scrobble)

/**/

#include <QCryptographicHash>
#include <QStringList>
#include <QUrlQuery>
#include <QImage>

LastFM::LastFM(Module &module) :
    coverReply(nullptr),
    loginReply(nullptr),
    dontShowLoginError(false),
    firstTime(true)
{
    SetModule(module);
    updateTim.setSingleShot(true);
    loginTimer.setSingleShot(true);
    connect(&updateTim, SIGNAL(timeout()), this, SLOT(processScrobbleQueue()));
    connect(&loginTimer, SIGNAL(timeout()), this, SLOT(login()));
    connect(&QMPlay2Core, SIGNAL(updatePlaying(bool, QString, QString, QString, int, bool, QString)), this, SLOT(updatePlaying(bool, QString, QString, QString, int, bool, QString)));
}

bool LastFM::set()
{
    downloadCovers = sets().getBool("LastFM/DownloadCovers");

    imageSizes.clear();
    if (sets().getBool("LastFM/AllowBigCovers"))
        imageSizes += "mega";
    imageSizes += {"extralarge", "large", "medium", "small"};

    const QString _user = sets().getString("LastFM/Login");
    const QString _md5pass = sets().getString("LastFM/Password");

    if (!sets().getBool("LastFM/UpdateNowPlayingAndScrobble"))
        logout();
    else if (_user != user || _md5pass != md5pass)
    {
        user = _user;
        md5pass = _md5pass;
        if (!firstTime)
        {
            logout(false);
            login();
        }
    }

    firstTime = false;

    return true;
}

void LastFM::getAlbumCover(const QString &title, const QString &artist, const QString &album, bool titleAsAlbum)
{
    /*
     * Get cover for artist and album.
     * If album is not specified or cover art for album doesn't exist - get cover art for title and artist.
     * If title is not specified - do nothing. If cover art for title doesn't exist - set title as album and try again.
     * In the last mode, LastFM gets artist and album again (possibly with other album name), but QMPlay2 gets only artist and title.
    */
    if (!artist.isEmpty() && (!title.isEmpty() || !album.isEmpty()))
    {
        const QString titleEncoded  = title.toUtf8().toPercentEncoding();
        const QString artistEncoded = artist.toUtf8().toPercentEncoding();
        const QString albumEncoded  = album.toUtf8().toPercentEncoding();

        const QString trackOrAlbum = (albumEncoded.isEmpty() ? "track" : "album");

        QString url = audioScrobbler2URL;
        url += QString("/?method=%1.getInfo").arg(trackOrAlbum);
        url += QString("&api_key=%1").arg(api_key);
        url += QString("&artist=%1").arg(artistEncoded);
        url += QString("&%1=%2").arg(trackOrAlbum, albumEncoded.isEmpty() ? titleEncoded : albumEncoded);

        if (coverReply)
        {
            disconnect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
            coverReply->deleteLater();
        }
        coverReply = net.start(url);
        coverReply->setProperty("taa", QStringList {
            titleAsAlbum ? album : title,
            artist,
            titleAsAlbum ? QString() : album
        });
        coverReply->setProperty("titleAsAlbum", titleAsAlbum);
        connect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
    }
}

void LastFM::login()
{
    static const QString getSessionURL = audioScrobbler2URL + QString("/?method=auth.getmobilesession&username=%1&authToken=%2&api_key=%3&api_sig=%4");
    if (!loginReply && !user.isEmpty() && md5pass.length() == 32)
    {
        const QString auth_token = QCryptographicHash::hash(user.toUtf8() + md5pass.toUtf8(), QCryptographicHash::Md5).toHex();
        const QString api_sig = QCryptographicHash::hash(QString("api_key%1authToken%2methodauth.getmobilesessionusername%3%4").arg(api_key, auth_token, user, secret).toUtf8(), QCryptographicHash::Md5).toHex();
        loginReply = net.start(getSessionURL.arg(user, auth_token, api_key, api_sig));
        connect(loginReply, SIGNAL(finished()), this, SLOT(loginFinished()));
    }
}
void LastFM::logout(bool canClear)
{
    updateTim.stop();
    loginTimer.stop();
    if (loginReply)
    {
        loginReply->deleteLater();
        loginReply = nullptr;
    }
    while (!m_scrobbleReplies.isEmpty())
    {
        m_scrobbleReplies.takeLast()->deleteLater();
    }
    if (canClear)
        clear();
    else
        session_key.clear();
}

void LastFM::updateNowPlayingAndScrobble(const Scrobble &scrobble)
{
    if (session_key.isEmpty())
        return;

    NetworkReply *reply = nullptr;

    // updateNowPlaying
    const auto duration = QString::number(qMax<int>(0, scrobble.duration - (time(nullptr) - scrobble.startTime)));

    auto apiSig = QCryptographicHash::hash(
        QString("album%1api_key%2artist%3duration%4methodtrack.updatenowplayingsk%5track%6%7")
            .arg(scrobble.album, api_key, scrobble.artist, duration, session_key, scrobble.title, secret)
            .toUtf8(),
        QCryptographicHash::Md5
    ).toHex();

    QUrlQuery updateNowPlayingQuery;
    updateNowPlayingQuery.addQueryItem("method", "track.updatenowplaying");
    updateNowPlayingQuery.addQueryItem("artist", scrobble.artist);
    updateNowPlayingQuery.addQueryItem("track", scrobble.title);
    updateNowPlayingQuery.addQueryItem("album", scrobble.album.isEmpty() ? "" : scrobble.album);
    updateNowPlayingQuery.addQueryItem("duration", duration);
    updateNowPlayingQuery.addQueryItem("api_key", api_key);
    updateNowPlayingQuery.addQueryItem("api_sig", apiSig);
    updateNowPlayingQuery.addQueryItem("sk", session_key);

    reply = net.start(audioScrobbler2URL, updateNowPlayingQuery.toString(QUrl::EncodeDelimiters).toUtf8(), NetworkAccess::UrlEncoded);
    connect(reply, &NetworkReply::finished,
            reply, &NetworkReply::deleteLater);

    // scrobble
    const auto ts = QString::number(scrobble.startTime);

    apiSig = QCryptographicHash::hash(
        QString("album%1api_key%2artist%3methodtrack.scrobblesk%4timestamp%5track%6%7")
            .arg(scrobble.album, api_key, scrobble.artist, session_key, ts, scrobble.title, secret)
            .toUtf8(),
        QCryptographicHash::Md5
    ).toHex();

    QUrlQuery scrobbleQuery;
    scrobbleQuery.addQueryItem("method", "track.scrobble");
    scrobbleQuery.addQueryItem("artist", scrobble.artist);
    scrobbleQuery.addQueryItem("track", scrobble.title);
    scrobbleQuery.addQueryItem("timestamp", ts);
    scrobbleQuery.addQueryItem("album", scrobble.album.isEmpty() ? "" : scrobble.album);
    scrobbleQuery.addQueryItem("api_key", api_key);
    scrobbleQuery.addQueryItem("api_sig", apiSig);
    scrobbleQuery.addQueryItem("sk", session_key);

    reply = net.start(audioScrobbler2URL, scrobbleQuery.toString(QUrl::EncodeDelimiters).toUtf8(), NetworkAccess::UrlEncoded);
    reply->setProperty("scrobble", QVariant::fromValue(scrobble));
    m_scrobbleReplies.push_back(reply);
    connect(reply, &NetworkReply::destroyed,
            this, [=] {
        m_scrobbleReplies.removeOne(reply);
    });
    connect(reply, &NetworkReply::finished,
            this, &LastFM::scrobbleFinished);
}

void LastFM::clear()
{
    user.clear();
    md5pass.clear();
    updateTim.stop();
    loginTimer.stop();
    session_key.clear();
    scrobbleQueue.clear();
    dontShowLoginError = false;
}

void LastFM::updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName)
{
    Q_UNUSED(fileName)
    if (!artist.isEmpty() && (!title.isEmpty() || !album.isEmpty()))
    {
        if (!user.isEmpty() && !md5pass.isEmpty())
        {
            const time_t currTime = time(nullptr);
            const Scrobble scrobble = {
                title,
                artist,
                album,
                currTime,
                length,
                true,
            };
            if (play)
            {
                if (!scrobbleQueue.isEmpty() && currTime - scrobbleQueue.last().startTime < scrobbleSec)
                    scrobbleQueue.removeLast();
                if (length < 0)
                    length = 0;
                scrobbleQueue.enqueue(scrobble);
            }
            else for (int i = 0; i < scrobbleQueue.count(); ++i)
                if (scrobbleQueue[i] == scrobble && currTime - scrobbleQueue[i].startTime < scrobbleSec)
                    scrobbleQueue.removeAt(i);
            if (scrobbleQueue.isEmpty())
                updateTim.stop();
            else if (!session_key.isEmpty())
                updateTim.start(scrobbleSec * 1000);
            else if (play && !loginTimer.isActive())
                login();
        }
        if (downloadCovers && needCover)
            getAlbumCover(title, artist, album);
    }
}

void LastFM::albumFinished()
{
    const bool isCoverImage = !coverReply->url().contains("api_key");
    const bool titleAsAlbum = coverReply->property("titleAsAlbum").toBool();
    const QStringList taa = coverReply->property("taa").toStringList();
    bool coverNotFound = false;
    if (coverReply->hasError())
    {
        if (!isCoverImage && coverReply->error() == NetworkReply::Error::Connection400)
            coverNotFound = true;
    }
    else
    {
        const QByteArray reply = coverReply->readAll();
        if (isCoverImage)
            emit QMPlay2Core.updateCover(taa[0], taa[1], taa[2], reply);
        else
        {
            for (const QString &size : qAsConst(imageSizes))
            {
                int idx = reply.indexOf(size);
                if (idx > -1)
                {
                    idx += size.length();
                    const int end_idx = reply.indexOf("<", idx);
                    idx = reply.indexOf("http", idx);
                    if (idx > -1 && end_idx > -1 && end_idx > idx)
                    {
                        const QString imgUrl = reply.mid(idx, end_idx - idx);
                        if (imgUrl.contains("noimage"))
                            continue;
                        coverReply->deleteLater();
                        coverReply = net.start(imgUrl);
                        coverReply->setProperty("taa", taa);
                        connect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
                        return;
                    }
                }
            }
            coverNotFound = true;
        }
    }
    if (coverNotFound && !titleAsAlbum)
    {
        if (taa[2].isEmpty())
        {
             //Can't find the track, try find the album with track name
            getAlbumCover(QString(), taa[1], taa[0], true); //This also will delete the old "coverReply"
            return;
        }
        else if (!taa[0].isEmpty() && !taa[1].isEmpty() && !taa[2].isEmpty())
        {
             //Can't find the album, try find the track
            getAlbumCover(taa[0], taa[1], QString()); //This also will delete the old "coverReply"
            return;
        }
    }
    coverReply->deleteLater();
    coverReply = nullptr;
}
void LastFM::loginFinished()
{
    if (loginReply->hasError())
    {
        const bool wrongLoginOrPassword = (loginReply->error() == NetworkReply::Error::Connection403);
        if (!dontShowLoginError || wrongLoginOrPassword)
            QMPlay2Core.logError(tr("LastFM login error.") + (wrongLoginOrPassword ? (" " + tr("Check login and password!")) : QString()));
        if (wrongLoginOrPassword)
            clear();
        else
        {
            dontShowLoginError = true;
            loginTimer.start(30000);
        }
    }
    else
    {
        const QByteArray reply = loginReply->readAll();
        int idx = reply.indexOf("<key>");
        if (idx > -1)
        {
            int end_idx = reply.indexOf("</key>", idx += 5);
            if (end_idx > -1)
            {
                session_key = reply.mid(idx, end_idx - idx);
                QMPlay2Core.log(tr("Logged in to LastFM!"), InfoLog);
                if (!scrobbleQueue.isEmpty() && !updateTim.isActive())
                    updateTim.start(scrobbleSec * 1000);
                dontShowLoginError = false;
            }
        }
    }
    loginReply->deleteLater();
    loginReply = nullptr;
}
void LastFM::scrobbleFinished()
{
    const auto reply = qobject_cast<NetworkReply *>(sender());
    if (reply->hasError())
    {
        auto scrobble = reply->property("scrobble").value<Scrobble>();
        if (scrobble.first)
        {
            scrobble.first = false;
            scrobbleQueue.enqueue(scrobble);
            logout(false);
            login();
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        reply->deleteLater();
    }
}

void LastFM::processScrobbleQueue()
{
    while (!scrobbleQueue.isEmpty())
        updateNowPlayingAndScrobble(scrobbleQueue.dequeue());
}
