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

#include <Playlist.hpp>

#include <Functions.hpp>
#include <Module.hpp>
#include <Writer.hpp>
#include <Reader.hpp>

Playlist::Entries Playlist::read(const QString &url, QString *name)
{
    Entries list;
    Playlist *playlist = create(url, ReadOnly, name);
    if (playlist)
    {
        list = playlist->read();
        delete playlist;
    }
    return list;
}
bool Playlist::write(const Entries &list, const QString &url, QString *name)
{
    bool OK = false;
    Playlist *playlist = create(url, WriteOnly, name);
    if (playlist)
    {
        OK = playlist->write(list);
        delete playlist;
    }
    return OK;
}
QStringList Playlist::extensions()
{
    QStringList extensions;
    for (const Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::PLAYLIST)
                extensions += mod.extensions;
    return extensions;
}

Playlist *Playlist::create(const QString &url, OpenMode openMode, QString *name)
{
    if (url.startsWith("http") && url.endsWith(".m3u8", Qt::CaseInsensitive))
        return nullptr; // Probably HLS
    const QString extension = Functions::fileExt(url).toLower();
    if (extension.isEmpty())
        return nullptr;
    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::PLAYLIST && mod.extensions.contains(extension))
            {
                if (openMode == NoOpen)
                {
                    if (name)
                        *name = mod.name;
                    return nullptr;
                }
                Playlist *playlist = (Playlist *)module->createInstance(mod.name);
                if (!playlist)
                    continue;
                switch (openMode)
                {
                    case ReadOnly:
                    {
                        IOController<Reader> &reader = playlist->ioCtrl.toRef<Reader>();
                        Reader::create(url, reader); //TODO przerywanie (po co?)
                        if (reader && reader->size() <= 0)
                            reader.reset();
                    } break;
                    case WriteOnly:
                        playlist->ioCtrl.assign(Writer::create(url));
                        break;
                    default:
                        break;
                }
                if (playlist->ioCtrl)
                {
                    if (name)
                        *name = mod.name;
                    return playlist;
                }
                delete playlist;
            }
    return nullptr;
}

QString Playlist::getPlaylistPath(const QString &url)
{
    const QString playlistPath = Functions::filePath(url);
    if (playlistPath.startsWith("file://"))
        return playlistPath.mid(7);
    return QString();
}

QList<QByteArray> Playlist::readLines()
{
    IOController<Reader> &reader = ioCtrl.toRef<Reader>();

    QByteArray data = reader->read(3);
    if (data.startsWith("\xEF\xBB\xBF")) //Remove UTF-8 BOM
        data.clear();
    data.append(reader->read(reader->size() - reader->pos()));

    return data.replace('\r', QByteArray()).split('\n');
}
