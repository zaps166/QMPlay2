// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ModuleCommon.hpp>

class QMPLAY2SHAREDLIB_EXPORT AudioFilter : public ModuleCommon
{
public:
    static QVector<AudioFilter *> open();

    virtual bool setAudioParameters(uchar chn, uint srate) = 0;
    virtual int bufferedSamples() const;
    virtual void clearBuffers();
    virtual double filter(QByteArray &data, bool flush = false) = 0; //returns delay in [s]
};
