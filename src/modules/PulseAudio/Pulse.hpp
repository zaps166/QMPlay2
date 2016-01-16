#ifndef PULSE_HPP
#define PULSE_HPP

#include <QByteArray>

#include <pulse/simple.h>
#include <pulse/pulseaudio.h>

class Pulse
{
public:
	Pulse();
	inline ~Pulse()
	{
		stop();
	}

	inline bool isOK() const
	{
		return _isOK;
	}
	inline bool isOpen() const
	{
		return pulse;
	}

	bool start();
	void stop();

	bool write(const QByteArray &);

	double delay;
	uchar channels;
	uint sample_rate;
private:
	bool _isOK;

	pa_simple *pulse;
	pa_sample_spec ss;
};

#endif //PULSE_HPP
