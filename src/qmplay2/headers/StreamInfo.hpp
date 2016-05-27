#ifndef STREAMINFO_HPP
#define STREAMINFO_HPP

#include <QCoreApplication>
#include <QByteArray>
#include <QList>
#include <QPair>

typedef QPair<QString, QString> QMPlay2Tag;

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

class StreamInfo
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

	QMPlay2MediaType type;
	QByteArray codec_name, title, artist;
	QList<QMPlay2Tag> other_info;
	QByteArray data; //subtitles header or extradata for some codecs
	bool is_default, must_decode;
	struct {int num, den;} time_base;
	int bitrate, bpcs;
	unsigned codec_tag;
	/* audio only */
	quint32 sample_rate, block_align;
	quint8 channels;
	/* video only */
	double sample_aspect_ratio, FPS;
	int img_fmt, W, H;
};

class StreamsInfo : public QList<StreamInfo *>
{
	Q_DISABLE_COPY(StreamsInfo)
public:
	inline StreamsInfo()
	{}
	inline ~StreamsInfo()
	{
		for (int i = 0; i < count(); ++i)
			delete at(i);
	}
};

#endif // STREAMINFO_HPP
