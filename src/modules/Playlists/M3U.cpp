/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <M3U.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Writer.hpp>

Playlist::Entries M3U::read()
{
    Reader *reader = ioCtrl.rawPtr<Reader>();
    Entries list;

    const QString playlistPath = getPlaylistPath(reader->getUrl());

    bool hasExtinf = false;
    QString extinf[2];
    QHash<QByteArray, QByteArray> extvlcopt;
    for (const QByteArray &line : readLines())
    {
        if (line.simplified().isEmpty())
            continue;
        if (line.startsWith("#EXTINF:"))
        {
            const int idx = line.indexOf(',');
            if (idx < 0)
            {
                hasExtinf = false;
                continue;
            }
            extinf[0] = line.mid(8, idx - 8);
            extinf[1] = line.right(line.length() - idx - 1);
            hasExtinf = true;
        }
        else if (line.startsWith("#EXTVLCOPT:"))
        {
            const int idx = line.indexOf('=');
            if (idx < 0)
            {
                continue;
            }
            extvlcopt[line.mid(11, idx - 11)] = line.right(line.length() - idx - 1);
        }
        else if (!line.startsWith("#"))
        {
            Entry entry;
            if (!hasExtinf)
                entry.name = Functions::fileName(line, false);
            else
            {
                entry.length = extinf[0].toInt();
                entry.name = extinf[1].replace('\001', '\n');
            }
            entry.url = Functions::Url(line, playlistPath);
            if (const auto userAgent = extvlcopt.value("http-user-agent"); !userAgent.isEmpty())
                entry.params[Entry::UserAgentParam] = userAgent;
            if (const auto referrer = extvlcopt.value("http-referrer"); !referrer.isEmpty())
                entry.params[Entry::ReferrerParam] = referrer;
            list += entry;
            hasExtinf = false;
        }
    }

    return list;
}
bool M3U::write(const Entries &list)
{
    Writer *writer = ioCtrl.rawPtr<Writer>();
    const QString playlistPath = getPlaylistPath(writer->getUrl());
    writer->write("#EXTM3U\r\n");
    for (const Entry &entry : list)
    {
        if (!entry.GID)
        {
            const QString length = QString::number((qint32)(entry.length + 0.5));
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
            writer->write(QString("#EXTINF:" + length + "," + QString(entry.name).replace('\n', '\001') + "\r\n").toUtf8());
            for (auto it = entry.params.cbegin(), itEnd = entry.params.cend(); it != itEnd; ++it)
            {
                const auto &key = it.key();
                if (key == Playlist::Entry::UserAgentParam)
                {
                    writer->write(QStringLiteral("#EXTVLCOPT:http-user-agent=%1\r\n").arg(QString(it.value().trimmed())).toUtf8());
                }
                else if (key == Playlist::Entry::ReferrerParam)
                {
                    writer->write(QStringLiteral("#EXTVLCOPT:http-referrer=%1\r\n").arg(QString(it.value().trimmed())).toUtf8());
                }
            }
            writer->write((url + "\r\n").toUtf8());
        }
    }
    return true;
}
