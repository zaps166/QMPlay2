#include <DeintFilter.hpp>

void DeintFilter::addFramesToDeinterlace(QQueue<FrameBuffer> &framesQueue, bool checkSize)
{
	while (!framesQueue.isEmpty())
	{
		const VideoFrame &videoFrame = framesQueue.at(0).frame;
		if (((deintFlags & AutoDeinterlace) && !videoFrame.interlaced) || (checkSize && videoFrame.hasNoData()))
			break;
		internalQueue.enqueue(framesQueue.dequeue());
	}
}
