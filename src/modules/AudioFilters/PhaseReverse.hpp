#include <AudioFilter.hpp>

class PhaseReverse : public AudioFilter
{
public:
	PhaseReverse( Module & );

	bool set();
private:
	bool setAudioParameters( uchar, uint );
	double filter( QByteArray &, bool );

	bool enabled, hasParameters, canFilter, reverseRight;
	uchar chn;
};

#define PhaseReverseName "Phase Reverse"
