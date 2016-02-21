#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <Buffer.hpp>

class VideoFrame
{
public:
	VideoFrame(int height, int chromaHeight, AVBufferRef *bufferRef[], const int newLinesize[], bool interlaced, bool tff);
	VideoFrame(int height, int chromaHeight, const int newLinesize[], bool interlaced = false, bool tff = false);
	VideoFrame(quintptr surfaceId, bool interlaced, bool tff);
	inline VideoFrame()
	{
		clear();
	}

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

	void copy(void *dest, int luma_width, int chroma_width, int height) const;

	Buffer buffer[3];
	int linesize[3];
	quintptr surfaceId;
	bool interlaced, tff;
};

#endif
