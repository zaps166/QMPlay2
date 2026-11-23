// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PulseAudioHeaders.hpp>

#include <Writer.hpp>
#include <Pulse.hpp>

class PulseAudioWriter final : public Writer
{
    Q_DECLARE_TR_FUNCTIONS(PulseAudioWriter)
public:
    PulseAudioWriter(Module &);
    ~PulseAudioWriter();
private:
    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    qint64 write(const QByteArray &) override;

    QString name() const override;

    bool open() override;

    /**/

    Pulse pulse;
    bool err;
};

#define PulseAudioWriterName "PulseAudio"
