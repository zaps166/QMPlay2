#include <BlendDeint.hpp>
#include <VideoFilters.hpp>

BlendDeint::BlendDeint()
{
	addParam("W");
	addParam("H");
}

bool BlendDeint::filter(QQueue<FrameBuffer> &framesQueue)
{
	addFramesToDeinterlace(framesQueue);
	while (!internalQueue.isEmpty())
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		VideoFrame &videoFrame = dequeued.frame;
		videoFrame.setNoInterlaced();
		for (int p = 0; p < 3; ++p)
		{
			const int linesize = videoFrame.linesize[p];
			quint8 *data = videoFrame.buffer[p].data() + linesize;
			const int h = videoFrame.size.getHeight(p) - 2;
			for (int i = 0; i < h; ++i)
			{
				VideoFilters::averageTwoLines(data, data, data + linesize, linesize);
				data += linesize;
			}
		}
		framesQueue.enqueue(dequeued);
	}
	return !internalQueue.isEmpty();
}

bool BlendDeint::processParams(bool *)
{
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (deintFlags & DoubleFramerate))
		return false;
	return true;
}
