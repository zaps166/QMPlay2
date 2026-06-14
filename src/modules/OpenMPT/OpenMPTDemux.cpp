/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#include <OpenMPTDemux.hpp>

#include <Functions.hpp>
#include <OpenMPTVisPatterns.hpp>
#include <OpenMPTVisSamples.hpp>
#include <Packet.hpp>
#include <Reader.hpp>

#include <libopenmpt/libopenmpt.hpp>

#include <algorithm>

OpenMPDemux::OpenMPDemux(Module &module)
    : m_sampleRate(Functions::getBestSampleRate())
{
    SetModule(module);
}
OpenMPDemux::~OpenMPDemux()
{
}

bool OpenMPDemux::set()
{
    const auto prevVisMode = m_visMode;
    const int prevVisChannels = m_visChannels;
    const int prevVisRows = m_visRows;
    const int prevStereoSeparation = m_stereoSeparation;
    const int prevInterpolationFilter = m_interpolationFilter;
    const auto prevSubsongMode = m_subsongMode;
    m_visMode = static_cast<OpenMPTVisBase::VisMode>(sets().getInt("VisMode"));
    m_visChannels = sets().getInt("VisChannels");
    m_visRows = sets().getInt("VisRows");
    m_stereoSeparation = sets().getInt("StereoSeparation");
    m_interpolationFilter = sets().getInt("InterpolationFilter");
    m_subsongMode = static_cast<SubsongMode>(sets().getInt("SubsongMode"));
    if (m_visMode != prevVisMode
            || m_visChannels != prevVisChannels
            || m_visRows != prevVisRows
            || m_stereoSeparation != prevStereoSeparation
            || m_interpolationFilter != prevInterpolationFilter
            || m_subsongMode != prevSubsongMode)
    {
        return false;
    }
    return !m_aborted && sets().getBool("Enabled");
}

QString OpenMPDemux::name() const
{
    return QString::fromStdString(m_module->get_metadata("type_long"));
}
QString OpenMPDemux::title() const
{
    return m_title;
}
QList<QMPlay2Tag> OpenMPDemux::tags() const
{
    return m_tags;
}
double OpenMPDemux::length() const
{
    return m_module->get_duration_seconds();
}
int OpenMPDemux::bitrate() const
{
    if (m_overallLength <= 0.0)
        return -1;
    return qRound(m_fileSize * 8.0 / m_overallLength / 1000.0);
}

bool OpenMPDemux::dontUseBuffer() const
{
    return m_visMode != OpenMPTVisBase::VisMode::Off;
}

bool OpenMPDemux::seek(double pos, bool)
{
    pos = std::clamp(pos, 0.0, m_overallLength);

    if (m_module->get_selected_subsong() < 0)
    {
        // Workaround for libopenmpt bug

        const double curPos = m_module->get_position_seconds();
        if (qFuzzyCompare(pos, curPos))
            return true;

        if (pos < curPos)
            m_module->set_position_seconds(0.0);

        constexpr int kBufSize = 1024;
        std::int16_t buf[kBufSize];
        while (!m_aborted && m_module->get_position_seconds() < pos)
        {
            if (m_module->read(m_sampleRate, kBufSize, buf) <= 0)
            {
                break;
            }
        }
    }
    else
    {
        m_module->set_position_seconds(pos);
    }

    m_pendingVideoPacket.clear();
    m_lastAudioDuration = 0.0;
    if (m_visualizer)
        m_visualizer->reset();
    return true;
}
bool OpenMPDemux::read(Packet &decoded, int &idx)
{
    if (m_aborted)
        return false;

    if (m_visualizer)
    {
        if (Frame visFrame = m_visualizer->render(m_module.get()); !visFrame.isEmpty())
        {
            Packet videoPacket;
            videoPacket.setFrame(std::move(visFrame));
            videoPacket.setTS(m_module->get_position_seconds());
            if (!m_pendingVideoPacket.isEmpty())
            {
                m_pendingVideoPacket.setDuration(qMax(0.0, videoPacket.ts() - m_pendingVideoPacket.ts()));
                decoded = std::move(m_pendingVideoPacket);
                m_pendingVideoPacket = std::move(videoPacket);
                idx = 1;
                return true;
            }
            m_pendingVideoPacket = std::move(videoPacket);
        }
    }

    constexpr int samples = 1024;
    constexpr int bufferSize = samples * sizeof(float) * 2;
    decoded.resize(bufferSize);

    int framesRendered = m_module->read_interleaved_stereo(m_sampleRate, samples, reinterpret_cast<float *>(decoded.data()));
    if (framesRendered <= 0)
    {
        if (!m_pendingVideoPacket.isEmpty())
        {
            m_pendingVideoPacket.setDuration(m_lastAudioDuration);
            decoded = std::move(m_pendingVideoPacket);
            m_pendingVideoPacket.clear();
            idx = 1;
            return true;
        }
        return false;
    }

    decoded.resize(framesRendered * sizeof(float) * 2);

    m_lastAudioDuration = static_cast<double>(framesRendered) / m_sampleRate;
    decoded.setTS(m_module->get_position_seconds());
    decoded.setDuration(m_lastAudioDuration);
    idx = 0;

    return true;
}
void OpenMPDemux::abort()
{
    m_reader.abort();
    m_aborted = true;
}

Playlist::Entries OpenMPDemux::fetchTracks(const QString &url, bool &ok)
{
    Playlist::Entries entries;

    if (m_subsongMode != SubsongMode::Group)
    {
        ok = true;
        return entries;
    }

    openModule(url);
    if (!m_module)
    {
        ok = false;
        return entries;
    }

    const int numSubsongs = m_module->get_num_subsongs();
    if (numSubsongs <= 1)
    {
        ok = true;
        return entries;
    }

    const auto subsongNames = m_module->get_subsong_names();
    for (int i = 0; i < numSubsongs; ++i)
    {
        m_module->select_subsong(i);

        Playlist::Entry entry;
        entry.url = QString::fromLatin1(DemuxerName) + QStringLiteral("://") + QChar('{') + url + QStringLiteral("}") + QString::number(i);
        entry.name = formatSubsongTitle(i, subsongNames, m_moduleTitle).first;
        entry.length = m_module->get_duration_seconds();

        entries.append(entry);
    }

    for (int i = 0; i < entries.count(); ++i)
        entries[i].parent = 1;

    Playlist::Entry groupEntry;
    groupEntry.name = m_moduleTitle;
    groupEntry.url = url;
    groupEntry.GID = 1;
    entries.prepend(groupEntry);

    ok = true;
    return entries;
}

bool OpenMPDemux::open(const QString &urlArg)
{
    QString prefix, url, param;
    const bool hasPluginPrefix = Functions::splitPrefixAndUrlIfHasPluginPrefix(urlArg, &prefix, &url, &param);
    if (hasPluginPrefix)
    {
        if (prefix != DemuxerName)
            return false;
    }
    else
    {
        if (url.startsWith(DemuxerName "://"))
            return false;
        url = urlArg;
    }

    openModule(url);
    if (!m_module)
        return false;

    const int numSubsongs = m_module->get_num_subsongs();
    QString subsongOrModuleTitle;

    if (hasPluginPrefix)
    {
        bool ok = false;
        const int subsong = param.toInt(&ok);
        if (!ok || subsong < 0 || subsong >= numSubsongs)
            return false;
        m_module->select_subsong(subsong);
        std::tie(m_title, subsongOrModuleTitle) = formatSubsongTitle(subsong, m_module->get_subsong_names(), m_moduleTitle);
    }
    else
    {
        if (m_subsongMode == SubsongMode::OnlyFirst)
        {
            m_module->select_subsong(0);
        }
        m_title = subsongOrModuleTitle = m_moduleTitle;
    }

    m_module->set_render_param(openmpt::module::RENDER_STEREOSEPARATION_PERCENT, m_stereoSeparation);
    m_module->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, m_interpolationFilter);

    {
        m_tags += {QString::number(QMPLAY2_TAG_TITLE), subsongOrModuleTitle};

        if (const auto tagArtist = QString::fromStdString(m_module->get_metadata("artist")).trimmed(); !tagArtist.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_ARTIST), tagArtist};

        if (const auto tagAlbum = QString::fromStdString(m_module->get_metadata("album")).trimmed(); !tagAlbum.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_ALBUM), tagAlbum};

        if (const auto tagDate = QString::fromStdString(m_module->get_metadata("date")).trimmed(); !tagDate.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_DATE), tagDate};

        m_tags += {tr("Samples"), QString::number(m_module->get_num_samples())};
        m_tags += {tr("Patterns"), QString::number(m_module->get_num_patterns())};
        m_tags += {tr("Channels"), QString::number(m_module->get_num_channels())};
        if (const auto nInstr = m_module->get_num_instruments(); nInstr > 0)
            m_tags += {tr("Instruments"), QString::number(nInstr)};

        if (numSubsongs > 1)
        {
            if (const int curSubsong = m_module->get_selected_subsong(); curSubsong >= 0)
                m_tags += {tr("Track"), QString::number(curSubsong + 1)};
        }
    }

    streams_info += new StreamInfo(m_sampleRate, 2);

    // Video stream for visualization
    QByteArray videoStreamTitle;
    if (m_visMode == OpenMPTVisBase::VisMode::Patterns)
    {
        m_visualizer = std::make_unique<OpenMPTVisPatterns>(m_module.get(), m_visChannels, m_visRows);
        videoStreamTitle = "Patterns visualization";
    }
    else if (m_visMode == OpenMPTVisBase::VisMode::Samples)
    {
        m_visualizer = std::make_unique<OpenMPTVisSamples>(m_module.get());
        videoStreamTitle = "Samples visualization";
    }
    if (m_visualizer)
    {
        const int w = m_visualizer->width();
        const int h = m_visualizer->height();
        if (w > 0 && h > 0)
        {
            auto *videoStreamInfo = new StreamInfo;
            videoStreamInfo->params->codec_type = AVMEDIA_TYPE_VIDEO;
            videoStreamInfo->params->width = w;
            videoStreamInfo->params->height = h;
            videoStreamInfo->params->format = AV_PIX_FMT_BGRA;
            videoStreamInfo->title = videoStreamTitle;
            streams_info += videoStreamInfo;
        }
        else
        {
            m_visualizer->reset();
        }
    }

    return true;
}

void OpenMPDemux::openModule(const QString &url)
{
    m_module.reset();

    if (!Reader::create(url, m_reader))
        return;

    m_fileSize = m_reader->size();
    if (m_fileSize > 0)
    {
        try
        {
            m_module = std::make_unique<openmpt::module>(m_reader->read(m_fileSize).constData(), static_cast<std::size_t>(m_fileSize));
        }
        catch (const openmpt::exception &e)
        {
            Q_UNUSED(e);
        }
    }
    m_reader.reset();

    if (m_module)
    {
        if (m_module->get_num_subsongs() > 1)
        {
            // "SubsongMode::AllSequentially" by default - we want to obtain full duration; it will be overridden later if needed
            m_module->select_subsong(-1);
        }
        m_overallLength = m_module->get_duration_seconds();
        m_moduleTitle = QString::fromStdString(m_module->get_metadata("title")).trimmed();
        if (m_moduleTitle.isEmpty())
            m_moduleTitle = Functions::fileName(url, false);
    }
}

std::pair<QString, QString> OpenMPDemux::formatSubsongTitle(int subsong, const std::vector<std::string> &subsongNames, const QString &rawTitle) const
{
    QString subsongTitle;
    if (subsong < static_cast<int>(subsongNames.size()))
        subsongTitle = QString::fromStdString(subsongNames[subsong]).trimmed();
    if (subsongTitle.isEmpty())
        subsongTitle = rawTitle;
    return {tr("Track %1").arg(subsong + 1, 2, 10, QChar('0')) + QStringLiteral(" - ") + subsongTitle, subsongTitle};
}
