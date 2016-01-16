#include <DiscardDeint.hpp>
#include <VideoFilters.hpp>

DiscardDeint::DiscardDeint()
{
	addParam("W");
	addParam("H");
}

void DiscardDeint::filter(QQueue< FrameBuffer > &framesQueue)
{
	int insertAt = addFramesToDeinterlace(framesQueue);
	while (!internalQueue.isEmpty())
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		VideoFrame *videoFrame = VideoFrame::fromData(dequeued.data);
		const bool TFF = isTopFieldFirst(videoFrame);
		videoFrame->setNoInterlaced();
		for (int p = 0; p < 3; ++p)
		{
			const int linesize = videoFrame->linesize[ p ];
			quint8 *src = videoFrame->data[ p ];
			quint8 *dst = videoFrame->data[ p ];
			const int lines = (p ? h >> 2 : h >> 1) - 1;
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
		framesQueue.insert(insertAt++, dequeued);
	}
}

bool DiscardDeint::processParams(bool *)
{
	w = getParam("W").toInt();
	h = getParam("H").toInt();
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (w < 2 || h < 4 || (deintFlags & DoubleFramerate))
		return false;
	return true;
}
