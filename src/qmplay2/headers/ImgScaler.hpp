#ifndef IMGSCALER_HPP
#define IMGSCALER_HPP

#include <stddef.h>

/* YUV planar to RGB32 */

class VideoFrameSize;
struct SwsContext;
class VideoFrame;

class ImgScaler
{
public:
	ImgScaler();
	inline ~ImgScaler()
	{
		destroy();
	}

	bool create(const VideoFrameSize &size, int newWdst, int newHdst);
	void scale(const VideoFrame &videoFrame, void *dst = NULL);
	void scale(const void *src, const int srcLinesize[], int HChromaSrc, void *dst);
	void destroy();
private:
	SwsContext *img_convert_ctx;
	int Hsrc, dstLinesize;
};

#endif
