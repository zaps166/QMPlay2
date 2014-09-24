#include <SubsDec.hpp>

class Classic : public SubsDec
{
public:
	Classic( bool, double );
private:
	bool toASS( const QByteArray &, class libASS *, double );

	~Classic() {}

	/**/

	bool Use_mDVD_FPS;
	double Sub_max_s;
};

#define ClassicSubsName "Classic Subtitles"
