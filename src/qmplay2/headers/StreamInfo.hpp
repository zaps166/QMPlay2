/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#pragma once

#include <QMPlay2Lib.hpp>

#include <QCoreApplication>
#include <QByteArray>
#include <QVector>
#include <QPair>

using QMPlay2Tag = QPair<QString, QString>;

enum QMPlay2MediaType
{
	QMPLAY2_TYPE_UNKNOWN = -1,
	QMPLAY2_TYPE_VIDEO,
	QMPLAY2_TYPE_AUDIO,
	QMPLAY2_TYPE_DATA,
	QMPLAY2_TYPE_SUBTITLE,
	QMPLAY2_TYPE_ATTACHMENT
};

enum QMPlay2Tags
{
	QMPLAY2_TAG_UNKNOWN = -1,
	QMPLAY2_TAG_NAME, //should be as first
	QMPLAY2_TAG_DESCRIPTION, //should be as second
	QMPLAY2_TAG_LANGUAGE,
	QMPLAY2_TAG_TITLE,
	QMPLAY2_TAG_ARTIST,
	QMPLAY2_TAG_ALBUM,
	QMPLAY2_TAG_GENRE,
	QMPLAY2_TAG_DATE,
	QMPLAY2_TAG_COMMENT
};

class QMPLAY2SHAREDLIB_EXPORT StreamInfo
{
	Q_DECLARE_TR_FUNCTIONS(StreamInfo)
public:
	static QMPlay2Tags getTag(const QString &tag);
	static QString getTagName(const QString &tag);

	StreamInfo();
	StreamInfo(quint32 sample_rate, quint8 channels);

	inline double getTimeBase() const
	{
		return (double)time_base.num / (double)time_base.den;
	}
	inline double getAspectRatio() const
	{
		return sample_aspect_ratio * W / H;
	}

	QMPlay2MediaType type = QMPLAY2_TYPE_UNKNOWN;
	QByteArray codec_name, title, artist, format;
	QVector<QMPlay2Tag> other_info;
	QByteArray data; //subtitles header or extradata for some codecs
	bool is_default = true, must_decode = false;
	struct {int num, den;} time_base = {0, 0};
	int bitrate = 0, bpcs = 0;
	unsigned codec_tag = 0;
	/* audio only */
	quint32 sample_rate = 0, block_align = 0;
	quint8 channels = 0;
	/* video only */
	double sample_aspect_ratio = 1.0, FPS = 0.0;
	int W = 0, H = 0;
	double rotation = qQNaN();
	bool spherical = false;
};

class QMPLAY2SHAREDLIB_EXPORT StreamsInfo : public QList<StreamInfo *>
{
	Q_DISABLE_COPY(StreamsInfo)
public:
	StreamsInfo() = default;
	~StreamsInfo();
};
