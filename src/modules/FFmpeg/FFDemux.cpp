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

#include <FFDemux.hpp>
#include <FFCommon.hpp>
#include <FormatContext.hpp>

#include <Functions.hpp>
#include <OggHelper.hpp>
#include <Packet.hpp>

FFDemux::FFDemux(QMutex &avcodec_mutex, Module &module) :
	avcodec_mutex(avcodec_mutex),
	abortFetchTracks(false),
	reconnectStreamed(false)
{
	SetModule(module);
}
FFDemux::~FFDemux()
{
	streams_info.clear();
	for (const FormatContext *fmtCtx : formatContexts)
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
	for (FormatContext *fmtCtx : formatContexts)
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
	for (FormatContext *fmtCtx : formatContexts)
		fmtCtx->pause();
}
void FFDemux::abort()
{
	QMutexLocker mL(&mutex);
	for (FormatContext *fmtCtx : formatContexts)
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
		}
	}
	return !formatContexts.isEmpty();
}

Playlist::Entries FFDemux::fetchTracks(const QString &url, bool &ok)
{
	Playlist::Entries entries;
	if (!url.contains("://{") && url.startsWith("file://"))
	{
		OggHelper oggHelper(url.mid(7), abortFetchTracks);
		if (oggHelper.io)
		{
			int i = 0;
			for (const OggHelper::Chain &chains : oggHelper.getOggChains(ok))
			{
				const QString param = QString("OGG:%1:%2:%3").arg(++i).arg(chains.first).arg(chains.second);

				FormatContext *fmtCtx = new FormatContext(avcodec_mutex);
				{
					QMutexLocker mL(&mutex);
					formatContexts.append(fmtCtx);
				}

				if (fmtCtx->open(url, param))
				{
					Playlist::Entry entry;
					entry.url = QString("%1://{%2}%3").arg(DemuxerName, url, param);
					entry.name = fmtCtx->title();
					entry.length = fmtCtx->length();
					entries.append(entry);
				}

				{
					QMutexLocker mL(&mutex);
					const int idx = formatContexts.indexOf(fmtCtx);
					if (idx > -1)
						formatContexts.remove(idx);
				}
				delete fmtCtx;

				if (abortFetchTracks)
				{
					ok = false;
					break;
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
			}
		}
	}
	return entries;
}

void FFDemux::addFormatContext(QString url, const QString &param)
{
	FormatContext *fmtCtx = new FormatContext(avcodec_mutex, reconnectStreamed);
	{
		QMutexLocker mL(&mutex);
		formatContexts.append(fmtCtx);
	}
	if (!url.contains("://"))
		url.prepend("file://");
	if (fmtCtx->open(url, param))
		streams_info.append(fmtCtx->streamsInfo);
	else
	{
		{
			QMutexLocker mL(&mutex);
			formatContexts.erase(formatContexts.end() - 1);
		}
		delete fmtCtx;
	}
}
