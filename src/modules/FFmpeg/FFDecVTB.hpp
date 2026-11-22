// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDecHWAccel.hpp>

class FFMPEG_EXPORT FFDecVTB final : public FFDecHWAccel
{
public:
    FFDecVTB(Module &module);
    ~FFDecVTB();

    bool set() override;

    QString name() const override;

    bool open(StreamInfo &streamInfo) override;

private:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;
};
