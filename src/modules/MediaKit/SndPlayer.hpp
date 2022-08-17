#ifndef PULSE_HPP
#define PULSE_HPP

#include <QByteArray>

#include <SoundPlayer.h>

#include <Window.h>
#include <View.h>
#include <TextControl.h>

#include "RingBuffer.hpp"

class SndPlayer
{
public:
	SndPlayer();
	inline ~SndPlayer()
	{
		stop();
	}

	inline bool isOK() const
	{
		return _isOK;
	}
	inline bool isOpen() const
	{
		return player;
	}

	bool start();
	void stop();

	double getLatency();

	bool write( const QByteArray & );

	double delay;
	uchar channels;
	float sample_rate;
		
private:
	bool _isOK;
	BSoundPlayer *player;
	RingBuffer *ring;
};

#endif
