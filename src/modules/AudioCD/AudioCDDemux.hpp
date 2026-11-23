// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Demuxer.hpp>
#include <Playlist.hpp>

#include <AudioCDHeaders.hpp>

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
