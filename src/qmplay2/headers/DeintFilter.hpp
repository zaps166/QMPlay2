#ifndef DEINT_FILTER_HPP
#define DEINT_FILTER_HPP

#include <VideoFilter.hpp>
#include <VideoFrame.hpp>

class DeintFilter : public VideoFilter
{
public:
	enum DeintFlags {AutoDeinterlace = 0x1, DoubleFramerate = 0x2, AutoParity = 0x4, TopFieldFirst = 0x8};

	inline DeintFilter()
	{
		addParam("DeinterlaceFlags");
	}
protected:
	int addFramesToDeinterlace(QQueue< FrameBuffer > &framesQueue, bool checkSize = true);

	inline bool isTopFieldFirst(const VideoFrame *videoFrame) const
	{
		return ((deintFlags & AutoParity) && videoFrame->interlaced) ? videoFrame->top_field_first : deintFlags & TopFieldFirst;
	}

	quint8 deintFlags;
};

#endif
