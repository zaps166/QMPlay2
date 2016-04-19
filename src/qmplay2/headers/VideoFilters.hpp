#ifndef FILTERS_HPP
#define FILTERS_HPP

#include <VideoFilter.hpp>

#include <QWaitCondition>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QQueue>

class VideoFilters;
class TimeStamp;

class VideoFiltersThr : public QThread
{
public:
	VideoFiltersThr(VideoFilters &videoFilters);
	inline ~VideoFiltersThr()
	{
		stop();
	}

	void start();
	void stop();

	void filterFrame(const VideoFilter::FrameBuffer &frame);

	void waitForFinished(bool waitForAllFrames);

	QMutex bufferMutex;
private:
	void run();

	VideoFilters &videoFilters;

	bool br, filtering;

	QWaitCondition cond;
	QMutex mutex;

	VideoFilter::FrameBuffer frameToFilter;
};

class VideoFilters
{
	friend class VideoFiltersThr;
public:
	static void init();

	static inline void averageTwoLines(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
	{
		averageTwoLinesPtr(dest, src1, src2, linesize);
	}

	inline VideoFilters() :
		filtersThr(*this),
		outputNotEmpty(false)
	{}
	inline ~VideoFilters()
	{
		clear();
	}

	void start();
	void clear();

	VideoFilter *on(const QString &filterName);
	void off(VideoFilter *&videoFilter);

	void clearBuffers();
	void removeLastFromInputBuffer();

	void addFrame(const VideoFrame &videoFrame, double ts);
	bool getFrame(VideoFrame &videoFrame, TimeStamp &ts);

	bool readyRead();
private:
	static void (*averageTwoLinesPtr)(quint8 *, const quint8 *, const quint8 *, int);

	QQueue<VideoFilter::FrameBuffer> outputQueue;
	QVector<VideoFilter *> filters;
	VideoFiltersThr filtersThr;
	bool outputNotEmpty;
};

#endif
