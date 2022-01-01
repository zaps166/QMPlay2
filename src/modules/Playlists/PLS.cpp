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

#include <PLS.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Writer.hpp>

static void ensureLastEntryHasName(Playlist::Entries &list, int lastEntryIdx, int entryIdx = -1)
{
    if (lastEntryIdx > -1 && lastEntryIdx != entryIdx)
    {
        Playlist::Entry &entry = list[lastEntryIdx];
        if (entry.name.isEmpty())
            entry.name = Functions::fileName(entry.url, false);
    }
}

/**/

Playlist::Entries PLS::read()
{
    Reader *reader = ioCtrl.rawPtr<Reader>();
    Entries list;

    const QString playlistPath = getPlaylistPath(reader->getUrl());

    int lastEntryIdx = -1;

    const QList<QByteArray> playlistLines = readLines();
    for (int i = 0; i < playlistLines.count(); ++i)
    {
        QByteArray line = playlistLines[i];
        if (line.isEmpty())
            continue;

        int idx = line.indexOf('=');
        if (idx < 0)
            continue;

        int numberIdx = -1;
        for (int i = 0; i < line.length(); ++i)
        {
            const char c = line.at(i);
            if (c == '=')
            {
                if (list.isEmpty())
                {
                    const QByteArray tmpKey = line.left(i);
                    if (tmpKey == "File" || tmpKey == "Title" || tmpKey == "Length")
                    {
                        line.insert(i, '1');
                        numberIdx = i;
                        ++idx;
                    }
                }
                break;
            }
            if (c >= '0' && c <= '9')
            {
                numberIdx = i;
                break;
            }
        }
        if (numberIdx == -1)
            continue;

        const QByteArray key = line.left(numberIdx);
        const QByteArray value = line.mid(idx + 1);

        const int entryIdx = line.mid(numberIdx, idx - numberIdx).toInt() - 1;
        if (entryIdx < 0)
            continue;

        ensureLastEntryHasName(list, lastEntryIdx, entryIdx);
        lastEntryIdx = entryIdx;

        if (list.size() <= entryIdx)
            list.resize(entryIdx + 1);

        Entry &entry = list[entryIdx];
        if (key == "File")
            entry.url = Functions::Url(value, playlistPath);
        else if (key == "Title")
            (entry.name = value).replace('\001', '\n');
        else if (key == "Length" && entry.length == -1.0)
            entry.length = value.toInt();
        else if (key == "QMPlay_length")
            entry.length = value.toDouble();
        else if (key == "QMPlay_sel") //Obsolete
            entry.flags |= Entry::Selected;
        else if (key == "QMPlay_flags")
            entry.flags = value.toInt();
        else if (key == "QMPlay_queue")
            entry.queue = value.toInt();
        else if (key == "QMPlay_GID")
            entry.GID = value.toInt();
        else if (key == "QMPlay_parent")
            entry.parent = value.toInt();
    }

    ensureLastEntryHasName(list, lastEntryIdx);

    return list;
}
bool PLS::write(const Entries &list)
{
    Writer *writer = ioCtrl.rawPtr<Writer>();
    const QString playlistPath = getPlaylistPath(writer->getUrl());
    writer->write(QString("[playlist]\r\nNumberOfEntries=" + QString::number(list.size()) + "\r\n").toUtf8());
    for (int i = 0; i < list.size(); i++)
    {
        const Entry &entry = list[i];
        const QString idx = QString::number(i+1);
        QString url = entry.url;
        if (url.startsWith("file://"))
        {
            url.remove(0, 7);
            if (url.startsWith(playlistPath))
                url.remove(0, playlistPath.length());
#ifdef Q_OS_WIN
            url.replace("/", "\\");
#endif
        }
        if (!url.isEmpty())
            writer->write(QString("File" + idx + "=" + url + "\r\n").toUtf8());
        if (!entry.name.isEmpty())
            writer->write(QString("Title" + idx + "=" + QString(entry.name).replace('\n', '\001') + "\r\n").toUtf8());
        if (entry.length >= 0.0)
        {
            writer->write(QString("Length" + idx + "=" + QString::number((qint32)(entry.length + 0.5)) + "\r\n").toUtf8());
            writer->write(QString("QMPlay_length" + idx + "=" + QString::number(entry.length, 'g', 13) + "\r\n").toUtf8());
        }
        if (entry.flags)
            writer->write(QString("QMPlay_flags" + idx + "=" + QString::number(entry.flags) + "\r\n").toUtf8());
        if (entry.queue)
            writer->write(QString("QMPlay_queue" + idx + "=" + QString::number(entry.queue) + "\r\n").toUtf8());
        if (entry.GID)
            writer->write(QString("QMPlay_GID" + idx + "=" + QString::number(entry.GID) + "\r\n").toUtf8());
        if (entry.parent)
            writer->write(QString("QMPlay_parent" + idx + "=" + QString::number(entry.parent) + "\r\n").toUtf8());
    }
    return true;
}
