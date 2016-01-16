#include <VideoFilters.hpp>

#include <DeintFilter.hpp>
#include <VideoFrame.hpp>
#include <TimeStamp.hpp>
#include <Module.hpp>

extern "C"
{
	#include <libavutil/cpu.h>
}

#ifdef QMPLAY2_CPU_X86
	static void averageTwoLines_MMXEXT(quint8 *dest, quint8 *src1, quint8 *src2, int linesize)
	{
		const int remaining = linesize % 8;
		quint8 *dest_end = dest + linesize - remaining;
		while (dest < dest_end)
		{
			asm volatile
			(
				"movq  %1,    %%mm0;"
				"movq  %2,    %%mm1;"
				"pavgb %%mm1, %%mm0;"
				"movq  %%mm0, %0;"
				:"=m"(*dest)
				: "m"(*src1), "m"(*src2)
			);
			dest += 8;
			src1 += 8;
			src2 += 8;
		}
		asm volatile ("emms");
		dest_end += remaining;
		while (dest < dest_end)
			*dest++ = (*(src1++) + *(src2++)) >> 1;
	}
	static void averageTwoLines_SSE2(quint8 *dest, quint8 *src1, quint8 *src2, int linesize)
	{
		const int remaining = linesize % 16;
		quint8 *dest_end = dest + linesize - remaining;
		while (dest < dest_end)
		{
			asm volatile
			(
				"movdqu %1,     %%xmm0;"
				"movdqu %2,     %%xmm1;"
				"pavgb  %%xmm1, %%xmm0;"
				"movdqu %%xmm0, %0;"
				:"=m"(*dest)
				: "m"(*src1), "m"(*src2)
			);
			dest += 16;
			src1 += 16;
			src2 += 16;
		}
		dest_end += remaining;
		while (dest < dest_end)
			*dest++ = (*(src1++) + *(src2++)) >> 1;
	}
#endif

static void averageTwoLines_C(quint8 *dest, quint8 *src1, quint8 *src2, int linesize)
{
	for (int i = 0; i < linesize; ++i)
		dest[i] = (src1[i] + src2[i]) >> 1;
}

void (*VideoFilters::averageTwoLinesPtr)(quint8 *dest, quint8 *src1, quint8 *src2, int linesize);

/**/

class PrepareForHWBobDeint : public DeintFilter
{
public:
	void filter(QQueue< FrameBuffer > &framesQueue)
	{
		int insertAt = addFramesToDeinterlace(framesQueue, false);
		while (internalQueue.count() >= 2)
		{
			FrameBuffer dequeued = internalQueue.dequeue();
			const bool TFF = isTopFieldFirst(VideoFrame::fromData(dequeued.data));
			VideoFrame::fromData(dequeued.data)->top_field_first = TFF;
			framesQueue.insert(insertAt++, dequeued);
			VideoFrame::fromData(dequeued.data)->top_field_first = !TFF;
			framesQueue.insert(insertAt++,  FrameBuffer(dequeued.data, dequeued.ts + halfDelay(internalQueue.first(), dequeued)));
			VideoFrame::ref(dequeued.data);
		}
	}

	bool processParams(bool *)
	{
		deintFlags = getParam("DeinterlaceFlags").toInt();
		if (!(deintFlags & DoubleFramerate))
			return false;
		return true;
	}
};

/**/

void VideoFilters::init()
{
	averageTwoLinesPtr = averageTwoLines_C;
#ifdef QMPLAY2_CPU_X86
	const int cpuFlags = av_get_cpu_flags();
	if (cpuFlags & AV_CPU_FLAG_SSE2)
		averageTwoLinesPtr = averageTwoLines_SSE2;
	else if (cpuFlags & AV_CPU_FLAG_MMXEXT)
		averageTwoLinesPtr = averageTwoLines_MMXEXT;
#endif
}

void VideoFilters::clear()
{
	if (hasFilters)
	{
		foreach (VideoFilter *vFilter, videoFilters)
			delete vFilter;
		videoFilters.clear();
		hasFilters = false;
	}
	clearBuffers();
}

VideoFilter *VideoFilters::on(const QString &filterName)
{
	VideoFilter *filter = NULL;
	if (filterName == "PrepareForHWBobDeint")
		filter = new PrepareForHWBobDeint;
	else foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if ((mod.type & 0xF) == Module::VIDEOFILTER && mod.name == filterName)
			{
				filter = (VideoFilter *)module->createInstance(mod.name);
				break;
			}
	if (filter)
	{
		videoFilters.push_back(filter);
		hasFilters = true;
	}
	return filter;
}
void VideoFilters::off(VideoFilter *&videoFilter)
{
	const int idx = videoFilters.indexOf(videoFilter);
	if (idx > -1)
	{
		videoFilters.remove(idx);
		delete videoFilter;
		videoFilter = NULL;
	}
	hasFilters = !videoFilters.isEmpty();
}

void VideoFilters::clearBuffers()
{
	if (hasFilters)
		foreach (VideoFilter *vFilter, videoFilters)
			vFilter->clearBuffer();
	while (!outputQueue.isEmpty())
		VideoFrame::unref(outputQueue.dequeue().data);
	outputNotEmpty = false;
}
void VideoFilters::removeLastFromInputBuffer()
{
	if (hasFilters)
		for (int i = videoFilters.count() - 1; i >= 0; --i)
			if (videoFilters[i]->removeLastFromInternalBuffer())
				break;
}

void VideoFilters::addFrame(const QByteArray &videoFrameData, double ts)
{
	if (!hasFilters)
	{
		outputQueue.enqueue(VideoFilter::FrameBuffer(videoFrameData, ts));
		outputNotEmpty = true;
	}
	else
	{
		QQueue< VideoFilter::FrameBuffer > tmpQueue;
		tmpQueue.enqueue(VideoFilter::FrameBuffer(videoFrameData, ts));
		foreach (VideoFilter *vFilter, videoFilters)
		{
			vFilter->filter(tmpQueue);
			if (tmpQueue.isEmpty())
				break;
		}
		outputQueue += tmpQueue;
		outputNotEmpty = !outputQueue.isEmpty();
	}
}
bool VideoFilters::getFrame(QByteArray &videoFrameData, TimeStamp &ts)
{
	if (!outputQueue.isEmpty())
	{
		VideoFrame::unref(videoFrameData);
		videoFrameData = outputQueue.front().data;
		ts = outputQueue.front().ts;
		outputQueue.pop_front();
		outputNotEmpty = !outputQueue.isEmpty();
		return true;
	}
	return false;
}
