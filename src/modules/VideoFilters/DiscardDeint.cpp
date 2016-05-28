#include <DiscardDeint.hpp>
#include <VideoFilters.hpp>

DiscardDeint::DiscardDeint()
{
	addParam("W");
	addParam("H");
}

bool DiscardDeint::filter(QQueue<FrameBuffer> &framesQueue)
{
	addFramesToDeinterlace(framesQueue);
	if (!internalQueue.isEmpty())
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		VideoFrame &videoFrame = dequeued.frame;
		const bool TFF = isTopFieldFirst(videoFrame);
		videoFrame.setNoInterlaced();
		for (int p = 0; p < 3; ++p)
		{
			const int linesize = videoFrame.linesize[p];
			const quint8 *src = videoFrame.buffer[p].data();
			quint8 *dst = videoFrame.buffer[p].data();
			const int lines = (videoFrame.size.getHeight(p) >> 1) - 1;
			if (!TFF)
			{
				memcpy(dst, src + linesize, linesize);
				src += linesize;
				dst += linesize;
			}
			dst += linesize;
			src += linesize;
			for (int i = 0; i < lines; ++i)
			{
				VideoFilters::averageTwoLines(dst, src - linesize, src + linesize, linesize);
				src += linesize << 1;
				dst += linesize << 1;
			}
			if (TFF)
				memcpy(dst, src - linesize, linesize);
		}
		framesQueue.enqueue(dequeued);
	}
	return !internalQueue.isEmpty();
}

bool DiscardDeint::processParams(bool *)
{
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (deintFlags & DoubleFramerate))
		return false;
	return true;
}
