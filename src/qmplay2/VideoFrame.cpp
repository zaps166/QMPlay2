#include <VideoFrame.hpp>
#include <Functions.hpp>

void VideoFrame::copyYV12(void *dest, const VideoFrame &videoFrame, int luma_width, int chroma_width, int height)
{
	int offset = 0;
	quint8 *dest_data = (quint8 *)dest;
	for (int i = 0; i < height; ++i)
	{
		memcpy(dest_data, videoFrame.buffer[0].data() + offset, luma_width);
		offset += videoFrame.linesize[0];
		dest_data += luma_width;
	}
	offset = 0;
	height >>= 1;
	const int wh = chroma_width * height;
	for (int i = 0; i < height; ++i)
	{
		memcpy(dest_data, videoFrame.buffer[2].data() + offset, chroma_width);
		memcpy(dest_data + wh, videoFrame.buffer[1].data() + offset, chroma_width);
		offset += videoFrame.linesize[1];
		dest_data += chroma_width;
	}
}

VideoFrame::VideoFrame(int height, int chromaHeight, AVBufferRef *bufferRef[], const int newLinesize[], bool interlaced, bool tff) :
	surfaceId(0),
	interlaced(interlaced),
	tff(tff)
{
	for (int i = 0; i < 3 && bufferRef[i]; ++i)
	{
		linesize[i] = newLinesize[i];
		buffer[i].assign(bufferRef[i], linesize[i] * ((i == 1 || i == 2) ? chromaHeight : height));
		bufferRef[i] = NULL;
	}
}
VideoFrame::VideoFrame(int height, int chromaHeight, const int newLinesize[], bool interlaced, bool tff) :
	surfaceId(0),
	interlaced(interlaced),
	tff(tff)
{
	for (int i = 0; i < 3; ++i)
	{
		linesize[i] = newLinesize[i];
		buffer[i].resize(linesize[i] * ((i == 1 || i == 2) ? chromaHeight : height));
	}
}
VideoFrame::VideoFrame(quintptr surfaceId, bool interlaced, bool tff) :
	surfaceId(surfaceId),
	interlaced(interlaced),
	tff(tff)
{
	for (int i = 0; i < 3; ++i)
		linesize[i] = 0;
}

void VideoFrame::clear()
{
	for (int i = 0; i < 3; ++i)
	{
		buffer[i].clear();
		linesize[i] = 0;
	}
	setNoInterlaced();
	surfaceId = 0;
}
