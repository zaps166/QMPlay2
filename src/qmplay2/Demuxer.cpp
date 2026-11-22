// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Demuxer.hpp>

#include <Functions.hpp>
#include <Module.hpp>

bool Demuxer::create(const QString &url, IOController<Demuxer> &demuxer, FetchTracks *fetchTracks)
{
    const QString scheme = Functions::getUrlScheme(url);
    if (demuxer.isAborted() || url.isEmpty() || scheme.isEmpty())
        return false;
    const QString extension = Functions::fileExt(url).toLower();
    for (int i = 0; i <= 1; ++i)
        for (Module *module : QMPlay2Core.getPluginsInstance())
            for (const Module::Info &mod : module->getModulesInfo())
                if (mod.type == Module::DEMUXER && (mod.name == scheme || (!i ? mod.extensions.contains(extension) : mod.extensions.isEmpty())))
                {
                    if (!demuxer.assign((Demuxer *)module->createInstance(mod.name)))
                        continue;
                    bool canDoOpen = true;
                    if (fetchTracks)
                    {
                        fetchTracks->isOK = true;
                        fetchTracks->tracks = demuxer->fetchTracks(url, fetchTracks->isOK);
                        if (fetchTracks->isOK) //If tracks are fetched correctly (even if track list is empty)
                        {
                            if (!fetchTracks->tracks.isEmpty()) //Return tracks list
                            {
                                demuxer.reset();
                                return true;
                            }
                            if (fetchTracks->onlyTracks) //If there are no tracks and we want only track list - return false
                            {
                                demuxer.reset();
                                return false;
                            }
                        }
                        else //Tracks can't be fetched - an error occured
                        {
                            fetchTracks->tracks.clear(); //Clear if list is not empty
                            canDoOpen = false;
                        }
                    }
                    if (canDoOpen && demuxer->open(url))
                        return true;
                    demuxer.reset();
                    if (mod.name == scheme || demuxer.isAborted())
                        return false;
                }
    return false;
}

Demuxer::~Demuxer()
{
    for (StreamInfo *streamInfo : std::as_const(streams_info))
        delete streamInfo;
}

bool Demuxer::metadataChanged() const
{
    return false;
}

bool Demuxer::isStillImage() const
{
    return false;
}

QList<ProgramInfo> Demuxer::getPrograms() const
{
    return QList<ProgramInfo>();
}
QList<ChapterInfo> Demuxer::getChapters() const
{
    return QList<ChapterInfo>();
}

QList<QMPlay2Tag> Demuxer::tags() const
{
    return QList<QMPlay2Tag>();
}

qint64 Demuxer::size() const
{
    return -1;
}

QByteArray Demuxer::image(bool forceCopy) const
{
    Q_UNUSED(forceCopy)
    return QByteArray();
}

bool Demuxer::localStream() const
{
    return true;
}
bool Demuxer::dontUseBuffer() const
{
    return false;
}

void Demuxer::selectStreams(const QSet<int> &selectedStreams)
{
    Q_UNUSED(selectedStreams)
}

Playlist::Entries Demuxer::fetchTracks(const QString &url, bool &ok)
{
    Q_UNUSED(url)
    Q_UNUSED(ok)
    return Playlist::Entries();
}
