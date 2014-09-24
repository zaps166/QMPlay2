#include <VideoFilter.hpp>
#include <VideoFrame.hpp>

void VideoFilter::clearBuffer()
{
	while ( !internalQueue.isEmpty() )
		VideoFrame::unref( internalQueue.dequeue().data );
}
bool VideoFilter::removeLastFromInternalBuffer()
{
	if ( !internalQueue.isEmpty() )
	{
		VideoFrame::unref( internalQueue.takeLast().data );
		return true;
	}
	return false;
}

int VideoFilter::addFramesToInternalQueue( QQueue<VideoFilter::FrameBuffer> &framesQueue )
{
	while ( !framesQueue.isEmpty() )
	{
		if ( !VideoFrame::fromData( framesQueue.first().data )->data_size )
			break;
		internalQueue.enqueue( framesQueue.dequeue() );
	}
	return framesQueue.count();
}
