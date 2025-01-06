/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <AudioFilter.hpp>

struct AVFilterContext;
struct AVFilterGraph;
struct AVFilterInOut;
struct AVFrame;

class AVAudioFilter final : public AudioFilter
{
public:
    static QStringList getAvailableFilters();

    static bool validateFilters(const QString &filters);

public:
    AVAudioFilter(Module &module);
    ~AVAudioFilter();

    bool set() override;

private:
    bool setAudioParameters(uchar chn, uint srate) override;
    int bufferedSamples() const override;
    void clearBuffers() override;
    double filter(QByteArray &data, bool flush) override;

private:
    inline void setCanFilter();

    bool ensureFilters();

    void destroyFilters();

private:
    bool m_enabled = false;
    bool m_hasParameters = false;
    bool m_canFilter = false;
    int m_chn = 0;
    int m_srate = 0;

    QByteArray m_filtersStr;

    AVFilterGraph *m_filterGraph = nullptr;

    AVFilterContext *m_filtIn = nullptr;
    AVFilterContext *m_filtOut = nullptr;

    AVFilterInOut *m_inputs = nullptr;
    AVFilterInOut *m_outputs = nullptr;

    AVFrame *m_frameIn = nullptr;
    AVFrame *m_frameOut = nullptr;

    qintptr m_overallSamples = 0;
    int m_bufferedSamples = 0;

    bool m_paramsChanged = false;
    bool m_initialized = false;
    bool m_flushed = false;
    bool m_error = false;
};

#define AVAudioFilterName "FFmpeg Audio Filters"
