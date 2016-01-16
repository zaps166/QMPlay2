#include <MotionBlur.hpp>
#include <VideoFilters.hpp>
#include <VideoFrame.hpp>

MotionBlur::MotionBlur()
{
	addParam( "W" );
	addParam( "H" );
}

void MotionBlur::filter( QQueue< FrameBuffer > &framesQueue )
{
	int insertAt = addFramesToInternalQueue( framesQueue );
	while ( internalQueue.count() >= 2 )
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		FrameBuffer lookup   = internalQueue.first();

		QByteArray videoFrameData2;

		VideoFrame *videoFrame1 = VideoFrame::fromData( dequeued.data );
		VideoFrame *videoFrame2 = VideoFrame::create( videoFrameData2, w, h );
		VideoFrame *videoFrame3 = VideoFrame::fromData( lookup.data );

		for ( int p = 0; p < 3; ++p )
		{
			quint8 *src1 = videoFrame1->data[ p ];
			quint8 *src2 = videoFrame3->data[ p ];
			quint8 *dest = videoFrame2->data[ p ];
			const int linesize = videoFrame1->linesize[ p ];
			const int H = p ? h >> 1 : h;
			for ( int i = 0; i < H; ++i )
			{
				VideoFilters::averageTwoLines( dest, src1, src2, linesize );
				dest += linesize;
				src1 += linesize;
				src2 += linesize;
			}
		}

		framesQueue.insert( insertAt++, dequeued );
		framesQueue.insert( insertAt++, FrameBuffer( videoFrameData2, dequeued.ts + halfDelay( lookup, dequeued ) ) );
	}
}

bool MotionBlur::processParams( bool * )
{
	w = getParam( "W" ).toInt();
	h = getParam( "H" ).toInt();
	if ( w < 2 || h < 4 )
		return false;
	return true;
}
