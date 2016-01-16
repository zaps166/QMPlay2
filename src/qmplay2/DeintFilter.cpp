#include <DeintFilter.hpp>

int DeintFilter::addFramesToDeinterlace(QQueue< FrameBuffer > &framesQueue, bool checkSize)
{
	while (!framesQueue.isEmpty())
	{
		VideoFrame *videoFrame = VideoFrame::fromData(framesQueue.first().data);
		if (((deintFlags & AutoDeinterlace) && !videoFrame->interlaced) || (checkSize && !videoFrame->data_size))
			break;
		internalQueue.enqueue(framesQueue.dequeue());
	}
	return framesQueue.isEmpty();
}
