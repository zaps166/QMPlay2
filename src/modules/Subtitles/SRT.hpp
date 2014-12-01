#include <SubsDec.hpp>

class SRT : public SubsDec
{
	bool toASS( const QByteArray &, class LibASS *, double );

	~SRT() {}
};

#define SRTSubsName "SRT Subtitles"
