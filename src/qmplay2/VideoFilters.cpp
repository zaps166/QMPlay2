/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <VideoFilters.hpp>

#include <DeintFilter.hpp>
#include <VideoFrame.hpp>
#include <TimeStamp.hpp>
#include <Module.hpp>
#include <CPU.hpp>

extern "C"
{
	#include <libavutil/cpu.h>
}

#ifdef QMPLAY2_CPU_X86
#ifdef QMPLAY2_CPU_X86_32 //Every x86-64 CPU has SSE2, so MMXEXT is unused there
static void averageTwoLines_MMXEXT(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
{
	const int remaining = linesize % 8;
	quint8 *dest_end = dest + linesize - remaining;
	while (dest < dest_end)
	{
		asm volatile
		(
			"movq  %1,    %%mm0\n\t"
			"movq  %2,    %%mm1\n\t"
			"pavgb %%mm1, %%mm0\n\t"
			"movq  %%mm0, %0\n\t"
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
#endif // QMPLAY2_CPU_X86_32
static void averageTwoLines_SSE2(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
{
	const int remaining = linesize % 16;
	quint8 *dest_end = dest + linesize - remaining;
	while (dest < dest_end)
	{
		asm volatile
		(
			"movdqu %1,     %%xmm0\n\t"
			"movdqu %2,     %%xmm1\n\t"
			"pavgb  %%xmm1, %%xmm0\n\t"
			"movdqu %%xmm0, %0\n\t"
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
#endif // QMPLAY2_CPU_X86
static void averageTwoLines_C(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
{
	for (int i = 0; i < linesize; ++i)
		dest[i] = (src1[i] + src2[i]) >> 1;
}

void (*VideoFilters::averageTwoLinesPtr)(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize);

/**/

class PrepareForHWBobDeint : public DeintFilter
{
public:
	void clearBuffer() override
	{
		secondFrame = false;
		lastTS = -1.0;
		DeintFilter::clearBuffer();
	}

	bool filter(QQueue<FrameBuffer> &framesQueue) override
	{
		addFramesToDeinterlace(framesQueue, false);
		if (internalQueue.count() >= 1)
		{
			FrameBuffer frameBuffer = internalQueue.at(0);

			frameBuffer.frame.tff = (isTopFieldFirst(frameBuffer.frame) != secondFrame);
			if (secondFrame)
				frameBuffer.ts += halfDelay(frameBuffer.ts, lastTS);

			framesQueue.enqueue(frameBuffer);

			if (secondFrame || lastTS < 0.0)
				lastTS = frameBuffer.ts;

			if (secondFrame)
				internalQueue.removeFirst();
			secondFrame = !secondFrame;
		}
		return internalQueue.count() >= 1;
	}

	bool processParams(bool *) override
	{
		deintFlags = getParam("DeinterlaceFlags").toInt();
		if (!(deintFlags & DoubleFramerate))
			return false;
		secondFrame = false;
		lastTS = -1.0;
		return true;
	}

private:
	bool secondFrame;
	double lastTS;
};

/**/

class VideoFiltersThr final : public QThread
{
public:
	VideoFiltersThr(VideoFilters &videoFilters) :
		videoFilters(videoFilters)
	{
		setObjectName("VideoFiltersThr");
	}
	~VideoFiltersThr()
	{
		stop();
	}

	void start()
	{
		br = filtering = false;
		QThread::start();
	}
	void stop()
	{
		{
			QMutexLocker locker(&mutex);
			br = true;
			cond.wakeOne();
		}
		wait();
	}

	void filterFrame(const VideoFilter::FrameBuffer &frame)
	{
		QMutexLocker locker(&mutex);
		frameToFilter = frame;
		filtering = true;
		cond.wakeOne();
	}

	void waitForFinished(bool waitForAllFrames)
	{
		bufferMutex.lock();
		while (filtering && !br)
		{
			if (!waitForAllFrames && !videoFilters.outputQueue.isEmpty())
				break;
			cond.wait(&bufferMutex);
		}
		if (waitForAllFrames)
			bufferMutex.unlock();
	}

	QMutex bufferMutex;
private:
	void run() override
	{
		while (!br)
		{
			QMutexLocker locker(&mutex);

			if (frameToFilter.frame.isEmpty() && !br)
				cond.wait(&mutex);
			if (frameToFilter.frame.isEmpty() || br)
				continue;

			QQueue<VideoFilter::FrameBuffer> queue;
			queue.enqueue(frameToFilter);
			frameToFilter.frame.clear();

			bool pending = false;
			do
			{
				for (VideoFilter *vFilter : asConst(videoFilters.filters))
				{
					pending |= vFilter->filter(queue);
					if (queue.isEmpty())
					{
						pending = false;
						break;
					}
				}

				{
					QMutexLocker locker(&bufferMutex);
					if (!queue.isEmpty())
					{
						videoFilters.outputQueue.append(queue);
						videoFilters.outputNotEmpty = true;
						queue.clear();
					}
					if (!pending)
						filtering = false;
				}

				cond.wakeOne();
			} while (pending && !br);
		}
		if (br)
		{
			QMutexLocker locker(&bufferMutex);
			filtering = false;
			cond.wakeOne();
		}
	}

	VideoFilters &videoFilters;

	bool br = false, filtering = false;

	QWaitCondition cond;
	QMutex mutex;

	VideoFilter::FrameBuffer frameToFilter;
};

/**/

void VideoFilters::init()
{
	averageTwoLinesPtr = averageTwoLines_C;
#ifdef QMPLAY2_CPU_X86
	const int cpuFlags = av_get_cpu_flags();
	if (cpuFlags & AV_CPU_FLAG_SSE2)
		averageTwoLinesPtr = averageTwoLines_SSE2;
#ifdef QMPLAY2_CPU_X86_32
	else if (cpuFlags & AV_CPU_FLAG_MMXEXT)
		averageTwoLinesPtr = averageTwoLines_MMXEXT;
#endif // QMPLAY2_CPU_X86_32
#endif // QMPLAY2_CPU_X86
}

VideoFilters::VideoFilters() :
	filtersThr(*(new VideoFiltersThr(*this))),
	outputNotEmpty(false)
{}
VideoFilters::~VideoFilters()
{
	clear();
	delete &filtersThr;
}

void VideoFilters::start()
{
	if (!filters.isEmpty())
		filtersThr.start();
}
void VideoFilters::clear()
{
	if (!filters.isEmpty())
	{
		filtersThr.stop();
		for (VideoFilter *vFilter : asConst(filters))
			delete vFilter;
		filters.clear();
	}
	clearBuffers();
}

VideoFilter *VideoFilters::on(const QString &filterName)
{
	VideoFilter *filter = nullptr;
	if (filterName == "PrepareForHWBobDeint")
		filter = new PrepareForHWBobDeint;
	else for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if ((mod.type & 0xF) == Module::VIDEOFILTER && mod.name == filterName)
			{
				filter = (VideoFilter *)module->createInstance(mod.name);
				break;
			}
	if (filter)
		filters.append(filter);
	return filter;
}
void VideoFilters::off(VideoFilter *&videoFilter)
{
	const int idx = filters.indexOf(videoFilter);
	if (idx > -1)
	{
		filters.remove(idx);
		delete videoFilter;
		videoFilter = nullptr;
	}
}

void VideoFilters::clearBuffers()
{
	if (!filters.isEmpty())
	{
		filtersThr.waitForFinished(true);
		for (VideoFilter *vFilter : asConst(filters))
			vFilter->clearBuffer();
	}
	outputQueue.clear();
	outputNotEmpty = false;
}
void VideoFilters::removeLastFromInputBuffer()
{
	if (!filters.isEmpty())
	{
		filtersThr.waitForFinished(true);
		for (int i = filters.count() - 1; i >= 0; --i)
			if (filters[i]->removeLastFromInternalBuffer())
				break;
	}
}

void VideoFilters::addFrame(const VideoFrame &videoFrame, double ts)
{
	const VideoFilter::FrameBuffer frame(videoFrame, ts);
	if (!filters.isEmpty())
		filtersThr.filterFrame(frame);
	else
	{
		outputQueue.enqueue(frame);
		outputNotEmpty = true;
	}
}
bool VideoFilters::getFrame(VideoFrame &videoFrame, TimeStamp &ts)
{
	bool locked, ret;
	if ((locked = !filters.isEmpty()))
		filtersThr.waitForFinished(false);
	if ((ret = !outputQueue.isEmpty()))
	{
		videoFrame = outputQueue.at(0).frame;
		ts = outputQueue.at(0).ts;
		outputQueue.removeFirst();
		outputNotEmpty = !outputQueue.isEmpty();
	}
	if (locked)
		filtersThr.bufferMutex.unlock();
	return ret;
}

bool VideoFilters::readyRead()
{
	filtersThr.waitForFinished(false);
	const bool ret = outputNotEmpty;
	filtersThr.bufferMutex.unlock();
	return ret;
}
