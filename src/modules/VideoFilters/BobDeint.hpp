#include <DeintFilter.hpp>

class BobDeint : public DeintFilter
{
public:
	BobDeint();

	void filter( QQueue< FrameBuffer > &framesQueue );

	bool processParams( bool *paramsCorrected );
private:
	int w, h;
};

#define BobDeintName "Bob"
