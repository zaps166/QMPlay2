#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <QByteArray>
#include <QAtomicInt>

class VideoFrame
{
public:
	static VideoFrame *create( QByteArray &videoFrameData, quint8 *data[ 4 ], const int linesize[ 4 ], bool interlaced = false, bool top_field_first = false, int ref_c = 0, int data_size = 0 );
	static VideoFrame *create( QByteArray &videoFrameData, int width, int height, bool interlaced = false, bool top_field_first = false );

	static bool testLinesize( int width, const int linesize[ 4 ] );

	static inline void ref( QByteArray &videoFrameData )
	{
		VideoFrame *videoFrame = fromData( videoFrameData );
		if ( videoFrame->ref_c )
			videoFrame->ref_c->ref();
	}

	static inline void unref( const QByteArray &videoFrameData )
	{
		if ( !videoFrameData.isEmpty() )
			do_unref( videoFrameData );
	}
	static inline void unref( QByteArray &videoFrameData )
	{
		if ( !videoFrameData.isEmpty() )
		{
			do_unref( videoFrameData );
			videoFrameData.clear();
		}
	}

	static inline const VideoFrame *fromData( const QByteArray &videoFrameData )
	{
		return ( const VideoFrame * )videoFrameData.data();
	}
	static inline VideoFrame *fromData( QByteArray &videoFrameData )
	{
		return ( VideoFrame * )videoFrameData.data();
	}

	static void copyYV12( void *dest, const QByteArray &videoFrameData, unsigned luma_width, unsigned chroma_width, unsigned height ); //Use only on YUV420 frames!

	static void clearBuffers();

	inline void setNoInterlaced()
	{
		interlaced = top_field_first = false;
	}

	quint8 *data[ 4 ];
	int linesize[ 4 ], data_size;
	bool interlaced, top_field_first;
private:
	VideoFrame();
	VideoFrame( const VideoFrame & );
	~VideoFrame();

	static void do_unref( const QByteArray &videoFrameData );

	QAtomicInt *ref_c;
};

#endif
