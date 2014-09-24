#include <VideoFilter.hpp>

class MotionBlur : public VideoFilter
{
public:
	MotionBlur();

	void filter( QQueue< FrameBuffer > &framesQueue );

	bool processParams( bool *paramsCorrected );
private:
	int w, h;
};

#define MotionBlurName "Motion Blur"
