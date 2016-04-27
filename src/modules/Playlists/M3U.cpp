#include <M3U.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Writer.hpp>

Playlist::Entries M3U::read()
{
	Reader *reader = ioCtrl.rawPtr<Reader>();
	Entries list;

	QString playlistPath = Functions::filePath(reader->getUrl());
	if (playlistPath.startsWith("file://"))
		playlistPath.remove(0, 7);
	else
		playlistPath.clear();

	QList<QByteArray> playlistLines = readLines();
	bool hasExtinf = false;
	QString extinf[2];
	while (!playlistLines.isEmpty())
	{
		const QByteArray line = playlistLines.takeFirst();
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
		else
		{
			if (!line.startsWith("#"))
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
				list += entry;
			}
			hasExtinf = false;
		}
	}

	return list;
}
bool M3U::write(const Entries &list)
{
	Writer *writer = ioCtrl.rawPtr<Writer>();
	writer->write("#EXTM3U\r\n");
	for (int i = 0; i < list.size(); i++)
	{
		const Entry &entry = list[i];
		if (!entry.GID)
		{
			const QString length = QString::number((qint32)(entry.length + 0.5));
			QString url = entry.url;
			const bool isFile = url.startsWith("file://");
			if (isFile)
				url.remove(0, 7);
#ifdef Q_OS_WIN
			if (isFile)
				url.replace("/", "\\");
#endif
			writer->write(QString("#EXTINF:" + length + "," + QString(entry.name).replace('\n', '\001') + "\r\n" + url + "\r\n").toUtf8());
		}
	}
	return true;
}

M3U::~M3U()
{}
