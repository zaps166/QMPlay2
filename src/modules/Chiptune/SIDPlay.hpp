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

#include <sidplayfp/builders/residfp.h>
#include <sidplayfp/sidplayfp.h>

class SidTuneInfo;
class Reader;

class SIDPlay final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(SIDPlay)
public:
    SIDPlay(Module &);
    ~SIDPlay();
private:
    bool set() override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double, bool backward) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    Playlist::Entries fetchTracks(const QString &url, bool &ok) override;


    bool open(const QString &url, bool tracksOnly);

    QString getTitle(const SidTuneInfo *info, int track) const;


    IOController<Reader> m_reader;
    quint32 m_srate;
    bool m_aborted;
    double m_time;
    int m_length;
    quint8 m_chn;

    QList<QMPlay2Tag> m_tags;
    QString m_url, m_title;

    ReSIDfpBuilder m_rs;
    sidplayfp m_sidplay;
    SidTune *m_tune;
};

#define SIDPlayName "SIDPlay"
