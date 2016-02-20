#include <AudioFilter.hpp>

class Buffer;

class PhaseReverse : public AudioFilter
{
public:
	PhaseReverse(Module &);

	bool set();
private:
	bool setAudioParameters(uchar, uint);
	double filter(Buffer &, bool);

	bool enabled, hasParameters, canFilter, reverseRight;
	uchar chn;
};

#define PhaseReverseName "Phase Reverse"
