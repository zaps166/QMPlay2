#include <StreamInfo.hpp>

QMPlay2Tags StreamInfo::getTag(const QString &tag)
{
	bool ok;
	const int tagID = tag.toInt(&ok);
	if (ok && tagID >= QMPLAY2_TAG_LANGUAGE && tagID <= QMPLAY2_TAG_COMMENT)
		return (QMPlay2Tags)tagID;
	return QMPLAY2_TAG_UNKNOWN;
}
QString StreamInfo::getTagName(const QString &tag)
{
	const QMPlay2Tags tagID = getTag(tag);
	switch (tagID)
	{
		case QMPLAY2_TAG_LANGUAGE:
			return tr("Language");
		case QMPLAY2_TAG_TITLE:
			return tr("Title");
		case QMPLAY2_TAG_ARTIST:
			return tr("Artist");
		case QMPLAY2_TAG_ALBUM:
			return tr("Album");
		case QMPLAY2_TAG_GENRE:
			return tr("Genre");
		case QMPLAY2_TAG_DATE:
			return tr("Date");
		case QMPLAY2_TAG_COMMENT:
			return tr("Comment");
		default:
			return tag;
	}
}

StreamInfo::StreamInfo() :
	type(QMPLAY2_TYPE_UNKNOWN),
	is_default(true), must_decode(false),
	bitrate(0), bpcs(0),
	codec_tag(0),
	sample_rate(0), block_align(0),
	channels(0),
	aspect_ratio(0.0), FPS(0.0),
	img_fmt(0), W(0), H(0)
{
	time_base.num = time_base.den = 0;
}
StreamInfo::StreamInfo(quint32 sample_rate, quint8 channels) :
	type(QMPLAY2_TYPE_AUDIO),
	is_default(true), must_decode(false),
	bitrate(0), bpcs(0),
	codec_tag(0),
	sample_rate(sample_rate), block_align(0),
	channels(channels),
	aspect_ratio(0.0), FPS(0.0),
	img_fmt(0), W(0), H(0)
{
	time_base.num = time_base.den = 0;
}
