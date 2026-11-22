// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    QString urlForValidation;
    if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(url, nullptr, &urlForValidation))
        urlForValidation = url;
    if (urlForValidation.startsWith("http") && urlForValidation.endsWith(".m3u8", Qt::CaseInsensitive))
        return nullptr; // Probably HLS
    const QString extension = Functions::fileExt(urlForValidation).toLower();
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

    return data.replace('\r', '\n').split('\n');
}
