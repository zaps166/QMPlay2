// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <InputsHeaders.hpp>

#include <Demuxer.hpp>

class ToneGenerator final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(ToneGenerator)
public:
    ToneGenerator(Module &);

    bool set() override;
private:
    bool metadataChanged() const override;

    QString name() const override;
    QString title() const override;
    double length() const override;
    int bitrate() const override;

    bool dontUseBuffer() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    /**/

    std::atomic_bool aborted;
    mutable std::atomic_bool metadata_changed;
    bool fromUrl;
    double pos;
    quint32 srate;
    QVector<quint32> freqs;
};

#define ToneGeneratorName "ToneGenerator"
