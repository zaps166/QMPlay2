#include <DeintFilter.hpp>

class BobDeint : public DeintFilter
{
public:
	BobDeint();

	bool filter(QQueue<FrameBuffer> &framesQueue);

	bool processParams(bool *paramsCorrected);
private:
	bool secondFrame;
	int w, h;
};

#define BobDeintName "Bob"
