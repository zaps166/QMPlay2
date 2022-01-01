/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <SIDPlay.hpp>
#include <Common.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Packet.hpp>

#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidInfo.h>

SIDPlay::SIDPlay(Module &module) :
    m_srate(Functions::getBestSampleRate()),
    m_aborted(false),
    m_time(-1.0),
    m_rs(nullptr),
    m_tune(nullptr)
{
    SetModule(module);
}
SIDPlay::~SIDPlay()
{
    delete m_tune;
}

bool SIDPlay::set()
{
    m_length = sets().getInt("DefaultLength");
    return sets().getBool("SIDPlay");
}

QString SIDPlay::name() const
{
    return m_tune->getInfo()->formatString();
}
QString SIDPlay::title() const
{
    return m_title;
}
QList<QMPlay2Tag> SIDPlay::tags() const
{
    return m_tags;
}
double SIDPlay::length() const
{
    return m_length;
}
int SIDPlay::bitrate() const
{
    return -1;
}

bool SIDPlay::seek(double s, bool backward)
{
    m_time = -1.0;

    if (backward && !m_sidplay.load(m_tune)) //backward
        return false;

    if (s > 0.0)
    {
        const int pos = s;
        const int bufferSize = m_chn * m_srate;
        qint16 *buff1sec = new qint16[bufferSize];
        for (int i = m_sidplay.time(); i <= pos && !m_aborted; ++i)
            m_sidplay.play(buff1sec, bufferSize);
        delete[] buff1sec;
    }

    return true;
}
bool SIDPlay::read(Packet &decoded, int &idx)
{
    if (m_aborted)
        return false;

    if (m_time < 0.0)
        m_time = m_sidplay.time();

    if (m_time > m_length)
        return false;

    const int chunkSize = 1024 * m_chn;

    decoded.resize(chunkSize * sizeof(float));

    int16_t *srcData = (int16_t *)decoded.data();
    float *dstData = (float *)decoded.data();

    m_sidplay.play(srcData, chunkSize);

    for (int i = chunkSize - 1; i >= 0; --i)
        dstData[i] = srcData[i] / 16384.0;

    const double fadePos = m_time - (m_length - 5);
    if (fadePos >= 0)
        ChiptuneCommon::doFadeOut(dstData, chunkSize, m_chn, m_srate, fadePos, 5.0);

    decoded.setTS(m_time);
    decoded.setDuration(chunkSize / m_chn / (double)m_srate);

    idx = 0;

    m_time += decoded.duration();

    return true;
}
void SIDPlay::abort()
{
    m_reader.abort();
    m_aborted = true;
}

bool SIDPlay::open(const QString &url)
{
    return open(url, false);
}

Playlist::Entries SIDPlay::fetchTracks(const QString &url, bool &ok)
{
    Playlist::Entries entries;
    if (open(url, true))
    {
        const int tracks = m_tune->getInfo()->songs();
        for (int i = 0; i < tracks; ++i)
        {
            const SidTuneInfo *info = m_tune->getInfo(i);
            if (info)
            {
                Playlist::Entry entry;
                entry.url = SIDPlayName + QString("://{%1}%2").arg(m_url).arg(i);
                entry.name = getTitle(info, i);
                entry.length = m_length;
                entries.append(entry);
            }
        }
        if (entries.count() > 1)
        {
            for (int i = 0; i < entries.count(); ++i)
                entries[i].parent = 1;
            Playlist::Entry entry;
            entry.name = Functions::fileName(m_url, false);
            entry.url = m_url;
            entry.GID = 1;
            entries.prepend(entry);
        }
    }
    ok = !entries.isEmpty();
    return entries;
}


bool SIDPlay::open(const QString &_url, bool tracksOnly)
{
    QString prefix, url, param;
    const bool hasPluginPrefix = Functions::splitPrefixAndUrlIfHasPluginPrefix(_url, &prefix, &url, &param);

    if (tracksOnly == hasPluginPrefix)
        return false;

    int track = 0;
    if (!hasPluginPrefix)
    {
        if (url.startsWith(SIDPlayName "://"))
            return false;
        url = _url;
    }
    else
    {
        if (prefix != SIDPlayName)
            return false;
        bool ok;
        track = param.toInt(&ok);
        if (track < 0 || !ok)
            return false;
    }

    if (Reader::create(url, m_reader))
    {
        const QByteArray data = m_reader->read(m_reader->size());
        m_reader.reset();

        m_tune = new SidTune((const quint8 *)data.constData(), data.length());
        if (!m_tune->getStatus())
            return false;

        if (!hasPluginPrefix)
        {
            m_aborted = true;
            m_url = url;
            return true;
        }

        const SidTuneInfo *info = m_tune->getInfo();

        if (track >= (int)info->songs())
            return false;

        m_rs.create(m_sidplay.info().maxsids());
        if (!m_rs.getStatus())
            return false;
        m_rs.filter(true);

#if ((LIBSIDPLAYFP_VERSION_MAJ << 16 | LIBSIDPLAYFP_VERSION_MIN << 8 | LIBSIDPLAYFP_VERSION_LEV) > 0x010800)
        const bool isStereo = info->sidChips() > 1 ? true : false;
#else
        const bool isStereo = info->isStereo();
#endif

        SidConfig cfg;
        cfg.frequency = m_srate;
        cfg.sidEmulation = &m_rs;
        if (isStereo)
            cfg.playback = SidConfig::STEREO;
        cfg.samplingMethod = SidConfig::INTERPOLATE;
        if (!m_sidplay.config(cfg))
            return false;

        m_tune->selectSong(track + 1);

        m_title = getTitle(info, track);
        m_chn = isStereo ? 2 : 1;

        const QString title    = info->infoString(0);
        const QString author   = info->infoString(1);
        const QString released = info->infoString(2);
        if (!title.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_TITLE), title};
        if (!author.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_ARTIST), author};
        if (!released.isEmpty())
            m_tags += {QString::number(QMPLAY2_TAG_DATE), released};
        m_tags += {tr("Track"), QString::number(track + 1)};

        streams_info += new StreamInfo(m_srate, m_chn);

        return m_sidplay.load(m_tune);
    }

    return false;
}

QString SIDPlay::getTitle(const SidTuneInfo *info, int track) const
{
    const QString title  = info->infoString(0);
    const QString author = info->infoString(1);
    QString ret;
    if (!author.isEmpty() && !title.isEmpty())
        ret = author + " - " + title;
    else
        ret = title;
    if (info->songs() > 1)
        return tr("Track") + QString(" %1%2").arg(track + 1).arg(ret.isEmpty() ? QString() : (" - " + ret));
    return ret;
}
