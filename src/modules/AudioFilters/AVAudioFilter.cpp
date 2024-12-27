/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <AVAudioFilter.hpp>

#include <QDebug>

extern "C"
{
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/opt.h>
}

QStringList AVAudioFilter::getAvailableFilters()
{
    QStringList filters;
    for (void *it = nullptr;;)
    {
        const auto filter = av_filter_iterate(&it);
        if (!filter)
            break;

        if (filter->flags & AVFILTER_FLAG_METADATA_ONLY)
            continue;

        if (filter->nb_inputs == 0)
            continue;

        if (filter->nb_inputs > 0 && avfilter_pad_get_type(filter->inputs, 0) != AVMEDIA_TYPE_AUDIO)
            continue;

        if (filter->nb_outputs > 0 && avfilter_pad_get_type(filter->outputs, 0) != AVMEDIA_TYPE_AUDIO)
            continue;

        const auto name = QString(filter->name);

        if (name.startsWith("anull") || name.startsWith("abuffer") || name == "join")
            continue;

        filters.push_back(name);
    }
    std::sort(filters.begin(), filters.end());
    return filters;
}

bool AVAudioFilter::validateFilters(const QString &filters)
{
    if (filters.isEmpty())
        return true;

    auto filterGraph = avfilter_graph_alloc();
    bool ret = (avfilter_graph_parse_ptr(filterGraph, filters.toLatin1().constData(), nullptr, nullptr, nullptr) == 0);
    avfilter_graph_free(&filterGraph);
    return ret;
}

AVAudioFilter::AVAudioFilter(Module &module)
{
    SetModule(module);
}
AVAudioFilter::~AVAudioFilter()
{
    destroyFilters();
}

bool AVAudioFilter::set()
{
    m_enabled = sets().getBool("AVAudioFilter");
    auto filtersStr = sets().getByteArray("AVAudioFilter/Filters").trimmed();
    if (m_filtersStr != filtersStr)
    {
        m_filtersStr = std::move(filtersStr);
        m_paramsChanged = true;
    }
    setCanFilter();
    return true;
}

bool AVAudioFilter::setAudioParameters(uchar chn, uint srate)
{
    m_hasParameters = (chn > 0 && srate > 0);
    if (m_hasParameters)
    {
        if (m_chn != static_cast<int>(chn) || m_srate != static_cast<int>(srate))
        {
            m_paramsChanged = true;
        }
        m_chn = chn;
        m_srate = srate;
    }
    setCanFilter();
    return m_hasParameters;
}
int AVAudioFilter::bufferedSamples() const
{
    return m_bufferedSamples;
}
void AVAudioFilter::clearBuffers()
{
    destroyFilters();
}
double AVAudioFilter::filter(QByteArray &data, bool flush)
{
    if (!m_canFilter)
    {
        if (m_initialized)
        {
            destroyFilters();
        }
        return 0.0;
    }

    if (!flush && m_flushed)
    {
        destroyFilters();
    }

    if (!ensureFilters())
    {
        return 0.0;
    }

    m_frameIn->data[0] = reinterpret_cast<uint8_t *>(data.data());
    m_frameIn->nb_samples = data.size() / m_chn / sizeof(float);

    double delay = 0.0;

    if (av_buffersrc_add_frame(m_filtIn, flush ? nullptr : m_frameIn) == 0)
    {
        if (av_buffersink_get_frame(m_filtOut, m_frameOut) == 0)
        {
            const int n = m_frameOut->nb_samples * m_chn * sizeof(float);
            m_bufferedSamples = (qintptr)m_frameIn->opaque - (qintptr)m_frameOut->opaque;
            delay = static_cast<double>(m_bufferedSamples) / static_cast<double>(m_srate);
            if (n == data.size())
            {
                memcpy(data.data(), m_frameOut->data[0], n);
            }
            else
            {
                data.clear();
                data.append(reinterpret_cast<const char *>(m_frameOut->data[0]), n);
            }
            av_frame_unref(m_frameOut);
        }
        else
        {
            data.clear();
            m_bufferedSamples = 0;
        }
        if (flush)
        {
            m_flushed = true;
        }
    }

    m_overallSamples += m_frameIn->nb_samples;

    m_frameIn->data[0] = nullptr;
    m_frameIn->opaque = reinterpret_cast<void *>(m_overallSamples);
    m_frameIn->nb_samples = 0;

    return delay;
}

inline void AVAudioFilter::setCanFilter()
{
    m_canFilter = m_enabled && m_hasParameters && !m_filtersStr.isEmpty();
}

bool AVAudioFilter::ensureFilters()
{
    if (m_paramsChanged)
    {
        destroyFilters();
    }

    if (m_initialized)
    {
        return true;
    }

    if (m_error)
    {
        return false;
    }

    auto init = [this] {
        const auto chnLayout = [this] {
            AVChannelLayout cl = {};
            cl.nb_channels = m_chn;
            int ret = av_channel_layout_describe(&cl, nullptr, 0);
            if (ret > 0)
            {
                QByteArray chnLayout(ret - 1, Qt::Uninitialized);
                if (av_channel_layout_describe(&cl, chnLayout.data(), ret) == ret)
                    return chnLayout;
            }
            return QByteArray();
        }();
        if (chnLayout.isEmpty())
        {
            qWarning() << "AVAudioFilter :: Unsupported channel count:" << m_chn;
            return false;
        }

        const auto args = QStringLiteral("sample_rate=%1:sample_fmt=flt:channel_layout=%2").arg(m_srate).arg(chnLayout.constData()).toLatin1();

        m_filterGraph = avfilter_graph_alloc();

        if (avfilter_graph_create_filter(&m_filtIn, avfilter_get_by_name("abuffer"), "in", args, nullptr, m_filterGraph) < 0)
        {
            qWarning() << "AVAudioFilter :: Can't create in filter";
            return false;
        }

        if (avfilter_graph_create_filter(&m_filtOut, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, m_filterGraph))
        {
            qWarning() << "AVAudioFilter :: Can't create out filter";
            return false;
        }

        const AVSampleFormat sampleFmts[] = {AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_NONE};
        av_opt_set_int_list(m_filtOut, "sample_fmts", sampleFmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

        const int sampleRates[] = {m_srate, 0};
        av_opt_set_int_list(m_filtOut, "sample_rates", sampleRates, 0, AV_OPT_SEARCH_CHILDREN);

        av_opt_set(m_filtOut, "ch_layouts", chnLayout.constData(), AV_OPT_SEARCH_CHILDREN);

        m_inputs = avfilter_inout_alloc();
        m_inputs->name = av_strdup("out");
        m_inputs->filter_ctx = m_filtOut;

        m_outputs = avfilter_inout_alloc();
        m_outputs->name = av_strdup("in");
        m_outputs->filter_ctx = m_filtIn;

        if (avfilter_graph_parse_ptr(m_filterGraph, m_filtersStr.constData(), &m_inputs, &m_outputs, nullptr) < 0)
        {
            qWarning() << "AVAudioFilter :: Can't parse filter string:" << m_filtersStr;
            return false;
        }

        if (avfilter_graph_config(m_filterGraph, nullptr) < 0)
        {
            qWarning() << "AVAudioFilter :: Can't configure filter graph";
            return false;
        }

        m_frameIn = av_frame_alloc();
        m_frameIn->format = AV_SAMPLE_FMT_FLT;
        m_frameIn->sample_rate = m_srate;
        m_frameIn->ch_layout.nb_channels = m_chn;

        m_frameOut = av_frame_alloc();

        return true;
    };

    if (!init())
    {
        destroyFilters();
        m_error = true;
        return false;
    }

    m_initialized = true;
    m_error = false;
    return true;
}

void AVAudioFilter::destroyFilters()
{
    if (m_frameOut)
        av_frame_free(&m_frameOut);
    if (m_frameIn)
        av_frame_free(&m_frameIn);

    if (m_filterGraph)
        avfilter_graph_free(&m_filterGraph);

    m_inputs = nullptr;
    m_outputs = nullptr;

    m_filtIn = nullptr;
    m_filtOut = nullptr;

    m_overallSamples = 0;
    m_bufferedSamples = 0;

    m_paramsChanged = false;
    m_initialized = false;
    m_flushed = false;
    m_error = false;
}
