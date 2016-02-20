#ifndef FILTERS_HPP
#define FILTERS_HPP

#include <VideoFilter.hpp>

#include <QByteArray>
#include <QVector>
#include <QQueue>
#include <QPair>

class TimeStamp;

class VideoFilters
{
public:
	static void init();

	static inline void averageTwoLines(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
	{
		averageTwoLinesPtr(dest, src1, src2, linesize);
	}

	inline VideoFilters() :
		hasFilters(false),
		outputNotEmpty(false)
	{}
	inline ~VideoFilters()
	{
		clear();
	}

	void clear();

	VideoFilter *on(const QString &filterName);
	void off(VideoFilter *&videoFilter);

	void clearBuffers();
	void removeLastFromInputBuffer();

	void addFrame(const VideoFrame &videoFrame, double ts);
	bool getFrame(VideoFrame &videoFrame, TimeStamp &ts);

	inline bool readyToRead() const
	{
		return outputNotEmpty;
	}
private:
	static void (*averageTwoLinesPtr)(quint8 *, const quint8 *, const quint8 *, int);

	QQueue< VideoFilter::FrameBuffer > outputQueue;
	QVector< VideoFilter * > videoFilters;
	bool hasFilters, outputNotEmpty;
};

#endif
