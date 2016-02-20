#include <AudioFilter.hpp>

class Buffer;

class VoiceRemoval : public AudioFilter
{
public:
	VoiceRemoval(Module &);

	bool set();
private:
	bool setAudioParameters(uchar, uint);
	double filter(Buffer &, bool);

	bool enabled, hasParameters, canFilter;
	uchar chn;
};

#define VoiceRemovalName "Voice Removal"
