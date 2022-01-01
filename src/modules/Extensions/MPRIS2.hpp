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

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include <memory>

class MediaPlayer2Root final : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen)
    Q_PROPERTY(bool Fullscreen READ isFullscreen WRITE setFullscreen)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
public:
    MediaPlayer2Root(QObject *p);

    bool canQuit() const;
    bool canRaise() const;
    bool canSetFullscreen() const;
    bool isFullscreen() const;
    void setFullscreen(bool fs);
    bool hasTrackList() const;
    QString identity() const;
    QString desktopEntry() const;
    QStringList supportedMimeTypes() const;
    QStringList supportedUriSchemes() const;
public slots:
    void Quit();
    void Raise();
private slots:
    void fullScreenChanged(bool fs);
private:
    bool fullScreen;
};

/**/

class MediaPlayer2Player final : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

    Q_PROPERTY(bool CanControl READ canControl)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(qint64 Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
public:
    MediaPlayer2Player(QObject *p);
    ~MediaPlayer2Player();

    bool canControl() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPause() const;
    bool canPlay() const;
    bool canSeek() const;

    QVariantMap metadata() const;
    QString playbackStatus() const;
    qint64 position() const;

    double minimumRate() const;
    double maximumRate() const;
    double rate() const;
    void setRate(double rate);

    double volume() const;
    void setVolume(double value);
public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qint64 Offset);
    void SetPosition(const QDBusObjectPath &TrackId, qint64 Position);
    void OpenUri(const QString &Uri);
signals:
    void Seeked(qint64 Position);
private slots:
    void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName);
    void coverDataFromMediaFile(const QByteArray &cover);
    void playStateChanged(const QString &plState);
    void coverFile(const QString &filePath);
    void speedChanged(double speed);
    void volumeChanged(double v);
    void posChanged(int p);
    void seeked(int pos);
private:
    void clearMetaData();

    bool removeCover;

    QDBusObjectPath trackID;
    QVariantMap m_data;
    QString playState;
    bool can_seek;
    double vol, r;
    qint64 pos;
};

/**/

class MPRIS2Interface final : public QObject
{
public:
    MPRIS2Interface();
    ~MPRIS2Interface();

    inline bool isOk() const;
private:
    QString service;
    bool objectOk, serviceOk;
    MediaPlayer2Root mediaPlayer2Root;
    MediaPlayer2Player mediaPlayer2Player;
};

/**/

#include <QMPlay2Extensions.hpp>

class MPRIS2 final : public QMPlay2Extensions
{
public:
    MPRIS2(Module &module);
    ~MPRIS2();
private:
    bool set() override;

    std::unique_ptr<MPRIS2Interface> mpris2Interface;
};

#define MPRIS2Name "MPRIS2"
