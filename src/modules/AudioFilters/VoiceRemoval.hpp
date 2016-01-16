#include <AudioFilter.hpp>

class VoiceRemoval : public AudioFilter
{
public:
	VoiceRemoval(Module &);

	bool set();
private:
	bool setAudioParameters(uchar, uint);
	double filter(QByteArray &, bool);

	bool enabled, hasParameters, canFilter;
	uchar chn;
};

#define VoiceRemovalName "Voice Removal"
