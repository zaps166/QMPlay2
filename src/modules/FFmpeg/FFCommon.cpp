#include <FFCommon.hpp>

#include <QMPlay2Core.hpp>
#include <DeintFilter.hpp>

extern "C"
{
	#include <libavformat/version.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/dict.h>
}

QString FFCommon::prepareUrl( QString url, AVDictionary *&options, bool *isLocal )
{
	if ( url.startsWith( "file://" ) )
	{
		url.remove( 0, 7 );
		if ( isLocal )
			*isLocal = true;
	}
	else
	{
		if ( url.left( 4 ) == "mms:" )
			url.insert( 3, 'h' );
#if LIBAVFORMAT_VERSION_MAJOR <= 55
		if ( url.left( 4 ) == "http" )
			av_dict_set( &options, "icy", "1", 0 );
#endif
		av_dict_set( &options, "user-agent", "QMPlay2/" QMPlay2Version, 0 );
		if ( isLocal )
			*isLocal = false;
	}
	return url;
}

int FFCommon::getField( const VideoFrame *videoFrame, int deinterlace, int fullFrame, int topField, int bottomField )
{
	if ( deinterlace )
	{
		const quint8 deintFlags = deinterlace >> 1;
		if ( videoFrame->interlaced || !( deintFlags & DeintFilter::AutoDeinterlace ) )
		{
			bool topFieldFirst;
			if ( ( deintFlags & DeintFilter::DoubleFramerate ) || ( ( deintFlags & DeintFilter::AutoParity ) && videoFrame->interlaced ) )
				topFieldFirst = videoFrame->top_field_first;
			else
				topFieldFirst = deintFlags & DeintFilter::TopFieldFirst;
			return topFieldFirst ? topField : bottomField;
		}
	}
	return fullFrame;
}

AVPacket *FFCommon::createAVPacket()
{
	AVPacket *packet;
#if LIBAVCODEC_VERSION_MAJOR >= 57
	packet = av_packet_alloc();
#else
	packet = ( AVPacket * )av_malloc( sizeof( AVPacket ) );
	av_init_packet( packet );
#endif
	return packet;
}
void FFCommon::freeAVPacket( AVPacket *&packet )
{
#if LIBAVCODEC_VERSION_MAJOR >= 57
	av_packet_free( &packet );
#else
	// Packet buffer is always NULL here, so no need to unref!
	av_freep( &packet );
#endif
}
