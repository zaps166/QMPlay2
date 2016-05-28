#include <VideoFilter.hpp>

class MotionBlur : public VideoFilter
{
public:
	MotionBlur();

	bool filter(QQueue<FrameBuffer> &framesQueue);

	bool processParams(bool *paramsCorrected);
};

#define MotionBlurName "Motion Blur"
