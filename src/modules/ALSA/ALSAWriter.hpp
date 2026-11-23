// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ALSACommon.hpp>
#include <Writer.hpp>

class ALSAWriter final : public Writer
{
    Q_DECLARE_TR_FUNCTIONS(ALSAWriter)
public:
    ALSAWriter(Module &);
private:
    ~ALSAWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    qint64 write(const QByteArray &) override;
    void pause() override;

    QString name() const override;

    bool open() override;

    /**/

    void close();

    QString devName;

    QByteArray int_samples;
    unsigned sample_size;
    _snd_pcm *snd;

    double delay;
    unsigned sample_rate, channels;
    bool autoFindMultichannelDevice, err, mustSwapChn, canPause;
};

#define ALSAWriterName "ALSA"
