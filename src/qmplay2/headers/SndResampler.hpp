#ifndef SNDRESAMPLER_HPP
#define SNDRESAMPLER_HPP

#ifndef QMPLAY2_AVRESAMPLE
	#include <QVector>
#endif
#include <stddef.h>

class QByteArray;

class SndResampler
{
public:
	inline SndResampler() :
		snd_convert_ctx( NULL ),
		src_samplerate( 0 ), src_channels( 0 ), dst_samplerate( 0 ), dst_channels( 0 )
	{}
	inline ~SndResampler()
	{
		destroy();
	}

	inline const char *name() const
	{
#ifdef QMPLAY2_AVRESAMPLE
		return "AVResample";
#else
		return "SWResample";
#endif
	}

	inline bool isOpen() const
	{
		return snd_convert_ctx;
	}

	bool create( int _src_samplerate, int _src_channels, int _dst_samplerate, int _dst_channels );
	void convert( const QByteArray &src, QByteArray &dst );
	void destroy();
private:
#ifdef QMPLAY2_AVRESAMPLE
	struct AVAudioResampleContext *snd_convert_ctx;
#else
	struct SwrContext *snd_convert_ctx;
	QVector< int > channel_map;
#endif
	int src_samplerate, src_channels, dst_samplerate, dst_channels;
};

#endif
