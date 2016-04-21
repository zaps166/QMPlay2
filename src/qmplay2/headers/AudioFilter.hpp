#ifndef AUDIOFILTER_HPP
#define AUDIOFILTER_HPP

#include <ModuleCommon.hpp>

#include <QVector>

class Buffer;

class AudioFilter : public ModuleCommon
{
public:
	static QVector<AudioFilter *> open();

	virtual bool setAudioParameters(uchar chn, uint srate) = 0;
	virtual int bufferedSamples() const {return 0;}
	virtual void clearBuffers() {}
	virtual double filter(Buffer &data, bool flush = false) = 0; //returns delay in [s]

	virtual ~AudioFilter() {}
};

#endif
