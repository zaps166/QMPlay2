#include <FFDemux.hpp>
#include <FFCommon.hpp>
#include <FormatContext.hpp>

#include <Functions.hpp>
#include <Packet.hpp>

#include <QDebug>

FFDemux::FFDemux(QMutex &avcodec_mutex, Module &module) :
	avcodec_mutex(avcodec_mutex)
{
	SetModule(module);
}
FFDemux::~FFDemux()
{
	streams_info.clear();
	foreach (FormatContext *fmtCtx, formatContexts)
		delete fmtCtx;
}

bool FFDemux::set()
{
	return sets().getBool("DemuxerEnabled");
}

bool FFDemux::metadataChanged() const
{
	bool isMetadataChanged = false;
	foreach (FormatContext *fmtCtx, formatContexts)
		isMetadataChanged |= fmtCtx->metadataChanged();
	return isMetadataChanged;
}

QList< ChapterInfo > FFDemux::getChapters() const
{
	if (formatContexts.count() == 1)
		return formatContexts.at(0)->getChapters();
	return QList< ChapterInfo >();
}

QString FFDemux::name() const
{
	QString name;
	foreach (FormatContext *fmtCtx, formatContexts)
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
QList< QMPlay2Tag > FFDemux::tags() const
{
	if (formatContexts.count() == 1)
		return formatContexts.at(0)->tags();
	return QList< QMPlay2Tag >();
}
bool FFDemux::getReplayGain(bool album, float &gain_db, float &peak) const
{
	if (formatContexts.count() == 1)
		return formatContexts.at(0)->getReplayGain(album, gain_db, peak);
	return false;
}
double FFDemux::length() const
{
	double length = -1.0;
	foreach (FormatContext *fmtCtx, formatContexts)
		length = qMax(length, fmtCtx->length());
	return length;
}
int FFDemux::bitrate() const
{
	int bitrate = 0;
	foreach (FormatContext *fmtCtx, formatContexts)
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
	foreach (FormatContext *fmtCtx, formatContexts)
		if (!fmtCtx->isLocal)
			return false;
	return true;
}

bool FFDemux::seek(int pos, bool backward)
{
	bool seeked = false;
	foreach (FormatContext *fmtCtx, formatContexts)
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
	foreach (FormatContext *fmtCtx, formatContexts)
		fmtCtx->pause();
}
void FFDemux::abort()
{
	QMutexLocker mL(&mutex);
	foreach (FormatContext *fmtCtx, formatContexts)
		fmtCtx->abort();
}

bool FFDemux::open(const QString &entireUrl)
{
	QString prefix, url;
	if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(entireUrl, &prefix, &url))
		addFormatContext(entireUrl);
	else if (prefix == DemuxerName)
	{
		foreach (QString stream, url.split("][", QString::SkipEmptyParts))
		{
			stream.remove('[');
			stream.remove(']');
			addFormatContext(stream);
		}
	}
	return !formatContexts.isEmpty();
}

void FFDemux::addFormatContext(QString url)
{
	FormatContext *fmtCtx = new FormatContext(avcodec_mutex);
	{
		QMutexLocker mL(&mutex);
		formatContexts.append(fmtCtx);
	}
	if (!url.contains("://"))
		url.prepend("file://");
	if (fmtCtx->open(url))
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
