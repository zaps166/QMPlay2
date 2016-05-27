#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <Buffer.hpp>

class VideoFrameSize
{
public:
	VideoFrameSize(qint32 width, qint32 height, qint32 chromaShiftW, qint32 chromaShiftH);
	inline VideoFrameSize()
	{
		clear();
	}

	void clear();

	inline qint32 getWidth(qint32 plane = 0) const
	{
		return plane ? chromaWidth : width;
	}
	inline qint32 getHeight(qint32 plane = 0) const
	{
		return plane ? chromaHeight : height;
	}

	qint32 width, height;
	qint32 chromaWidth, chromaHeight;
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

	void copy(void *dest, qint32 luma_width, qint32 chroma_width, qint32 height) const;

	VideoFrameSize size;
	Buffer buffer[3];
	qint32 linesize[3];
	bool interlaced, tff;
	quintptr surfaceId;
};

#endif
