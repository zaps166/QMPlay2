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

#include <IOController.hpp>

namespace QMPlay2ModPlug {
    struct File;
}
class Reader;

class MPDemux final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(MPDemux)
public:
    MPDemux(Module &);
private:
    ~MPDemux();

    bool set() override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    /**/

    bool aborted;
    double pos;
    quint32 srate;
    QMPlay2ModPlug::File *mpfile;
    IOController<Reader> reader;
};

#define DemuxerName "Modplug Demuxer"
