#include <DeintFilter.hpp>

class BlendDeint : public DeintFilter
{
public:
	BlendDeint();

	bool filter(QQueue<FrameBuffer> &framesQueue);

	bool processParams(bool *paramsCorrected);
private:
	int w, h;
};

#define BlendDeintName "Blend"
