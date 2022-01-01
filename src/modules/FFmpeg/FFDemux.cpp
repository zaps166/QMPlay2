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

#include <FFDemux.hpp>
#include <FFCommon.hpp>
#include <FormatContext.hpp>

#include <Functions.hpp>
#include <OggHelper.hpp>
#include <Packet.hpp>

#include <QFile>

FFDemux::FFDemux(Module &module) :
    abortFetchTracks(false),
    reconnectStreamed(false)
{
    SetModule(module);
}
FFDemux::~FFDemux()
{
    streams_info.clear();
    for (const FormatContext *fmtCtx : qAsConst(formatContexts))
        delete fmtCtx;
}

bool FFDemux::set()
{
    bool restartPlayback = false;

    const bool tmpReconnectStreamed = sets().getBool("ReconnectStreamed");
    if (tmpReconnectStreamed != reconnectStreamed)
    {
        reconnectStreamed = tmpReconnectStreamed;
        restartPlayback = true;
    }

    return sets().getBool("DemuxerEnabled") && !restartPlayback;
}

bool FFDemux::metadataChanged() const
{
    bool isMetadataChanged = false;
    for (const FormatContext *fmtCtx : formatContexts)
        isMetadataChanged |= fmtCtx->metadataChanged();
    return isMetadataChanged;
}

bool FFDemux::isStillImage() const
{
    bool stillImage = true;
    for (const FormatContext *fmtCtx : formatContexts)
        stillImage &= fmtCtx->isStillImage();
    return stillImage;
}

QList<ProgramInfo> FFDemux::getPrograms() const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->getPrograms();
    return QList<ProgramInfo>();
}
QList<ChapterInfo> FFDemux::getChapters() const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->getChapters();
    return QList<ChapterInfo>();
}

QString FFDemux::name() const
{
    QString name;
    for (const FormatContext *fmtCtx : formatContexts)
    {
        const QString fmtCtxName = fmtCtx->name();
        if (!name.contains(fmtCtxName))
            name += fmtCtxName + ";";
    }
    name.chop(1);
    return name;
}
QString FFDemux::title() const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->title();
    return QString();
}
QList<QMPlay2Tag> FFDemux::tags() const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->tags();
    return QList<QMPlay2Tag>();
}
bool FFDemux::getReplayGain(bool album, float &gain_db, float &peak) const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->getReplayGain(album, gain_db, peak);
    return false;
}
qint64 FFDemux::size() const
{
    qint64 bytes = -1;
    for (const FormatContext *fmtCtx : formatContexts)
    {
        const qint64 s = fmtCtx->size();
        if (s < 0)
            return -1;
        bytes += s;
    }
    return bytes;
}
double FFDemux::length() const
{
    double length = -1.0;
    for (const FormatContext *fmtCtx : formatContexts)
        length = qMax(length, fmtCtx->length());
    return length;
}
int FFDemux::bitrate() const
{
    int bitrate = 0;
    for (const FormatContext *fmtCtx : formatContexts)
        bitrate += fmtCtx->bitrate();
    return bitrate;
}
QByteArray FFDemux::image(bool forceCopy) const
{
    if (formatContexts.count() == 1)
        return formatContexts.at(0)->image(forceCopy);
    return QByteArray();
}

bool FFDemux::localStream() const
{
    for (const FormatContext *fmtCtx : formatContexts)
        if (!fmtCtx->isLocal)
            return false;
    return true;
}

bool FFDemux::seek(double pos, bool backward)
{
    bool seeked = false;
    for (FormatContext *fmtCtx : qAsConst(formatContexts))
    {
        if (fmtCtx->seek(pos, backward))
            seeked |= true;
        else if (fmtCtx->isStreamed && formatContexts.count() > 1)
        {
            fmtCtx->setStreamOffset(pos);
            seeked |= true;
        }
    }
    return seeked;
}
bool FFDemux::read(Packet &encoded, int &idx)
{
    int fmtCtxIdx = -1;
    int numErrors = 0;

    double ts;
    for (int i = 0; i < formatContexts.count(); ++i)
    {
        FormatContext *fmtCtx = formatContexts.at(i);
        if (fmtCtx->isError)
        {
            ++numErrors;
            continue;
        }
        if (fmtCtxIdx < 0 || fmtCtx->currPos < ts)
        {
            ts = fmtCtx->currPos;
            fmtCtxIdx = i;
        }
    }

    if (fmtCtxIdx < 0) //Every format context has an error
        return false;

    if (formatContexts.at(fmtCtxIdx)->read(encoded, idx))
    {
        for (int i = 0; i < fmtCtxIdx; ++i)
            idx += formatContexts.at(i)->streamsInfo.count();
        return true;
    }

    return numErrors < formatContexts.count() - 1; //Not Every format context has an error
}
void FFDemux::pause()
{
    for (FormatContext *fmtCtx : qAsConst(formatContexts))
        fmtCtx->pause();
}
void FFDemux::abort()
{
    QMutexLocker mL(&mutex);
    for (FormatContext *fmtCtx : qAsConst(formatContexts))
        fmtCtx->abort();
    abortFetchTracks = true;
}

bool FFDemux::open(const QString &entireUrl)
{
    QString prefix, url, param;
    if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(entireUrl, &prefix, &url, &param))
        addFormatContext(entireUrl);
    else if (prefix == DemuxerName)
    {
        if (!param.isEmpty())
            addFormatContext(url, param);
        else for (QString stream : url.split("][", QString::SkipEmptyParts))
        {
            stream.remove('[');
            stream.remove(']');
            addFormatContext(stream);
            if (abortFetchTracks)
                break;
        }
    }
    return !formatContexts.isEmpty();
}

Playlist::Entries FFDemux::fetchTracks(const QString &url, bool &ok)
{
    if (url.contains("://{") || !url.startsWith("file://"))
        return {};

    const auto createFmtCtx = [&] {
        FormatContext *fmtCtx = new FormatContext;
        {
            QMutexLocker mL(&mutex);
            formatContexts.append(fmtCtx);
        }
        return fmtCtx;
    };
    const auto destroyFmtCtx = [&](FormatContext *fmtCtx) {
        {
            QMutexLocker mL(&mutex);
            const int idx = formatContexts.indexOf(fmtCtx);
            if (idx > -1)
                formatContexts.remove(idx);
        }
        delete fmtCtx;
    };

    if (url.endsWith(".cue", Qt::CaseInsensitive))
    {
        QFile cue(url.mid(7));
        if (cue.size() <= 0xFFFF && cue.open(QFile::ReadOnly | QFile::Text))
        {
            QList<QByteArray> data = Functions::textWithFallbackEncoding(cue.readAll()).split('\n');
            QString title, performer, audioUrl;
            double index0 = -1.0, index1 = -1.0;
            QHash<int, QPair<double, double>> indexes;
            Playlist::Entries entries;
            Playlist::Entry entry;
            int track = -1;
            const auto maybeFlushTrack = [&](int prevTrack) {
                if (track <= 0)
                    return;
                const auto cutFromQuotation = [](QString &str) {
                    const int idx1 = str.indexOf('"');
                    const int idx2 = str.lastIndexOf('"');
                    if (idx1 > -1 && idx2 > idx1)
                        str = str.mid(idx1 + 1, idx2 - idx1 - 1);
                    else
                        str.clear();
                };
                cutFromQuotation(title);
                cutFromQuotation(performer);
                if (!title.isEmpty() && !performer.isEmpty())
                    entry.name = performer + " - " + title;
                else if (title.isEmpty() && !performer.isEmpty())
                    entry.name = performer;
                else if (!title.isEmpty() && performer.isEmpty())
                    entry.name = title;
                if (entry.name.isEmpty())
                {
                    if (entry.parent == 1)
                        entry.name = tr("Track") + " " + QString::number(prevTrack);
                    else
                        entry.name = Functions::fileName(entry.url, false);
                }
                title.clear();
                performer.clear();
                if (prevTrack > 0)
                {
                    if (index0 <= 0.0)
                        index0 = index1; // "INDEX 00" doesn't exist, use "INDEX 01"
                    indexes[prevTrack].first = index0;
                    indexes[prevTrack].second = index1;
                    if (entries.count() <= prevTrack)
                        entries.resize(prevTrack + 1);
                    entries[prevTrack] = entry;
                }
                else
                {
                    entries.prepend(entry);
                }
                entry = Playlist::Entry();
                entry.parent = 1;
                entry.url = audioUrl;
                index0 = index1 = -1.0;
            };
            const auto parseTime = [](const QByteArray &time) {
                int m = 0, s = 0, f = 0;
                if (sscanf(time.constData(), "%2d:%2d:%2d", &m, &s, &f) == 3)
                    return m * 60.0 + s + f / 75.0;
                return -1.0;
            };
            entry.url = url;
            entry.GID = 1;
            for (QByteArray &line : data)
            {
                line = line.trimmed();
                if (track < 0)
                {
                    if (line.startsWith("TITLE "))
                        title = line;
                    else if (line.startsWith("PERFORMER "))
                        performer = line;
                    else if (line.startsWith("FILE "))
                    {
                        const int idx = line.lastIndexOf('"');
                        if (idx > -1)
                        {
                            audioUrl = line.mid(6, idx - 6);
                            if (!audioUrl.isEmpty())
                            {
                                audioUrl.prepend(Functions::filePath(url.mid(7)));
                                if (!QFile::exists(audioUrl))
                                {
                                    const QStringList knownExts {"wav", "WAV", "flac", "FLAC", "alac", "ALAC", "ape", "APE"};
                                    const auto ext = Functions::fileExt(audioUrl);
                                    if (!ext.isEmpty() && knownExts.contains(ext))
                                    {
                                        for (auto &&knownExt : knownExts)
                                        {
                                            if (knownExt == ext)
                                                continue;

                                            const QString tmpAudioUrl = audioUrl.mid(0, audioUrl.length() - ext.length()) + knownExt;
                                            if (QFile::exists(tmpAudioUrl))
                                            {
                                                audioUrl = tmpAudioUrl;
                                                break;
                                            }
                                        }
                                    }
                                }
                                audioUrl.prepend("file://");
                            }
                        }
                    }
                }
                else if (line.startsWith("FILE "))
                {
                    // QMPlay2 supports CUE files which uses only single audio file
                    return {};
                }
                if (line.startsWith("TRACK "))
                {
                    if (entries.isEmpty() && audioUrl.isEmpty())
                        break;
                    if (line.endsWith(" AUDIO"))
                    {
                        const int prevTrack = track;
                        track = line.mid(6, 2).toInt();
                        if (track < 99)
                            maybeFlushTrack(prevTrack);
                        else
                            track = 0;
                    }
                    else
                    {
                        track = 0;
                    }
                }
                else if (track > 0)
                {
                    if (line.startsWith("TITLE "))
                        title = line;
                    else if (line.startsWith("PERFORMER "))
                        performer = line;
                    else if (line.startsWith("INDEX 00 "))
                        index0 = parseTime(line.mid(9));
                    else if (line.startsWith("INDEX 01 "))
                        index1 = parseTime(line.mid(9));
                }
            }
            maybeFlushTrack(track);
            for (int i = entries.count() - 1; i >= 1; --i)
            {
                Playlist::Entry &entry = entries[i];
                const bool lastItem = (i == entries.count() - 1);
                const double start = indexes.value(i).second;
                const double end = indexes.value(i + 1, {-1.0, -1.0}).first;
                if (entry.url.isEmpty() || start < 0.0 || (end <= 0.0 && !lastItem))
                {
                    entries.removeAt(i);
                    continue;
                }
                const QString param = QString("CUE:%1:%2").arg(start).arg(end);
                if (lastItem && end < 0.0) // Last entry doesn't have specified length in CUE file
                {
                    FormatContext *fmtCtx = createFmtCtx();
                    if (fmtCtx->open(entry.url, param))
                        entry.length = fmtCtx->length();
                    destroyFmtCtx(fmtCtx);
                    if (abortFetchTracks)
                    {
                        ok = false;
                        return {};
                    }
                }
                else
                {
                    entry.length = end - start;
                }
                entry.url = QString("%1://{%2}%3").arg(DemuxerName, entry.url, param);
            }
            if (!entries.isEmpty())
                return entries;
        }
    }

    OggHelper oggHelper(url.mid(7), abortFetchTracks);
    if (oggHelper.io)
    {
        Playlist::Entries entries;
        int i = 0;
        for (const OggHelper::Chain &chains : oggHelper.getOggChains(ok))
        {
            const QString param = QString("OGG:%1:%2:%3").arg(++i).arg(chains.first).arg(chains.second);

            FormatContext *fmtCtx = createFmtCtx();
            if (fmtCtx->open(url, param))
            {
                Playlist::Entry entry;
                entry.url = QString("%1://{%2}%3").arg(DemuxerName, url, param);
                entry.name = fmtCtx->title();
                entry.length = fmtCtx->length();
                entries.append(entry);
            }
            destroyFmtCtx(fmtCtx);
            if (abortFetchTracks)
            {
                ok = false;
                return {};
            }
        }
        if (ok && !entries.isEmpty())
        {
            for (int i = 0; i < entries.count(); ++i)
                entries[i].parent = 1;
            Playlist::Entry entry;
            entry.name = Functions::fileName(url, false);
            entry.url = url;
            entry.GID = 1;
            entries.prepend(entry);
            return entries;
        }
    }

    return {};
}

void FFDemux::addFormatContext(QString url, const QString &param)
{
    FormatContext *fmtCtx = new FormatContext(reconnectStreamed);
    {
        QMutexLocker mL(&mutex);
        formatContexts.append(fmtCtx);
    }
    if (!url.contains("://"))
        url.prepend("file://");
    if (fmtCtx->open(url, param))
    {
        streams_info.append(fmtCtx->streamsInfo);
    }
    else
    {
        {
            QMutexLocker mL(&mutex);
            formatContexts.erase(formatContexts.end() - 1);
        }
        delete fmtCtx;
    }
}
