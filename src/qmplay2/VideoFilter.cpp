#include <VideoFilter.hpp>
#include <VideoFrame.hpp>

bool VideoFilter::removeLastFromInternalBuffer()
{
	if (!internalQueue.isEmpty())
	{
		internalQueue.removeLast();
		return true;
	}
	return false;
}

int VideoFilter::addFramesToInternalQueue(QQueue<VideoFilter::FrameBuffer> &framesQueue)
{
	while (!framesQueue.isEmpty())
	{
		if (framesQueue.at(0).frame.hasNoData())
			break;
		internalQueue.enqueue(framesQueue.dequeue());
	}
	return framesQueue.count();
}
