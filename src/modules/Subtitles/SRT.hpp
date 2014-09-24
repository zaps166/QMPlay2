#include <SubsDec.hpp>

class SRT : public SubsDec
{
	bool toASS( const QByteArray &, class libASS *, double );

	~SRT() {}
};

#define SRTSubsName "SRT Subtitles"
