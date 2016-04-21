#include <BobDeint.hpp>
#include <VideoFilters.hpp>

BobDeint::BobDeint()
{
	addParam("W");
	addParam("H");
}

bool BobDeint::filter(QQueue<FrameBuffer> &framesQueue)
{
	int insertAt = addFramesToDeinterlace(framesQueue);
	if (internalQueue.count() >= 2)
	{
		const FrameBuffer &sourceBuffer = internalQueue.at(0);

		const VideoFrame &sourceFrame = sourceBuffer.frame;
		VideoFrame destFrame(h, (h + 1) >> 1, sourceFrame.linesize);

		const bool parity = (isTopFieldFirst(sourceFrame) == secondFrame);

		for (int p = 0; p < 3; ++p)
		{
			const int linesize = sourceFrame.linesize[p];
			const quint8 *src = sourceFrame.buffer[p].data();
			quint8 *dst = destFrame.buffer[p].data();

			const int halfH = (p ? h >> 2 : h >> 1) - 1;
			if (parity)
			{
				src += linesize;
				memcpy(dst, src, linesize); //Duplicate first line (simple deshake)
				dst += linesize;
			}
			for (int y = 0; y < halfH; ++y)
			{
				memcpy(dst, src, linesize);
				dst += linesize;

				VideoFilters::averageTwoLines(dst, src, src + (linesize << 1), linesize);
				dst += linesize;

				src += linesize << 1;
			}
			memcpy(dst, src, linesize); //Copy last line
			if (!parity)
				memcpy(dst + linesize, dst, linesize);
			if ((p ? (h >> 1) : h) & 1) //Duplicate last line for odd height
			{
				if (!parity)
					dst += linesize;
				memcpy(dst + linesize, dst, linesize);
			}
		}

		double ts = sourceBuffer.ts;
		if (secondFrame)
			ts += halfDelay(internalQueue.at(1), sourceBuffer);
		framesQueue.insert(insertAt++, FrameBuffer(destFrame, ts));

		if (secondFrame)
			internalQueue.removeFirst();
		secondFrame = !secondFrame;

	}
	return internalQueue.count() >= 2;
}

bool BobDeint::processParams(bool *)
{
	w = getParam("W").toInt();
	h = getParam("H").toInt();
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (w < 2 || h < 4 || !(deintFlags & DoubleFramerate))
		return false;
	secondFrame = false;
	return true;
}
