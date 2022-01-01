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

#include <XSPF.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Writer.hpp>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QUrl>

static void startExtension(QXmlStreamWriter &xmlWriter)
{
    xmlWriter.writeStartElement("extension");
    xmlWriter.writeAttribute("application", "QMPlay2");
}

/**/

Playlist::Entries XSPF::read()
{
    Reader *reader = ioCtrl.rawPtr<Reader>();
    Entries list;

    const QString playlistPath = getPlaylistPath(reader->getUrl());

    QXmlStreamReader xmlReader(reader->read(reader->size()));
    bool inTrack = false, inExtension = false;
    Entry curr;

    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        const QXmlStreamReader::TokenType token = xmlReader.readNext();
        const QStringRef name = xmlReader.name();
        switch (token)
        {
            case QXmlStreamReader::StartElement:
                if (name == "track")
                    inTrack = true;
                else if (inTrack)
                {
                    if (name == "extension")
                    {
                        if (xmlReader.attributes().value("application") == "QMPlay2")
                            inExtension = true;
                    }
                    else
                    {
                        const QString value = xmlReader.readElementText();
                        if (name == "location")
                            curr.url = Functions::Url(QUrl::fromPercentEncoding(value.toUtf8()), playlistPath);
                        else if (name == "title")
                            curr.name = value;
                        else if (name == "duration")
                            curr.length = value.toInt() / 1000.0;
                        if (inExtension)
                        {
                            if (name == "f")
                                curr.flags = value.toInt();
                            else if (name == "q")
                                curr.queue = value.toInt();
                            else if (name == "g")
                                curr.GID = value.toInt();
                            else if (name == "p")
                                curr.parent = value.toInt();
                        }
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (name == "extension")
                    inExtension = false;
                else if (name == "track")
                {
                    if (inTrack)
                    {
                        if (curr.name.isEmpty())
                            curr.name = Functions::fileName(curr.url, false);
                        list += curr;
                    }
                    curr = Entry();
                    inTrack = false;
                }
                break;
            default:
                break;
        }
    }

    return list;
}
bool XSPF::write(const Entries &list)
{
    Writer *writer = ioCtrl.rawPtr<Writer>();
    const QString playlistPath = getPlaylistPath(writer->getUrl());

    QByteArray buffer;
    QXmlStreamWriter xmlWriter(&buffer);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setAutoFormattingIndent(-1);

    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("playlist");
    xmlWriter.writeStartElement("trackList");

    for (const Entry &entry : list)
    {
        xmlWriter.writeStartElement("track");

        if (entry.GID)
            startExtension(xmlWriter);

        QString url = entry.url;
        if (url.startsWith("file://") && url.mid(7, playlistPath.length()) == playlistPath)
            url.remove(0, playlistPath.length() + 7);
        xmlWriter.writeTextElement("location", url);

        if (!entry.name.isEmpty())
            xmlWriter.writeTextElement("title", entry.name);
        if (entry.length >= 0.0)
            xmlWriter.writeTextElement("duration", QString::number(1000.0 * entry.length, 'f', 0));

        if (entry.flags || entry.queue || entry.GID || entry.parent)
        {
            if (!entry.GID)
                startExtension(xmlWriter);

            if (entry.flags)
                xmlWriter.writeTextElement("f", QString::number(entry.flags));
            if (entry.queue)
                xmlWriter.writeTextElement("q", QString::number(entry.queue));
            if (entry.GID)
                xmlWriter.writeTextElement("g", QString::number(entry.GID));
            if (entry.parent)
                xmlWriter.writeTextElement("p", QString::number(entry.parent));

            xmlWriter.writeEndElement(); // extension
        }

        xmlWriter.writeEndElement(); // track
    }

    xmlWriter.writeEndElement(); // trackList
    xmlWriter.writeEndElement(); // playlist
    xmlWriter.writeEndDocument();

    writer->write(buffer);
    return true;
}
