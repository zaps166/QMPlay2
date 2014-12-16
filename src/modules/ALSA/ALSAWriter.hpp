#include <Writer.hpp>
#include <ALSACommon.hpp>

#include <QCoreApplication>

struct _snd_pcm;

class ALSAWriter : public Writer
{
	Q_DECLARE_TR_FUNCTIONS( ALSAWriter )
public:
	ALSAWriter( Module & );
private:
	~ALSAWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void pause();

	qint64 size() const;
	QString name() const;

	bool open();

	/**/

	void close();

	QString devName;

	QByteArray int_samples;
	unsigned sample_size;
	_snd_pcm *snd;

	double delay;
	unsigned sample_rate, channels;
	bool autoFindMultichannelDevice, err, mustSwapChn, canPause;
};

#define ALSAWriterName "ALSA Writer"
