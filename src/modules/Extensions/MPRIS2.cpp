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

#include <MPRIS2.hpp>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>

static void propertyChanged(const QString &name, const QVariant &value)
{
    QVariantMap map;
    map.insert(name, value);
    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged");
    msg << "org.mpris.MediaPlayer2.Player" << map << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

/**/

MediaPlayer2Root::MediaPlayer2Root(QObject *p) :
    QDBusAbstractAdaptor(p),
    fullScreen(false)
{
    connect(&QMPlay2Core, SIGNAL(fullScreenChanged(bool)), this, SLOT(fullScreenChanged(bool)));
}

bool MediaPlayer2Root::canQuit() const
{
    return true;
}
bool MediaPlayer2Root::canRaise() const
{
    return true;
}
bool MediaPlayer2Root::canSetFullscreen() const
{
    return true;
}
bool MediaPlayer2Root::isFullscreen() const
{
    return fullScreen;
}
void MediaPlayer2Root::setFullscreen(bool fs)
{
    if (fullScreen != fs)
    {
        emit QMPlay2Core.processParam("fullscreen");
        fullScreen = fs;
    }
}
bool MediaPlayer2Root::hasTrackList() const
{
    parent()->setProperty("exportCovers", true);
    return false;
}
QString MediaPlayer2Root::identity() const
{
    return QCoreApplication::applicationName();
}
QString MediaPlayer2Root::desktopEntry() const
{
    return QCoreApplication::applicationName();
}
QStringList MediaPlayer2Root::supportedMimeTypes() const
{
    return {};
}
QStringList MediaPlayer2Root::supportedUriSchemes() const
{
    return {};
}

void MediaPlayer2Root::Quit()
{
    emit QMPlay2Core.processParam("quit");
}
void MediaPlayer2Root::Raise()
{
    emit QMPlay2Core.processParam("show");
}

void MediaPlayer2Root::fullScreenChanged(bool fs)
{
    propertyChanged("Fullscreen", fullScreen = fs);
}

/**/

MediaPlayer2Player::MediaPlayer2Player(QObject *p) :
    QDBusAbstractAdaptor(p),
    removeCover(false),
    trackID(QDBusObjectPath(QString("/org/qmplay2/MediaPlayer2/Track/0"))), //I don't use TrackID in QMPlay2
    playState("Stopped"),
    can_seek(false),
    vol(1.0), r(1.0),
    pos(0)
{
    clearMetaData();
    m_data["mpris:trackid"] = QVariant::fromValue<QDBusObjectPath>(trackID);
    connect(&QMPlay2Core, SIGNAL(updatePlaying(bool, const QString &, const QString &, const QString &, int, bool, const QString &)), this, SLOT(updatePlaying(bool, const QString &, const QString &, const QString &, int, bool, const QString &)));
    connect(&QMPlay2Core, SIGNAL(coverDataFromMediaFile(const QByteArray &)), this, SLOT(coverDataFromMediaFile(const QByteArray &)));
    connect(&QMPlay2Core, SIGNAL(playStateChanged(const QString &)), this, SLOT(playStateChanged(const QString &)));
    connect(&QMPlay2Core, SIGNAL(coverFile(const QString &)), this, SLOT(coverFile(const QString &)));
    connect(&QMPlay2Core, SIGNAL(speedChanged(double)), this, SLOT(speedChanged(double)));
    connect(&QMPlay2Core, SIGNAL(volumeChanged(double)), this, SLOT(volumeChanged(double)));
    connect(&QMPlay2Core, SIGNAL(posChanged(int)), this, SLOT(posChanged(int)));
    connect(&QMPlay2Core, SIGNAL(seeked(int)), this, SLOT(seeked(int)));
}
MediaPlayer2Player::~MediaPlayer2Player()
{
    if (removeCover)
        QFile::remove(m_data["mpris:artUrl"].toString().remove("file://"));
}

bool MediaPlayer2Player::canControl() const
{
    return true;
}
bool MediaPlayer2Player::canGoNext() const
{
    return true;
}
bool MediaPlayer2Player::canGoPrevious() const
{
    return true;
}
bool MediaPlayer2Player::canPause() const
{
    return true;
}
bool MediaPlayer2Player::canPlay() const
{
    return true;
}
bool MediaPlayer2Player::canSeek() const
{
    return can_seek;
}

QVariantMap MediaPlayer2Player::metadata() const
{
    parent()->setProperty("exportCovers", true);
    return m_data;
}
QString MediaPlayer2Player::playbackStatus() const
{
    return playState;
}
qint64 MediaPlayer2Player::position() const
{
    return pos;
}

double MediaPlayer2Player::minimumRate() const
{
    return 0.05;
}
double MediaPlayer2Player::maximumRate() const
{
    return 100.0;
}
double MediaPlayer2Player::rate() const
{
    return r;
}
void MediaPlayer2Player::setRate(double rate)
{
    if (rate >= minimumRate() && rate <= maximumRate())
        emit QMPlay2Core.processParam("speed", QString::number(rate));
}

double MediaPlayer2Player::volume() const
{
    return vol;
}
void MediaPlayer2Player::setVolume(double value)
{
    emit QMPlay2Core.processParam("volume", QString::number((int)(value * 100)));
}

void MediaPlayer2Player::Next()
{
    emit QMPlay2Core.processParam("next");
}
void MediaPlayer2Player::Previous()
{
    emit QMPlay2Core.processParam("prev");
}
void MediaPlayer2Player::Pause()
{
    if (playState == "Playing")
        emit QMPlay2Core.processParam("toggle");
}
void MediaPlayer2Player::PlayPause()
{
    emit QMPlay2Core.processParam("toggle");
}
void MediaPlayer2Player::Stop()
{
    emit QMPlay2Core.processParam("stop");
}
void MediaPlayer2Player::Play()
{
    if (playState != "Playing")
        emit QMPlay2Core.processParam("toggle");
}
void MediaPlayer2Player::Seek(qint64 Offset)
{
    if (Offset != 0)
    {
        const qint64 Position = position() + Offset;
        SetPosition(trackID, Position < 0 ? 0 : Position);
    }
}
void MediaPlayer2Player::SetPosition(const QDBusObjectPath &TrackId, qint64 Position)
{
    if (trackID == TrackId && Position != position() && Position >= 0 && Position <= m_data["mpris:length"].toLongLong())
        emit QMPlay2Core.processParam("seek", QString::number(Position / 1000000LL));
}
void MediaPlayer2Player::OpenUri(const QString &Uri)
{
    emit QMPlay2Core.processParam("open", Uri);
}

void MediaPlayer2Player::updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName)
{
    Q_UNUSED(needCover)
    const bool tmp = play && length > 0;
    if (tmp != can_seek)
        propertyChanged("CanSeek", can_seek = tmp);
    if (!play)
        clearMetaData();
    else
    {
        m_data["mpris:length"] = length < 0 ? -1 : length * 1000000LL;
        if (title.isEmpty() && artist.isEmpty())
            m_data["xesam:title"] = fileName;
        else
        {
            m_data["xesam:title"] = title;
            m_data["xesam:artist"] = QStringList{artist};
        }
        m_data["xesam:album"] = album;
    }
    propertyChanged("Metadata", m_data);
}
void MediaPlayer2Player::coverDataFromMediaFile(const QByteArray &cover)
{
    if (parent()->property("exportCovers").toBool())
    {
        QFile coverF(QDir::tempPath() + "/QMPlay2." + QString("%1.%2.mpris2cover").arg(getenv("USER")).arg(QCoreApplication::applicationPid()));
        if (coverF.open(QFile::WriteOnly))
        {
            coverF.write(cover);
            coverF.close();
            m_data["mpris:artUrl"] = QString("file://" + coverF.fileName());
            propertyChanged("Metadata", m_data);
            removeCover = true;
        }
    }
}

void MediaPlayer2Player::playStateChanged(const QString &plState)
{
    propertyChanged("PlaybackStatus", playState = plState);
}
void MediaPlayer2Player::coverFile(const QString &filePath)
{
    m_data["mpris:artUrl"] = QString("file://" + filePath);
    propertyChanged("Metadata", m_data);
    removeCover = false;
}
void MediaPlayer2Player::speedChanged(double speed)
{
    propertyChanged("Rate", r = speed);
}
void MediaPlayer2Player::volumeChanged(double v)
{
    propertyChanged("Volume", vol = v);
}
void MediaPlayer2Player::posChanged(int p)
{
    pos = p * 1000000LL;
    propertyChanged("Position", pos);
}
void MediaPlayer2Player::seeked(int pos)
{
    emit Seeked(pos * 1000000LL);
}

void MediaPlayer2Player::clearMetaData()
{
    if (removeCover)
    {
        QFile::remove(m_data["mpris:artUrl"].toString().remove("file://"));
        removeCover = false;
    }
    m_data["mpris:artUrl"] = m_data["xesam:title"] = m_data["xesam:album"] = QString();
    m_data["xesam:artist"] = QStringList{QString()};
    m_data["mpris:length"] = qint64();
}

/**/

MPRIS2Interface::MPRIS2Interface() :
    service("org.mpris.MediaPlayer2.QMPlay2"),
    objectOk(false), serviceOk(false),
    mediaPlayer2Root(this),
    mediaPlayer2Player(this)
{
    if (QDBusConnection::sessionBus().registerObject("/org/mpris/MediaPlayer2", this))
    {
        objectOk = true;
        serviceOk = QDBusConnection::sessionBus().registerService(service);
        if (!serviceOk)
        {
            service += QString(".instance%1").arg(QCoreApplication::applicationPid());
            serviceOk = QDBusConnection::sessionBus().registerService(service);
        }
    }
}
MPRIS2Interface::~MPRIS2Interface()
{
    if (serviceOk)
        QDBusConnection::sessionBus().unregisterService(service);
    if (objectOk)
        QDBusConnection::sessionBus().unregisterObject("/org/mpris/MediaPlayer2");
}

inline bool MPRIS2Interface::isOk() const
{
    return objectOk && serviceOk;
}

/**/

MPRIS2::MPRIS2(Module &module)
{
    SetModule(module);
}
MPRIS2::~MPRIS2()
{}

bool MPRIS2::set()
{
    if (!sets().getBool("MPRIS2/Enabled"))
        mpris2Interface.reset();
    else if (!mpris2Interface)
        mpris2Interface.reset(new MPRIS2Interface);
    if (mpris2Interface && !mpris2Interface->isOk())
        mpris2Interface.reset();
    return true;
}
