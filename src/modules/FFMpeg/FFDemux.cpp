#include <FFDemux.hpp>
#include <FFCommon.hpp>

#include <Reader.hpp>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextCodec>
#include <QMutex>
#include <QDebug>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
}

/**/

#if LIBAVFORMAT_VERSION_MAJOR > 55
static void matroska_fix_ass_packet( AVRational stream_timebase, AVPacket *pkt )
{
	AVBufferRef *line;
	char *layer, *ptr = ( char * )pkt->data, *end = ptr + pkt->size;
	for ( ; *ptr != ',' && ptr < end - 1 ; ptr++ );
	if ( *ptr == ',' )
		ptr++;
	layer = ptr;
	for ( ; *ptr != ',' && ptr < end - 1 ; ptr++ );
	if ( *ptr == ',' )
	{
		int64_t end_pts = pkt->pts + pkt->duration;
		int sc = pkt->pts * stream_timebase.num * 100 / stream_timebase.den;
		int ec = end_pts  * stream_timebase.num * 100 / stream_timebase.den;
		int sh, sm, ss, eh, em, es, len;
		sh     = sc / 360000;
		sc    -= 360000 * sh;
		sm     = sc / 6000;
		sc    -= 6000 * sm;
		ss     = sc / 100;
		sc    -= 100 * ss;
		eh     = ec / 360000;
		ec    -= 360000 * eh;
		em     = ec / 6000;
		ec    -= 6000 * em;
		es     = ec / 100;
		ec    -= 100 * es;
		*ptr++ = '\0';
		len    = 50 + end - ptr + FF_INPUT_BUFFER_PADDING_SIZE;
		if ( !( line = av_buffer_alloc( len ) ) )
			return;
		snprintf( ( char * )line->data, len, "Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s\r\n", layer, sh, sm, ss, sc, eh, em, es, ec, ptr );
		av_buffer_unref( &pkt->buf );
		pkt->buf  = line;
		pkt->data = line->data;
		pkt->size = strlen( ( char * )line->data );
	}
}
#endif

/**/

static int interruptCB( bool &aborted )
{
	return aborted;
}

static int q_read( void *ptr, unsigned char *buf, int buf_size )
{
	Reader *reader = ( Reader * )ptr;
	if ( !reader->readyRead() )
		return -1;
	const QByteArray arr = reader->read( buf_size );
	memcpy( buf, arr.data(), arr.size() );
	return arr.size();
}
static int64_t q_seek( void *ptr, int64_t offset, int wh )
{
	Reader *reader = ( Reader * )ptr;
	if ( wh == AVSEEK_SIZE )
	{
		const int64_t s = reader->size();
		return s > 0 ? s : 0;
	}
	if ( ( wh == 0 && offset < 0 ) || !reader->seek( offset, wh ) )
		return -1;
	return reader->pos();
}
static int q_read_pause( void *ptr, int pause )
{
	if ( pause )
		( ( Reader * )ptr )->pause();
	return 0;
}

static QString getTag( const QString &page, const QString &tag )
{
	int idx = page.indexOf( tag );
	if ( idx > -1 )
	{
		idx += tag.length();
		int start_idx = page.indexOf( "<b>", idx );
		if ( start_idx > -1 )
		{
			start_idx += 3;
			int end_idx = page.indexOf( "</b>", start_idx );
			if ( end_idx > -1 )
				return page.mid( start_idx, end_idx - start_idx );
		}
	}
	return QString();
}

/**/

FFDemux::FFDemux( QMutex &avcodec_mutex, Module &module ) :
	formatCtx( NULL ),
	paused( false ), aborted( false ), fix_mkv_ass( false ),
	isMetadataChanged( false ),
	lastTime( 0.0 ),
	lastErr( 0 ),
	avcodec_mutex( avcodec_mutex )
{
	lastTS.set( 0.0, 0.0 );
	SetModule( module );
}

FFDemux::~FFDemux()
{
	if ( formatCtx )
	{
		for ( int i = 0 ; i < streams.count() ; ++i )
		{
			AVStream *stream = streams[ i ];
			if ( stream->codec )
				switch ( ( quintptr )stream->codec->opaque )
				{
					case 1:
						stream->codec->extradata = NULL;
						stream->codec->extradata_size = 0;
						break;
					case 2:
						stream->codec->subtitle_header = NULL;
						stream->codec->subtitle_header_size = 0;
						break;
				}
		}
		if ( reader )
		{
			AVIOContext *pb = formatCtx->pb;
			avformat_close_input( &formatCtx );
			av_free( pb->buffer );
			av_free( pb );
		}
		else
			avformat_close_input( &formatCtx );
	}
}

bool FFDemux::set()
{
	return sets().getBool( "DemuxerEnabled" );
}

bool FFDemux::metadataChanged() const
{
#if LIBAVFORMAT_VERSION_MAJOR > 55
	if ( formatCtx->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED )
	{
		formatCtx->event_flags = 0;
		isMetadataChanged = false;
		return true;
	}
#endif
	if ( isMetadataChanged )
	{
		isMetadataChanged = false;
		return true;
	}
	return false;
}

QList< FFDemux::ChapterInfo > FFDemux::getChapters() const
{
	QList< ChapterInfo > chapters;
	for ( unsigned i = 0 ; i < formatCtx->nb_chapters ; i++ )
	{
		AVChapter &chapter = *formatCtx->chapters[ i ];
		ChapterInfo chapterInfo;
		chapterInfo.start = ( double )chapter.start / ( double )( chapter.time_base.num * chapter.time_base.den );
		chapterInfo.end = ( double )chapter.end / ( double )( chapter.time_base.num * chapter.time_base.den );
		AVDictionaryEntry *avtag = av_dict_get( chapter.metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX );
		if ( avtag )
			chapterInfo.title = avtag->value;
		chapters += chapterInfo;
	}
	return chapters;
}

QString FFDemux::name() const
{
	return formatCtx->iformat->name;
}
QString FFDemux::title() const
{
	if ( !streamTitle.isNull() )
		return streamTitle;
	else
	{
		AVDictionary *dict = ( !formatCtx->metadata && streams.count() == 1 ) ? streams[ 0 ]->metadata : formatCtx->metadata;
		if ( dict )
		{
			QString title, artist;
			AVDictionaryEntry *avtag;
			if ( ( avtag = av_dict_get( dict, "title", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				title = avtag->value;
			if ( ( avtag = av_dict_get( dict, "artist", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				artist = avtag->value;
			if ( !title.simplified().isEmpty() && !artist.simplified().isEmpty() )
				return artist + " - " + title;
			else if ( title.simplified().isEmpty() && !artist.simplified().isEmpty() )
				return artist;
			else if ( !title.simplified().isEmpty() && artist.simplified().isEmpty() )
				return title;
		}
		return QString();
	}
}
QList< QMPlay2Tag > FFDemux::tags() const
{
	QList< QMPlay2Tag > tagList;
	if ( !songTitle.isNull() && !streamGenre.isNull() )
	{
		if ( !songTitle.isEmpty() )
		{
			int idx = songTitle.indexOf( " - " );
			if ( idx < 0 )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_TITLE ), songTitle );
			else
			{
				tagList << qMakePair( QString::number( QMPLAY2_TAG_TITLE ), songTitle.mid( idx + 3 ) );
				tagList << qMakePair( QString::number( QMPLAY2_TAG_ARTIST ), songTitle.mid( 0, idx ) );
			}
		}
		if ( !streamGenre.isEmpty() )
			tagList << qMakePair( QString::number( QMPLAY2_TAG_GENRE ), streamGenre );
	}
	else
	{
		AVDictionary *dict = getMetadata();
		if ( dict )
		{
			AVDictionaryEntry *avtag;
			QString value;
			if ( ( avtag = av_dict_get( dict, "title", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_TITLE ), value );
			if ( ( avtag = av_dict_get( dict, "artist", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_ARTIST ), value );
			if ( ( avtag = av_dict_get( dict, "album", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_ALBUM ), value );
			if ( ( avtag = av_dict_get( dict, "genre", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_GENRE ), value );
			if ( ( avtag = av_dict_get( dict, "date", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_DATE ), value );
			if ( ( avtag = av_dict_get( dict, "comment", NULL, AV_DICT_IGNORE_SUFFIX ) ) && !( value = avtag->value ).simplified().isEmpty() )
				tagList << qMakePair( QString::number( QMPLAY2_TAG_COMMENT ), value );
		}
	}
	return tagList;
}
bool FFDemux::getReplayGain( bool album, float &gain_db, float &peak )
{
	AVDictionary *dict = getMetadata();
	if ( dict )
	{
		AVDictionaryEntry *avtag;
		QString album_gain_db, album_peak, track_gain_db, track_peak;

		if ( ( avtag = av_dict_get( dict, "REPLAYGAIN_ALBUM_GAIN", NULL, AV_DICT_IGNORE_SUFFIX ) ) && avtag->value )
			album_gain_db = avtag->value;
		if ( ( avtag = av_dict_get( dict, "REPLAYGAIN_ALBUM_PEAK", NULL, AV_DICT_IGNORE_SUFFIX ) ) && avtag->value )
			album_peak = avtag->value;
		if ( ( avtag = av_dict_get( dict, "REPLAYGAIN_TRACK_GAIN", NULL, AV_DICT_IGNORE_SUFFIX ) ) && avtag->value )
			track_gain_db = avtag->value;
		if ( ( avtag = av_dict_get( dict, "REPLAYGAIN_TRACK_PEAK", NULL, AV_DICT_IGNORE_SUFFIX ) ) && avtag->value )
			track_peak = avtag->value;

		if ( album_gain_db.isEmpty() && !track_gain_db.isEmpty() )
			album_gain_db = track_gain_db;
		if ( !album_gain_db.isEmpty() && track_gain_db.isEmpty() )
			track_gain_db = album_gain_db;
		if ( album_peak.isEmpty() && !track_peak.isEmpty() )
			album_peak = track_peak;
		if ( !album_peak.isEmpty() && track_peak.isEmpty() )
			track_peak = album_peak;

		QString str_gain_db, str_peak;
		if ( album )
		{
			str_gain_db = album_gain_db;
			str_peak = album_peak;
		}
		else
		{
			str_gain_db = track_gain_db;
			str_peak = track_peak;
		}

		int space_idx = str_gain_db.indexOf( ' ' );
		if ( space_idx > -1 )
			str_gain_db.remove( space_idx, str_gain_db.length() - space_idx );

		bool ok;
		float tmp = str_peak.toFloat( &ok );
		if ( ok )
			peak = tmp;
		tmp = str_gain_db.toFloat( &ok );
		if ( ok )
			gain_db = tmp;

		return ok;
	}
	return false;
}
double FFDemux::length() const
{
	if ( !isStreamed && formatCtx->duration != QMPLAY2_NOPTS_VALUE )
		return formatCtx->duration / ( double )AV_TIME_BASE;
	return -1.0;
}
int FFDemux::bitrate() const
{
	return formatCtx->bit_rate / 1000;
}
QByteArray FFDemux::image( bool forceCopy ) const
{
	foreach ( AVStream *stream, streams )
		if ( stream->disposition & AV_DISPOSITION_ATTACHED_PIC )
			return forceCopy ? QByteArray( ( const char * )stream->attached_pic.data, stream->attached_pic.size ) : QByteArray::fromRawData( ( const char * )stream->attached_pic.data, stream->attached_pic.size );
	return QByteArray();
}

bool FFDemux::localStream() const
{
	return reader && reader->getParam( "Local" ).toBool();
}

bool FFDemux::seek( int val, bool backward )
{
	if ( !isStreamed )
	{
		if ( val < 0 )
			val = 0;
		else if ( length() > 0 && val >= length() )
		{
			abort();
			return true;
		}
		val += start_time;
		if ( !seekByByte )
			return av_seek_frame( formatCtx, -1, ( int64_t )val * AV_TIME_BASE, backward ? AVSEEK_FLAG_BACKWARD : 0 ) >= 0;
		else if ( length() > 0 )
			return av_seek_frame( formatCtx, -1, ( int64_t )val * ( avio_size( formatCtx->pb ) - formatCtx->data_offset ) / length() + formatCtx->data_offset, AVSEEK_FLAG_BYTE | ( backward ? AVSEEK_FLAG_BACKWARD : 0 ) ) >= 0;
	}
	return false;
}
bool FFDemux::read( QByteArray &encoded, int &idx, TimeStamp &ts, double &duration )
{
	if ( aborted )
		return false;

	if ( paused )
	{
		paused = false;
		av_read_play( formatCtx );
	}

	class Packet : public AVPacket
	{
	public:
		inline Packet()
		{
			data = NULL;
		}
		inline ~Packet()
		{
			if ( data )
				av_free_packet( this );
		}
	} packet;
	const int ret = av_read_frame( formatCtx, &packet );
	if ( ret == AVERROR_INVALIDDATA )
	{
		if ( lastErr != AVERROR_INVALIDDATA )
		{
			lastErr = AVERROR_INVALIDDATA;
			return true;
		}
		return false;
	}
	else
		lastErr = 0;
	if ( ret == AVERROR( EAGAIN ) )
		return true;
	else if ( ret )
		return false;
	const int ff_idx = packet.stream_index;
	if ( ff_idx >= streams.count() )
	{
		QMPlay2Core.log( "Stream index out of range: " + QString::number( ff_idx ), ErrorLog | LogOnce | DontShowInGUI );
		return true;
	}

#if LIBAVFORMAT_VERSION_MAJOR > 55
	if ( streams[ ff_idx ]->event_flags & AVSTREAM_EVENT_FLAG_METADATA_UPDATED )
	{
		streams[ ff_idx ]->event_flags = 0;
		isMetadataChanged = true;
	}
	if ( fix_mkv_ass && streams[ ff_idx ]->codec->codec_id == AV_CODEC_ID_ASS )
		matroska_fix_ass_packet( streams[ ff_idx ]->time_base, &packet );
#endif

	encoded.clear();
	encoded.reserve( packet.size + FF_INPUT_BUFFER_PADDING_SIZE );
	encoded.resize( packet.size );
	memcpy( encoded.data(), packet.data, encoded.capacity() );

	const double time_base = av_q2d( streams[ ff_idx ]->time_base );

	ts = 0.0;
	if ( seekByByte )
	{
		if ( packet.pos > -1 && length() > 0.0 )
			lastTime = ts = ( ( packet.pos - formatCtx->data_offset ) * length() ) / ( avio_size( formatCtx->pb ) - formatCtx->data_offset );
		else
			ts = lastTime;
	}
	else
		ts.set( packet.dts * time_base, packet.pts * time_base, start_time );

	if ( packet.duration > 0 )
		duration = packet.duration * time_base;
	else if ( !ts || ( duration = ts - lastTS ) < 0.0 /* Calculate packet duration if doesn't exists */ )
		duration = 0.0;
	lastTS = ts;

	idx = index_map[ ff_idx ];

	return true;
}
void FFDemux::pause()
{
	av_read_pause( formatCtx );
	paused = true;
}
void FFDemux::abort()
{
	aborted = true;
	reader.abort();
}

bool FFDemux::open( const QString &_url )
{
	if ( Reader::create( _url, reader ) )
	{
		formatCtx = avformat_alloc_context();
		AVIOContext *pb = formatCtx->pb = avio_alloc_context( ( uchar * )av_malloc( 16384 ), 16384, 0, reader.rawPtr(), q_read, NULL, q_seek );
		pb->seekable = reader->canSeek();
		pb->read_pause = q_read_pause;
		if ( avformat_open_input( &formatCtx, "", NULL, NULL ) )
		{
			if ( !formatCtx )
			{
				av_free( pb->buffer );
				av_free( pb );
			}
			return false;
		}
	}
	else
	{
		QString url = _url;
		if ( url.left( 5 ) != "http:" && url.left( 6 ) != "https:" && url.left( 5 ) != "file:" )
		{
			if ( url.left( 4 ) == "mms:" )
				url.insert( 3, 'h' );
			formatCtx = avformat_alloc_context();
			formatCtx->interrupt_callback.callback = ( int( * )( void * ) )interruptCB;
			formatCtx->interrupt_callback.opaque = &aborted;
			if ( avformat_open_input( &formatCtx, url.toLocal8Bit(), NULL, NULL ) )
				return false;
		}
	}
	if ( !formatCtx )
		return false;

	formatCtx->flags |= AVFMT_FLAG_GENPTS;

	avcodec_mutex.lock();
	if ( avformat_find_stream_info( formatCtx, NULL ) < 0 )
	{
		avcodec_mutex.unlock();
		return false;
	}
	avcodec_mutex.unlock();
	if ( reader && formatCtx->pb )
		isStreamed = avio_size( formatCtx->pb ) <= 0;
	else
		isStreamed = formatCtx->duration == QMPLAY2_NOPTS_VALUE;
	seekByByte = name() == "mp3" && !isStreamed;

	start_time = formatCtx->start_time / ( double )AV_TIME_BASE;
	if ( start_time < 0.0 )
		start_time = 0.0;

	index_map.resize( formatCtx->nb_streams );
	for ( unsigned i = 0 ; i < formatCtx->nb_streams ; ++i )
	{
		StreamInfo *streamInfo = getStreamInfo( formatCtx->streams[ i ] );
		if ( !streamInfo )
			index_map[ i ] = -1;
		else
		{
			index_map[ i ] = streams_info.count();
			streams_info += streamInfo;
		}
#if LIBAVFORMAT_VERSION_MAJOR > 55
		if ( !fix_mkv_ass && formatCtx->streams[ i ]->codec->codec_id == AV_CODEC_ID_ASS && !strncasecmp( formatCtx->iformat->name, "matroska", 8 ) )
			fix_mkv_ass = true;
		formatCtx->streams[ i ]->event_flags = 0;
#endif
		streams += formatCtx->streams[ i ];
	}

	if ( isStreamed && _url.left( 5 ) == "http:" )
	{
		netInfoURL = _url;
		connect( &netInfoTimer, SIGNAL( timeout() ), this, SLOT( netInfoTimeout() ) );
		netInfoTimer.start( 0 );
	}

#if LIBAVFORMAT_VERSION_MAJOR > 55
	formatCtx->event_flags = 0;
#endif

	return true;
}


AVDictionary *FFDemux::getMetadata() const
{
	return ( !formatCtx->metadata && streams_info.count() == 1 ) ? streams[ 0 ]->metadata : formatCtx->metadata;
}
StreamInfo *FFDemux::getStreamInfo( AVStream *stream ) const
{
	const AVCodecID codecID = stream->codec->codec_id;
	if
	(
		!stream ||
		( stream->disposition & AV_DISPOSITION_ATTACHED_PIC ) ||
		( stream->codec->codec_type == AVMEDIA_TYPE_DATA )    ||
		( stream->codec->codec_type == AVMEDIA_TYPE_ATTACHMENT && codecID != AV_CODEC_ID_TTF && codecID != AV_CODEC_ID_OTF )
	) return NULL;

	StreamInfo *streamInfo = new StreamInfo;

	if ( AVCodec *codec = avcodec_find_decoder( codecID ) )
		streamInfo->codec_name = codec->name;
	streamInfo->must_decode = codecID != AV_CODEC_ID_SSA && codecID != AV_CODEC_ID_SUBRIP && codecID != AV_CODEC_ID_SRT;
#if LIBAVCODEC_VERSION_MAJOR > 54
	streamInfo->must_decode &= codecID != AV_CODEC_ID_ASS;
#endif
	streamInfo->bitrate = stream->codec->bit_rate;
	streamInfo->bpcs = stream->codec->bits_per_coded_sample;
	streamInfo->is_default = stream->disposition & AV_DISPOSITION_DEFAULT;
	streamInfo->time_base.num = stream->time_base.num;
	streamInfo->time_base.den = stream->time_base.den;
	streamInfo->type = ( QMPlay2MediaType )stream->codec->codec_type; //Enumy są takie same

	if ( streamInfo->type != QMPLAY2_TYPE_SUBTITLE && stream->codec->extradata_size )
	{
		streamInfo->data.reserve( stream->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE );
		streamInfo->data.resize( stream->codec->extradata_size );
		memcpy( streamInfo->data.data(), stream->codec->extradata, streamInfo->data.capacity() );
		av_free( stream->codec->extradata );
		stream->codec->extradata = ( quint8 * )streamInfo->data.data();
		stream->codec->opaque = ( void * )1;
	}

	AVDictionaryEntry *avtag;
	if ( streamInfo->type != QMPLAY2_TYPE_ATTACHMENT )
	{
		if ( streams_info.count() > 1 )
		{
			if ( ( avtag = av_dict_get( stream->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->title = avtag->value;
			if ( ( avtag = av_dict_get( stream->metadata, "artist", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->artist = avtag->value;
			if ( ( avtag = av_dict_get( stream->metadata, "album", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->other_info << qMakePair( QString::number( QMPLAY2_TAG_ALBUM ), QString( avtag->value ) );
			if ( ( avtag = av_dict_get( stream->metadata, "genre", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->other_info << qMakePair( QString::number( QMPLAY2_TAG_GENRE ), QString( avtag->value ) );
			if ( ( avtag = av_dict_get( stream->metadata, "date", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->other_info << qMakePair( QString::number( QMPLAY2_TAG_DATE ), QString( avtag->value ) );
			if ( ( avtag = av_dict_get( stream->metadata, "comment", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->other_info << qMakePair( QString::number( QMPLAY2_TAG_COMMENT ), QString( avtag->value ) );
		}
		if ( ( avtag = av_dict_get( stream->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
		{
			const QString value = avtag->value;
			if ( value != "und" )
				streamInfo->other_info << qMakePair( QString::number( QMPLAY2_TAG_LANGUAGE ), value );
		}
	}

	switch ( streamInfo->type )
	{
		case QMPLAY2_TYPE_AUDIO:
			streamInfo->channels = stream->codec->channels;
			streamInfo->sample_rate = stream->codec->sample_rate;
			streamInfo->block_align = stream->codec->block_align;
			streamInfo->other_info << qMakePair( tr( "format" ), QString( av_get_sample_fmt_name( stream->codec->sample_fmt ) ) );
			break;
		case QMPLAY2_TYPE_VIDEO:
			streamInfo->aspect_ratio = 1.0;
			if ( stream->sample_aspect_ratio.num )
				streamInfo->aspect_ratio = av_q2d( stream->sample_aspect_ratio );
			else if ( stream->codec->sample_aspect_ratio.num )
				streamInfo->aspect_ratio = av_q2d( stream->codec->sample_aspect_ratio );
			if ( streamInfo->aspect_ratio <= 0.0 )
				streamInfo->aspect_ratio = 1.0;
			if ( stream->codec->width > 0 && stream->codec->height > 0 )
				streamInfo->aspect_ratio *= ( double )stream->codec->width / ( double )stream->codec->height;
			streamInfo->W = stream->codec->width;
			streamInfo->H = stream->codec->height;
			streamInfo->img_fmt = stream->codec->pix_fmt;
			streamInfo->FPS = av_q2d( stream->r_frame_rate );
			streamInfo->other_info << qMakePair( tr( "format" ), QString( av_get_pix_fmt_name( stream->codec->pix_fmt ) ) );
			break;
		case QMPLAY2_TYPE_SUBTITLE:
			if ( stream->codec->subtitle_header_size )
			{
				streamInfo->data = QByteArray( ( char * )stream->codec->subtitle_header, stream->codec->subtitle_header_size );
				av_free( stream->codec->subtitle_header );
				stream->codec->subtitle_header = ( quint8 * )streamInfo->data.data();
				stream->codec->opaque = ( void * )2;
			}
			break;
		case AVMEDIA_TYPE_ATTACHMENT:
			if ( ( avtag = av_dict_get( stream->metadata, "filename", NULL, AV_DICT_IGNORE_SUFFIX ) ) )
				streamInfo->title = avtag->value;
			switch ( codecID )
			{
				case AV_CODEC_ID_TTF:
					streamInfo->codec_name = "TTF";
					break;
				case AV_CODEC_ID_OTF:
					streamInfo->codec_name = "OTF";
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return streamInfo;
}

void FFDemux::netInfoTimeout()
{
	netInfoTimer.stop();
	QNetworkReply *reply = net.get( QNetworkRequest( netInfoURL ) );
	connect( reply, SIGNAL( finished() ), SLOT( netFinished() ) );
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ), SLOT( netDLProgress( qint64, qint64 ) ) );
}
void FFDemux::netDLProgress( qint64 bytesReceived, qint64 )
{
	if ( bytesReceived > 0x10000 )
		sender()->deleteLater();
}
void FFDemux::netFinished()
{
	QNetworkReply *reply = ( QNetworkReply * )sender();
	if ( !reply->error() )
	{
		const QByteArray pageArr = reply->readAll();
		QString page;

		int charset_idx = pageArr.indexOf( "charset=" );
		if ( charset_idx > -1 )
		{
			charset_idx += 8;
			int charset_end_idx = pageArr.indexOf( '"', charset_idx );
			if ( charset_end_idx > -1 )
			{
				const QTextCodec *txtCodec = QTextCodec::codecForName( pageArr.mid( charset_idx, charset_end_idx - charset_idx ) );
				if ( txtCodec )
					page = txtCodec->toUnicode( pageArr );
			}
		}
		if ( page.isNull() )
			page = pageArr;

		const QString _streamTitle = getTag( page, "Stream Title" );
		const QString _streamGenre = getTag( page, "Stream Genre" );
		const QString _songTitle = getTag( page, "Current Song" );

		if ( !_streamTitle.isNull() && !_streamGenre.isNull() && !_songTitle.isNull() )
		{
			if ( ( isMetadataChanged = _streamTitle != streamTitle || _streamGenre != streamGenre || _songTitle != songTitle ) )
			{
				streamTitle = _streamTitle;
				streamGenre = _streamGenre;
				songTitle = _songTitle;
			}
			netInfoTimer.start( 10000 );
		}
	}
	reply->deleteLater();
}
