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

#include <AudioCDDemux.hpp>

#include <Functions.hpp>
#include <Packet.hpp>

#ifdef Q_OS_WIN
    #include <QDir>
    static const QRegExp cdaRegExp("file://\\D:/track\\d\\d.cda");
#endif

#define CD_BLOCKSIZE 2352/2
#define srate 44100

CDIODestroyTimer::CDIODestroyTimer()
{
    connect(this, SIGNAL(setInstance(CdIo_t *, const QString &, unsigned)), this, SLOT(setInstanceSlot(CdIo_t *, const QString &, unsigned)));
}
CDIODestroyTimer::~CDIODestroyTimer()
{
    if (timerId.fetchAndStoreRelaxed(0))
        cdio_destroy(cdio);
}

CdIo_t *CDIODestroyTimer::getInstance(const QString &_device, unsigned &_discID)
{
    if (timerId.fetchAndStoreRelaxed(0))
    {
        if (_device == device)
        {
            _discID = discID;
            return cdio;
        }
        cdio_destroy(cdio);
    }
    return nullptr;
}

void CDIODestroyTimer::setInstanceSlot(CdIo_t *_cdio, const QString &_device, unsigned _discID)
{
    const int newTimerId = startTimer(2500);
    CdIo_t *oldCdIo = cdio;
    if (!newTimerId)
        cdio_destroy(_cdio);
    else
    {
        cdio = _cdio;
        device = _device;
        discID = _discID;
    }
    if (timerId.fetchAndStoreRelaxed(newTimerId))
        cdio_destroy(oldCdIo);
}

void CDIODestroyTimer::timerEvent(QTimerEvent *e)
{
    if (timerId.testAndSetRelaxed(e->timerId(), 0))
        cdio_destroy(cdio);
    killTimer(e->timerId());
}

/**/

QStringList AudioCDDemux::getDevices()
{
    QStringList devicesList;
    if (char **devices = cdio_get_devices(DRIVER_DEVICE))
    {
        for (size_t i = 0; char *device = devices[i]; ++i)
        {
            devicesList += device;
#ifdef Q_OS_WIN
            devicesList.last().remove(0, 4);
#endif
        }
        cdio_free_device_list(devices);
    }
    return devicesList;
}

AudioCDDemux::AudioCDDemux(Module &module, CDIODestroyTimer &destroyTimer) :
    destroyTimer(destroyTimer),
    cdio(nullptr),
    sector(0),
    aborted(false),
    discID(0)
{
    SetModule(module);
}

AudioCDDemux::~AudioCDDemux()
{
    if (cdio)
        emit destroyTimer.setInstance(cdio, device, discID);
}

bool AudioCDDemux::set()
{
    useCDDB   = sets().getBool("AudioCD/CDDB");
    useCDTEXT = sets().getBool("AudioCD/CDTEXT");
    return true;
}

QString AudioCDDemux::name() const
{
    if (!cdTitle.isEmpty() && !cdArtist.isEmpty())
        return AudioCDName " [" + cdArtist + " - " + cdTitle + "]";
    else if (!cdTitle.isEmpty())
        return AudioCDName " [" + cdTitle + "]";
    else if (!cdArtist.isEmpty())
        return AudioCDName " [" + cdArtist + "]";
    return AudioCDName;
}
QString AudioCDDemux::title() const
{
    QString prefix, suffix;
    const QString artist = QMPlay2Core.getSettings().getBool("HideArtistMetadata") ? QString() : Artist;
    if (isData)
        suffix = " - " + tr("Data");
    else if (!Title.isEmpty() && !artist.isEmpty())
        return artist + " - " + Title;
    else if (!Title.isEmpty())
        return Title;
    else if (!artist.isEmpty())
        prefix = artist + " - ";
    return prefix + tr("Track") + " " + QString::number(trackNo) + suffix;
}
QList<QMPlay2Tag> AudioCDDemux::tags() const
{
    QList<QMPlay2Tag> tagList;
    if (!Title.isEmpty())
        tagList += {QString::number(QMPLAY2_TAG_TITLE), Title};
    if (!Artist.isEmpty())
        tagList += {QString::number(QMPLAY2_TAG_ARTIST), Artist};
    if (!cdTitle.isEmpty())
        tagList += {QString::number(QMPLAY2_TAG_ALBUM), cdTitle};
    if (!Genre.isEmpty())
        tagList += {QString::number(QMPLAY2_TAG_GENRE), Genre};
    tagList += {tr("Track"), QString::number(trackNo)};
    return tagList;
}
double AudioCDDemux::length() const
{
    return numSectors * duration;
}
int AudioCDDemux::bitrate() const
{
    return 8 * (srate * chn * 2) / 1000;
}

bool AudioCDDemux::seek(double s, bool)
{
    return (sector = (s / duration)) < numSectors;
}
bool AudioCDDemux::read(Packet &decoded, int &idx)
{
    if (aborted || numSectors <= sector || isData)
        return false;

    short cd_samples[CD_BLOCKSIZE];
    if (cdio_read_audio_sector(cdio, cd_samples, startSector + sector) == DRIVER_OP_SUCCESS)
    {
        decoded.resize(CD_BLOCKSIZE * sizeof(float));
        float *decoded_data = (float *)decoded.data();
        for (int i = 0; i < CD_BLOCKSIZE; ++i)
            decoded_data[i] = cd_samples[i] / 32768.0f;

        idx = 0;
        decoded.setTS(sector * duration);
        decoded.setDuration(duration);

        ++sector;
        return true;
    }

    return false;
}
void AudioCDDemux::abort()
{
    aborted = true;
}

bool AudioCDDemux::open(const QString &_url)
{
#ifdef Q_OS_WIN
    if (_url.toLower().contains(cdaRegExp))
    {
        QString url = _url;
        url.remove("file://");
        device = url.mid(0, url.indexOf('/'));
        trackNo = url.mid(url.toLower().indexOf("track") + 5, 2).toUInt();
    }
    else
#endif
    {
        QString prefix, param;
        if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(_url, &prefix, &device, &param) || prefix != AudioCDName)
            return false;
#ifdef Q_OS_WIN
        if (device.endsWith("/"))
            device.chop(1);
#endif
        bool ok;
        trackNo = param.toInt(&ok);
        if (!ok)
            return false;
    }
    if (trackNo > 0 && trackNo < CDIO_INVALID_TRACK)
    {
        cdio = destroyTimer.getInstance(device, discID);
        if (cdio || (cdio = cdio_open(device.toLocal8Bit(), DRIVER_UNKNOWN)))
        {
            cdio_set_speed(cdio, 1);
            numTracks = cdio_get_num_tracks(cdio);
            if (cdio_get_discmode(cdio) != CDIO_DISC_MODE_ERROR && numTracks > 0 && numTracks != CDIO_INVALID_TRACK)
            {
                chn = cdio_get_track_channels(cdio, trackNo);
                if (!chn) //NRG format returns 0 - why ?
                    chn = 2;
                if (numTracks >= trackNo && (chn == 2 || chn == 4))
                {
                    if (useCDTEXT)
                    {
                        readCDText(0);
                        readCDText(trackNo);
                    }
                    isData = cdio_get_track_format(cdio, trackNo) != TRACK_FORMAT_AUDIO;
                    duration = CD_BLOCKSIZE / chn / (double)srate;
                    startSector = cdio_get_track_lsn(cdio, trackNo);
                    numSectors = cdio_get_track_last_lsn(cdio, trackNo) - startSector;

                    if (useCDDB && Title.isEmpty())
                    {
                        cddb_disc_t *cddb_disc;
                        if (freedb_query(cddb_disc))
                        {
                            if (cdTitle.isEmpty() && cdArtist.isEmpty())
                                freedb_get_disc_info(cddb_disc);
                            freedb_get_track_info(cddb_disc);
                            cddb_disc_destroy(cddb_disc);
                        }
                    }

                    streams_info += new StreamInfo(srate, chn);
                    return true;
                }
                else
                    QMPlay2Core.log(tr("Error reading track"));
            }
            else
                QMPlay2Core.log(tr("No CD"));
        }
        else
            QMPlay2Core.log(tr("Invalid path to CD drive"));
    }
    return false;
}

Playlist::Entries AudioCDDemux::fetchTracks(const QString &url, bool &ok)
{
    Playlist::Entries entries;
    if (!url.contains("://{") && url.startsWith(AudioCDName "://"))
    {
        const QString realUrl = url.mid(strlen(AudioCDName) + 3);
        entries = getTracks(realUrl);
        if (entries.isEmpty())
            emit QMPlay2Core.sendMessage(tr("No AudioCD found!"), AudioCDName, 2, 0);
        else
        {
            for (int i = 0; i < entries.count(); ++i)
                entries[i].parent = 1;
            Playlist::Entry entry;
            entry.name = "Audio CD (" + realUrl + ")";
            entry.url = url;
            entry.GID = 1;
            entries.prepend(entry);
        }
    }
    if (entries.isEmpty())
    {
#ifdef Q_OS_WIN
        if (!url.toLower().contains(cdaRegExp))
#endif
            ok = false;
    }
    return entries;
}

void AudioCDDemux::readCDText(track_t trackNo)
{
#if LIBCDIO_VERSION_NUM >= 84
    if (cdtext_t *cdtext = cdio_get_cdtext(cdio))
    {
        if (trackNo == 0)
        {
            cdTitle  = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, 0);
            cdArtist = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, 0);
        }
        else
        {
            Title  = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, trackNo);
            Artist = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, trackNo);
            Genre  = cdtext_get_const(cdtext, CDTEXT_FIELD_GENRE, trackNo);
        }
    }
#else
    if (cdtext_t *cdtext = cdio_get_cdtext(cdio, trackNo))
    {
        if (trackNo == 0)
        {
            cdTitle  = cdtext_get_const(CDTEXT_TITLE, cdtext);
            cdArtist = cdtext_get_const(CDTEXT_PERFORMER, cdtext);
        }
        else
        {
            Title  = cdtext_get_const(CDTEXT_TITLE, cdtext);
            Artist = cdtext_get_const(CDTEXT_PERFORMER, cdtext);
            Genre  = cdtext_get_const(CDTEXT_GENRE, cdtext);
        }
    }
#endif
}

bool AudioCDDemux::freedb_query(cddb_disc_t *&cddb_disc)
{
#ifdef Q_OS_WIN
    bool hasHomeEnv = getenv("HOME");
    if (!hasHomeEnv)
        putenv("HOME=" + QDir::homePath().toLocal8Bit());
#endif
    cddb_conn_t *cddb = cddb_new();
#ifdef Q_OS_WIN
    if (!hasHomeEnv)
        putenv("HOME=");
#endif
    cddb_disc = cddb_disc_new();

#ifdef Q_OS_WIN
    const QString cddbDir = QMPlay2Core.getSettingsDir() + "CDDB";
    if (QDir(cddbDir).exists() || QDir(QMPlay2Core.getSettingsDir()).mkdir("CDDB"))
        cddb_cache_set_dir(cddb, cddbDir.toLocal8Bit());
#endif

    cddb_disc_set_length(cddb_disc, FRAMES_TO_SECONDS(cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK)));
    for (int trackno = 1; trackno <= numTracks; ++trackno)
    {
        cddb_track_t *pcddb_track = cddb_track_new();
        cddb_track_set_frame_offset(pcddb_track, cdio_get_track_lba(cdio, trackno));
        cddb_disc_add_track(cddb_disc, pcddb_track);
    }

    cddb_disc_calc_discid(cddb_disc);
    if (cddb_disc_get_discid(cddb_disc) == discID)
        cddb_cache_only(cddb);
    else
    {
        discID = cddb_disc_get_discid(cddb_disc);

        cddb_set_timeout(cddb, 3);
        cddb_set_server_name(cddb, "gnudb.gnudb.org");
        cddb_set_server_port(cddb, 8880);

        Settings sets("QMPlay2");
        if (sets.getBool("Proxy/Use"))
        {
            cddb_http_proxy_enable(cddb);
            cddb_set_http_proxy_server_name(cddb, sets.getString("Proxy/Host").toLocal8Bit());
            cddb_set_http_proxy_server_port(cddb, sets.getUInt("Proxy/Port"));
            if (sets.getBool("Proxy/Login"))
            {
                cddb_set_http_proxy_username(cddb, sets.getString("Proxy/User").toLocal8Bit());
                cddb_set_http_proxy_password(cddb, QString(QByteArray::fromBase64(sets.getByteArray("Proxy/Password"))).toLocal8Bit());
            }
        }
    }

    if (cddb_query(cddb, cddb_disc) > 0)
    {
        do if (cddb_disc_get_discid(cddb_disc) == discID)
        {
            cddb_read(cddb, cddb_disc);
            cddb_destroy(cddb);
            return true;
        } while (cddb_query_next(cddb, cddb_disc));
    }

    cddb_disc_destroy(cddb_disc);
    cddb_destroy(cddb);
    cddb_disc = nullptr;
    return false;
}
void AudioCDDemux::freedb_get_disc_info(cddb_disc_t *cddb_disc)
{
    if (cddb_disc)
    {
        cdTitle  = cddb_disc_get_title(cddb_disc);
        cdArtist = cddb_disc_get_artist(cddb_disc);
    }
}
void AudioCDDemux::freedb_get_track_info(cddb_disc_t *cddb_disc)
{
    cddb_track_t *cddb_track;
    if (cddb_disc && (cddb_track = cddb_disc_get_track(cddb_disc, trackNo - 1)))
    {
        Title  = cddb_track_get_title(cddb_track);
        Artist = cddb_track_get_artist(cddb_track);
    }
}

Playlist::Entries AudioCDDemux::getTracks(const QString &_device)
{
    Playlist::Entries tracks;
    device = _device;
#ifdef Q_OS_WIN
    if (device.endsWith("/"))
        device.chop(1);
#endif
    cdio_close_tray(device.toLocal8Bit(), nullptr);
    if ((cdio = cdio_open(device.toLocal8Bit(), DRIVER_UNKNOWN)))
    {
        numTracks = cdio_get_num_tracks(cdio);
        if (cdio_get_discmode(cdio) != CDIO_DISC_MODE_ERROR && numTracks > 0 && numTracks != CDIO_INVALID_TRACK)
        {
            cddb_disc_t *cddb_disc = nullptr;
            bool cddb_ok = useCDDB;
            for (trackNo = 1; trackNo <= numTracks; ++trackNo)
            {
                chn = cdio_get_track_channels(cdio, trackNo);
                if (!chn) //NRG format returns 0 - why ?
                    chn = 2;
                if (chn != 2 && chn != 4)
                    continue;

                if (useCDTEXT)
                    readCDText(trackNo);
                isData = cdio_get_track_format(cdio, trackNo) != TRACK_FORMAT_AUDIO;
                duration = CD_BLOCKSIZE / chn / (double)srate;
                numSectors = cdio_get_track_last_lsn(cdio, trackNo) - cdio_get_track_lsn(cdio, trackNo);

                if (cddb_ok && (cddb_disc || (Title.isEmpty() && (cddb_ok = freedb_query(cddb_disc)))))
                    freedb_get_track_info(cddb_disc);

                Playlist::Entry entry;
                entry.name = title();
                entry.url = AudioCDName "://{" + device + "}" + QString::number(trackNo);
                entry.length = length();
                tracks += entry;
            }
            cddb_disc_destroy(cddb_disc);
        }
    }
    return tracks;
}
