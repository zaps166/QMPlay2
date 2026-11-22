// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
