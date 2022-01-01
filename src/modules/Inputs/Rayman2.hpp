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

class Reader;

class Rayman2 final : public Demuxer
{
public:
    Rayman2(Module &);
private:
    bool set() override;

    QString name() const override;
    QString title() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double pos, bool backward) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    /**/

    void readHeader(const char *data);

    IOController<Reader> reader;

    double len;
    unsigned srate;
    unsigned short chn;
    int predictor[2];
    short stepIndex[2];
};

#define Rayman2Name "Rayman2 Audio"
