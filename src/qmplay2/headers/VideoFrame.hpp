#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <Buffer.hpp>
#include <PixelFormats.hpp>

class VideoFrameSize
{
	friend class VideoFrame;
public:
	inline VideoFrameSize(qint32 width, qint32 height, quint8 chromaShiftW, quint8 chromaShiftH) :
		width(width), height(height),
		chromaShiftW(chromaShiftW), chromaShiftH(chromaShiftH)
	{}
	inline VideoFrameSize()
	{
		clear();
	}

	qint32 chromaWidth() const;
	qint32 chromaHeight() const;

	inline qint32 getWidth(qint32 plane) const
	{
		return plane ? chromaWidth() : width;
	}
	inline qint32 getHeight(qint32 plane) const
	{
		return plane ? chromaHeight() : height;
	}

	QMPlay2PixelFormat getFormat() const;

	void clear();

	qint32 width, height;
	quint8 chromaShiftW, chromaShiftH;
};

/**/

class VideoFrame
{
public:
	VideoFrame(const VideoFrameSize &size, AVBufferRef *bufferRef[], const qint32 newLinesize[], bool interlaced, bool tff);
	VideoFrame(const VideoFrameSize &size, const qint32 newLinesize[], bool interlaced = false, bool tff = false);
	VideoFrame(quintptr surfaceId, bool interlaced, bool tff);
	VideoFrame();

	inline bool hasNoData() const
	{
		return buffer[0].isEmpty();
	}
	inline bool isEmpty() const
	{
		return hasNoData() && surfaceId == 0;
	}

	inline void setNoInterlaced()
	{
		interlaced = tff = false;
	}

	void clear();

	void copy(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const;

	VideoFrameSize size;
	Buffer buffer[3];
	qint32 linesize[3];
	bool interlaced, tff;
	quintptr surfaceId;
};

#endif
