#include <AudioFilter.hpp>

struct FFTContext;
struct FFTComplex;

class Equalizer : public AudioFilter
{
public:
	static QVector< float > interpolate(const QVector< float > &, const int);
	static QVector< float > freqs(Settings &);

	Equalizer(Module &);
	~Equalizer();

	bool set();
private:
	bool setAudioParameters(uchar, uint);
	int bufferedSamples() const;
	void clearBuffers();
	double filter(Buffer &data, bool flush);

	/**/

	void alloc(bool);
	void interpolateFilterCurve();

	int FFT_NBITS, FFT_SIZE, FFT_SIZE_2;

	uchar chn;
	uint srate;
	bool canFilter, hasParameters, enabled;

	mutable QMutex mutex;
	FFTContext *fftIn, *fftOut;
	FFTComplex *complex;
	QVector< QVector< float > > input, last_samples;
	QVector< float > wind_f, f;
	float preamp;
};

#define EqualizerName "Audio Equalizer"
