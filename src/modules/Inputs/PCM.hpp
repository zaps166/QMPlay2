// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Demuxer.hpp>

#include <IOController.hpp>

class Reader;

class PCM final : public Demuxer
{
public:
    enum FORMAT {PCM_U8, PCM_S8, PCM_S16, PCM_S24, PCM_S32, PCM_FLT, FORMAT_COUNT};

    PCM(Module &);
private:
    bool set() override;

    QString name() const override;
    QString title() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    /**/

    IOController<Reader> reader;

    double len;
    FORMAT fmt;
    unsigned char chn;
    int srate, offset;
    bool bigEndian;
};

#define PCMName "PCM Audio"
