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
    for (StreamInfo *streamInfo : qAsConst(streams_info))
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

Playlist::Entries Demuxer::fetchTracks(const QString &url, bool &ok)
{
    Q_UNUSED(url)
    Q_UNUSED(ok)
    return Playlist::Entries();
}
