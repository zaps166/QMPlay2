#ifndef IMGSCALER_HPP
#define IMGSCALER_HPP

#include <stddef.h>

/* YV12 to RGB32 */

struct SwsContext;
class VideoFrame;

class ImgScaler
{
public:
	inline ImgScaler() :
		img_convert_ctx(0),
		Wsrc(0), Hsrc(0), Wdst(0), Hdst(0),
		arr(NULL)
	{}
	inline ~ImgScaler()
	{
		destroy();
	}

	inline void *array() const
	{
		return arr;
	}

	bool create(int _Wsrc, int _Hsrc, int _Wdst, int _Hdst);
	bool createArray(const size_t bytes);
	void scale(const VideoFrame *videoFrame, void *dst = NULL);
	void scale(const void *src, void *dst = NULL);
	void destroy();
private:
	SwsContext *img_convert_ctx;
	int Wsrc, Hsrc, Wdst, Hdst;
	void *arr;
};

#endif
