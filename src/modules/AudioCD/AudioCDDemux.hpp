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

#include <Demuxer.hpp>
#include <Playlist.hpp>

#include <QAtomicInt>

#include <cdio/cdio.h>
#include <cddb/cddb.h>

class CDIODestroyTimer final : public QObject
{
    Q_OBJECT
public:
    CDIODestroyTimer();
    ~CDIODestroyTimer();

    Q_SIGNAL void setInstance(CdIo_t *_cdio, const QString &_device, unsigned _discID);
    CdIo_t *getInstance(const QString &_device, unsigned &_discID);
private slots:
    void setInstanceSlot(CdIo_t *_cdio, const QString &_device, unsigned _discID);
private:
    void timerEvent(QTimerEvent *e) override;

    QAtomicInt timerId;
    CdIo_t *cdio;
    QString device;
    unsigned discID;
};

/**/

class AudioCDDemux final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(AudioCDDemux)
public:
    static QStringList getDevices();

    AudioCDDemux(Module &, CDIODestroyTimer &destroyTimer);
private:
    ~AudioCDDemux();

    bool set() override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int & ) override;
    void abort() override;

    bool open(const QString &) override;

    Playlist::Entries fetchTracks(const QString &url, bool &ok) override;

    /**/

    void readCDText(track_t trackNo);

    bool freedb_query(cddb_disc_t *&cddb_disc);
    void freedb_get_disc_info(cddb_disc_t *cddb_disc);
    void freedb_get_track_info(cddb_disc_t *cddb_disc);

    CDIODestroyTimer &destroyTimer;

    Playlist::Entries getTracks(const QString &device);

    QString Title, Artist, Genre, cdTitle, cdArtist, device;
    CdIo_t *cdio;
    track_t trackNo, numTracks;
    lsn_t startSector, numSectors, sector;
    double duration;
    bool isData, aborted, useCDDB, useCDTEXT;
    unsigned char chn;
    unsigned discID;
};

#define AudioCDName "AudioCD"
