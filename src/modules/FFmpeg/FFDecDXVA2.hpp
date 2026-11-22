// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDecHWAccel.hpp>

class FFMPEG_EXPORT FFDecDXVA2 final : public FFDecHWAccel
{
public:
    FFDecDXVA2(Module &module);
    ~FFDecDXVA2();

    bool set() override;

    QString name() const override;

    std::shared_ptr<VideoFilter> hwAccelFilter() const override;

    bool open(StreamInfo &streamInfo) override;

private:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;
    std::shared_ptr<VideoFilter> m_filter;
};
