#include <VideoFrame.hpp>
#include <Functions.hpp>

extern "C"
{
	#include <libavutil/common.h>
}

VideoFrameSize::VideoFrameSize(qint32 width, qint32 height, qint32 chromaShiftW, qint32 chromaShiftH) :
	width(width), height(height),
	chromaWidth(FF_CEIL_RSHIFT(width, chromaShiftW)), chromaHeight(FF_CEIL_RSHIFT(height, chromaShiftH))
{}

void VideoFrameSize::clear()
{
	width = height = 0;
	chromaWidth = chromaHeight = 0;
}

/**/

VideoFrame::VideoFrame(const VideoFrameSize &size, AVBufferRef *bufferRef[], const qint32 newLinesize[], bool interlaced, bool tff) :
	size(size),
	interlaced(interlaced),
	tff(tff),
	surfaceId(0)
{
	for (qint32 p = 0; p < 3 && bufferRef[p]; ++p)
	{
		linesize[p] = newLinesize[p];
		buffer[p].assign(bufferRef[p], linesize[p] * size.getHeight(p));
		bufferRef[p] = NULL;
	}
}
VideoFrame::VideoFrame(const VideoFrameSize &size, const qint32 newLinesize[], bool interlaced, bool tff) :
	size(size),
	interlaced(interlaced),
	tff(tff),
	surfaceId(0)
{
	for (qint32 p = 0; p < 3; ++p)
	{
		linesize[p] = newLinesize[p];
		buffer[p].resize(linesize[p] * size.getHeight(p));
	}
}
VideoFrame::VideoFrame(quintptr surfaceId, bool interlaced, bool tff) :
	interlaced(interlaced),
	tff(tff),
	surfaceId(surfaceId)
{
	for (qint32 i = 0; i < 3; ++i)
		linesize[i] = 0;
}
VideoFrame::VideoFrame() :
	interlaced(false),
	tff(false),
	surfaceId(0)
{
	for (qint32 i = 0; i < 3; ++i)
		linesize[i] = 0;
}

void VideoFrame::clear()
{
	for (qint32 i = 0; i < 3; ++i)
	{
		buffer[i].clear();
		linesize[i] = 0;
	}
	setNoInterlaced();
	surfaceId = 0;
	size.clear();
}

void VideoFrame::copy(void *dest, qint32 luma_width, qint32 chroma_width, qint32 height) const
{
	qint32 offset = 0;
	quint8 *dest_data = (quint8 *)dest;
	for (qint32 i = 0; i < height; ++i)
	{
		memcpy(dest_data, buffer[0].data() + offset, luma_width);
		offset += linesize[0];
		dest_data += luma_width;
	}
	offset = 0;
	height >>= 1;
	const qint32 wh = chroma_width * height;
	for (qint32 i = 0; i < height; ++i)
	{
		memcpy(dest_data, buffer[2].data() + offset, chroma_width);
		memcpy(dest_data + wh, buffer[1].data() + offset, chroma_width);
		offset += linesize[1];
		dest_data += chroma_width;
	}
}
