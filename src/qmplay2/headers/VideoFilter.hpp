#ifndef VIDEOFILTER_HPP
#define VIDEOFILTER_HPP

#include <ModuleParams.hpp>
#include <VideoFrame.hpp>

#include <QQueue>

class VideoFilter : public ModuleParams
{
public:
	class FrameBuffer
	{
	public:
		inline FrameBuffer() :
			ts(0.0)
		{}
		inline FrameBuffer(const VideoFrame &frame, double ts) :
			frame(frame),
			ts(ts)
		{}

		VideoFrame frame;
		double ts;
	};

	virtual ~VideoFilter();

	inline void clearBuffer()
	{
		internalQueue.clear();
	}

	bool removeLastFromInternalBuffer();

	virtual bool filter(QQueue<FrameBuffer> &framesQueue) = 0;
protected:
	int addFramesToInternalQueue(QQueue<FrameBuffer> &framesQueue);

	inline double halfDelay(double f1_ts, double f2_ts) const
	{
		return (f1_ts - f2_ts) / 2.0;
	}

	QQueue<FrameBuffer> internalQueue;
};

#endif
