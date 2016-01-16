#include <AudioFilter.hpp>

class Echo : public AudioFilter
{
public:
	Echo(Module &);

	bool set();
private:
	bool setAudioParameters(uchar, uint);
	double filter(QByteArray &, bool);

	void alloc(bool);

	bool enabled, hasParameters, canFilter;

	uint echo_delay, echo_volume, echo_repeat;
	bool echo_surround;

	uchar chn;
	uint srate;

	int w_ofs;
	QVector< float > sampleBuffer;
};

#define EchoName "Echo"
