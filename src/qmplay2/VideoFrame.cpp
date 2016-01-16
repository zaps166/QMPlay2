#include <VideoFrame.hpp>
#include <Functions.hpp>

#include <QMutex>
#include <QList>

/* Static variables */

struct FrameBuffer
{
	quint8 *data;
	int data_size, age;
};
static QList< FrameBuffer > buffers;
static QMutex buffersMutex;

/**/

VideoFrame *VideoFrame::create( QByteArray &videoFrameData, quint8 *data[ 4 ], const int linesize[ 4 ], bool interlaced, bool top_field_first, int ref_c, int data_size )
{
	videoFrameData.resize( sizeof( VideoFrame ) );
	VideoFrame *videoFrame = fromData( videoFrameData );
	for ( int i = 0; i < 4; ++i )
	{
		videoFrame->data[ i ] = data[ i ];
		videoFrame->linesize[ i ] = linesize[ i ];
	}
	videoFrame->data_size = data_size;
	videoFrame->interlaced = interlaced;
	videoFrame->top_field_first = top_field_first;
	if ( ref_c > 0 )
		videoFrame->ref_c = new QAtomicInt( ref_c );
	else
		videoFrame->ref_c = NULL;
	return videoFrame;
}
VideoFrame *VideoFrame::create( QByteArray &videoFrameData, int width, int height, bool interlaced, bool top_field_first )
{
	const int aligned8W = Functions::aligned( width, 8 );
	const int linesize[ 4 ] = { aligned8W, aligned8W >> 1, aligned8W >> 1 };
	const int data_size = linesize[ 0 ] * height + linesize[ 1 ] * height;

	buffersMutex.lock();

	while ( !buffers.isEmpty() && buffers.first().data_size != data_size ) //Usuwanie buforów, które są innego rozmiaru niż wymagany
		delete[] buffers.takeFirst().data;

	quint8 *data[ 4 ] = { buffers.isEmpty() ? new quint8[ data_size ] : buffers.takeFirst().data };

	if ( !buffers.isEmpty() && !--buffers.last().age ) //Usuwanie starych, nieużywanych od jakiegoś czasu buforów
		delete[] buffers.takeLast().data;

	buffersMutex.unlock();

	data[ 2 ] = data[ 0 ] + linesize[ 0 ] * height;
	data[ 1 ] = data[ 2 ] + linesize[ 1 ] * ( height >> 1 );

	return create( videoFrameData, data, linesize, interlaced, top_field_first, 1, data_size );
}

bool VideoFrame::testLinesize( int width, const int linesize[ 4 ] )
{
	const int aligned8W = Functions::aligned( width, 8 );
	return aligned8W == linesize[ 0 ] && aligned8W >> 1 == linesize[ 1 ] && linesize[ 1 ] == linesize[ 2 ];
}

void VideoFrame::copyYV12( void *dest, const QByteArray &videoFrameData, unsigned luma_width, unsigned chroma_width, unsigned height )
{
	const VideoFrame *videoFrame = fromData( videoFrameData );
	unsigned offset = 0;
	unsigned char *dest_data = ( unsigned char * )dest;
	for ( unsigned i = 0; i < height; ++i )
	{
		memcpy( dest_data, videoFrame->data[ 0 ] + offset, luma_width );
		offset += videoFrame->linesize[ 0 ];
		dest_data += luma_width;
	}
	offset = 0;
	height >>= 1;
	const unsigned wh = chroma_width * height;
	for ( unsigned i = 0; i < height; ++i )
	{
		memcpy( dest_data, videoFrame->data[ 2 ] + offset, chroma_width );
		memcpy( dest_data + wh, videoFrame->data[ 1 ] + offset, chroma_width );
		offset += videoFrame->linesize[ 1 ];
		dest_data += chroma_width;
	}
}

void VideoFrame::clearBuffers()
{
	buffersMutex.lock();
	while ( !buffers.isEmpty() )
		delete[] buffers.takeFirst().data;
	buffersMutex.unlock();
}

void VideoFrame::do_unref( const QByteArray &videoFrameData )
{
	const VideoFrame *videoFrame = fromData( videoFrameData );
	if ( videoFrame->ref_c && !videoFrame->ref_c->deref() )
	{
		buffersMutex.lock();
		buffers.push_front( ( FrameBuffer ){ *videoFrame->data, videoFrame->data_size, 100 } );
		buffersMutex.unlock();
		delete videoFrame->ref_c;
	}
}
