// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ChapterProgramInfo.hpp>
#include <ModuleCommon.hpp>
#include <StreamInfo.hpp>
#include <Playlist.hpp>

class Packet;

class QMPLAY2SHAREDLIB_EXPORT Demuxer : protected ModuleCommon, public BasicIO
{
public:
    class FetchTracks
    {
    public:
        inline FetchTracks(bool onlyTracks) :
            onlyTracks(onlyTracks),
            isOK(true)
        {}

        Playlist::Entries tracks;
        bool onlyTracks, isOK;
    };

    static bool create(const QString &url, IOController<Demuxer> &demuxer, FetchTracks *fetchTracks = nullptr);

    ~Demuxer();

    virtual bool metadataChanged() const;

    virtual bool isStillImage() const;

    inline QList<StreamInfo *> streamsInfo() const
    {
        return streams_info;
    }

    virtual QList<ProgramInfo> getPrograms() const;
    virtual QList<ChapterInfo> getChapters() const;

    virtual QString name() const = 0;
    virtual QString title() const = 0;
    virtual QList<QMPlay2Tag> tags() const;
    virtual bool getReplayGain(bool album, float &gain_db, float &peak) const
    {
        Q_UNUSED(album)
        Q_UNUSED(gain_db)
        Q_UNUSED(peak)
        return false;
    }
    virtual qint64 size() const;
    virtual double length() const = 0;
    virtual int bitrate() const = 0;
    virtual QByteArray image(bool forceCopy = false) const;

    virtual bool localStream() const;
    virtual bool dontUseBuffer() const;

    virtual void selectStreams(const QSet<int> &selectedStreams);

    virtual bool seek(double pos, bool backward) = 0;
    virtual bool read(Packet &, int &) = 0;

private:
    virtual bool open(const QString &url) = 0;

    virtual Playlist::Entries fetchTracks(const QString &url, bool &ok);

protected:
    QList<StreamInfo *> streams_info;
};
