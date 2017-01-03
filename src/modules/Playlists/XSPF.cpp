/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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
#include <QFileInfo>

Playlist::Entries XSPF::read()
{
	Reader *reader = ioCtrl.rawPtr<Reader>();
	const QString readFilePath = Functions::filePath(reader->getUrl().mid(7));
	Entries list;

	QXmlStreamReader xmlReader(reader->read(reader->size()));

	bool inEntry = false;
	Entry curr;
	while (!xmlReader.atEnd() && !xmlReader.hasError())
	{
		QXmlStreamReader::TokenType token = xmlReader.readNext();
		if (token == QXmlStreamReader::StartDocument)
			continue;
		if (token == QXmlStreamReader::StartElement)
		{
			const QStringRef name = xmlReader.name();
			if (name == "track")
			{
				if (inEntry)
				{
					if (curr.name.isEmpty())
						curr.name = Functions::fileName(curr.url, false);
					list += curr;
				}
				curr = Entry();
				inEntry = true;
			}
			else if (inEntry)
			{
				const QString value = xmlReader.readElementText();
				if (name == "location")
				{
					if (value.startsWith("file://")) // local absolute path
						curr.url = Functions::Url(value);
					else if (Functions::splitPrefixAndUrlIfHasPluginPrefix(value, NULL, NULL, NULL)) // plugin URL
						curr.url = value;
					else if (QFileInfo::exists(readFilePath + value)) // relative path
						curr.url = Functions::Url(value, readFilePath);
				}
				else if (name == "title")
					curr.name = value;
				else if (name == "duration")
					curr.length = value.toInt() / 1000.0;
				else if (name == "flags")
					curr.flags = value.toInt();
				else if (name == "queue")
					curr.queue = value.toInt();
				else if (name == "group")
					curr.GID = value.toInt();
				else if (name == "parent")
					curr.parent = value.toInt();
			}
		}
		else if (token == QXmlStreamReader::EndElement && xmlReader.name() == "track")
		{
			if (inEntry)
			{
				if (curr.name.isEmpty())
					curr.name = Functions::fileName(curr.url, false);
				list += curr;
			}
			curr = Entry();
			inEntry = false;
		}
	}
	return list;
}
bool XSPF::write(const Entries &list)
{
	Writer *writer = ioCtrl.rawPtr<Writer>();
	const QString saveFilePath = Functions::filePath(writer->getUrl());

	QByteArray buffer;
	QXmlStreamWriter xmlWriter(&buffer);
	xmlWriter.setAutoFormatting(true);
	xmlWriter.setAutoFormattingIndent(-1);

	xmlWriter.writeStartDocument();
	xmlWriter.writeStartElement("playlist");
	xmlWriter.writeStartElement("trackList");

	for (Entries::const_iterator iter = list.constBegin(), end = list.constEnd(); iter != end; ++iter)
	{
		xmlWriter.writeStartElement("track");

		if (!iter->url.isEmpty())
		{
			QString url = iter->url;
			if (m_useRelative && url.startsWith(saveFilePath))
				url.remove(0, saveFilePath.length());
			xmlWriter.writeTextElement("location", url);
		}
		if (!iter->name.isEmpty())
			xmlWriter.writeTextElement("title", iter->name);
		if (iter->length >= 0.0)
			xmlWriter.writeTextElement("duration", QString::number((qint32)(1000 * iter->length)));
		if (iter->flags)
			xmlWriter.writeTextElement("flags", QString::number(iter->flags));
		if (iter->queue)
			xmlWriter.writeTextElement("queue", QString::number(iter->queue));
		if (iter->GID)
			xmlWriter.writeTextElement("group", QString::number(iter->GID));
		if (iter->parent)
			xmlWriter.writeTextElement("parent", QString::number(iter->parent));

		xmlWriter.writeEndElement(); // track
	}

	xmlWriter.writeEndElement(); // trackList
	xmlWriter.writeEndElement(); // playlist
	xmlWriter.writeEndDocument();

	writer->write(buffer);
	return true;
}
