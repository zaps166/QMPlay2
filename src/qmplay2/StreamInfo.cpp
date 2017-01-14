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
	sample_aspect_ratio(1.0), FPS(0.0),
	W(0), H(0)
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
	sample_aspect_ratio(1.0), FPS(0.0),
	W(0), H(0)
{
	time_base.num = time_base.den = 0;
}

/**/

StreamsInfo::~StreamsInfo()
{
	for (int i = 0; i < count(); ++i)
		delete at(i);
}
