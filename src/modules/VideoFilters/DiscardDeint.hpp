#include <DeintFilter.hpp>

class DiscardDeint : public DeintFilter
{
public:
	DiscardDeint();

	void filter(QQueue< FrameBuffer > &framesQueue);

	bool processParams(bool *paramsCorrected);
private:
	int w, h;
};

#define DiscardDeintName "Discard"
