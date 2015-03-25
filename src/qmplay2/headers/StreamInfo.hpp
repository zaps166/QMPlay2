#ifndef STREAMINFO_HPP
#define STREAMINFO_HPP

#include <QCoreApplication>
#include <QByteArray>
#include <QList>
#include <QPair>

typedef QPair< QString, QString > QMPlay2Tag;

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
	Q_DECLARE_TR_FUNCTIONS( StreamInfo )
public:
	static inline QMPlay2Tags getTag( const QString &tag )
	{
		bool ok;
		const int tagID = tag.toInt( &ok );
		if ( ok && tagID >= QMPLAY2_TAG_LANGUAGE && tagID <= QMPLAY2_TAG_COMMENT )
			return ( QMPlay2Tags )tagID;
		return QMPLAY2_TAG_UNKNOWN;
	}
	static QString getTagName( const QString &tag )
	{
		const QMPlay2Tags tagID = getTag( tag );
		switch ( tagID )
		{
			case QMPLAY2_TAG_LANGUAGE:
				return tr( "Język" );
			case QMPLAY2_TAG_TITLE:
				return tr( "Tytuł" );
			case QMPLAY2_TAG_ARTIST:
				return tr( "Artysta" );
			case QMPLAY2_TAG_ALBUM:
				return tr( "Album" );
			case QMPLAY2_TAG_GENRE:
				return tr( "Gatunek" );
			case QMPLAY2_TAG_DATE:
				return tr( "Data" );
			case QMPLAY2_TAG_COMMENT:
				return tr( "Komentarz" );
			default:
				return tag;
		}
	}

	inline StreamInfo() :
		type( QMPLAY2_TYPE_UNKNOWN ),
		is_default( true ), must_decode( false ),
		bitrate( 0 ), bpcs( 0 ),
		sample_rate( 0 ), block_align( 0 ),
		channels( 0 ),
		aspect_ratio( 0.0 ), FPS( 0.0 ),
		img_fmt( 0 ), W( 0 ), H( 0 )
	{
		time_base.num = time_base.den = 0;
	}

	inline double getTimeBase() const
	{
		return ( double )time_base.num / ( double )time_base.den;
	}

	QMPlay2MediaType type;
	QByteArray codec_name, title, artist;
	QList< QMPlay2Tag > other_info;
	QByteArray data; //subtitles header or extradata for some codecs
	bool is_default, must_decode;
	struct { int num, den; } time_base;
	int bitrate, bpcs;
	/* audio only */
	quint32 sample_rate, block_align;
	quint8 channels;
	/* video only */
	double aspect_ratio, FPS;
	int img_fmt, W, H;
};

#endif // STREAMINFO_HPP
