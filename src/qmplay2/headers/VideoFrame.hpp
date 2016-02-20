#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <Buffer.hpp>

class VideoFrame
{
public:
	static void copyYV12(void *dest, const VideoFrame &videoFrame, int luma_width, int chroma_width, int height); //Use only on YUV420 frames!

	VideoFrame(int height, int chromaHeight, AVBufferRef *bufferRef[], const int newLinesize[], bool interlaced, bool tff);
	VideoFrame(int height, int chromaHeight, const int newLinesize[], bool interlaced = false, bool tff = false);
	VideoFrame(quintptr surfaceId, bool interlaced, bool tff);
	inline VideoFrame()
	{
		clear();
	}

	bool isEmpty() const
	{
		return buffer[0].isEmpty() && surfaceId == 0;
	}
	bool hasNoData() const
	{
		return buffer[0].isEmpty();
	}

	inline void setNoInterlaced()
	{
		interlaced = tff = false;
	}

	void clear();

	Buffer buffer[3];
	int linesize[3];
	quintptr surfaceId;
	bool interlaced, tff;
};

#endif
