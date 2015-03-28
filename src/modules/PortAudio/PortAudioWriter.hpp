#include <Writer.hpp>

#include <PortAudioCommon.hpp>
#include <portaudio.h>

#include <QCoreApplication>

class PortAudioWriter : public Writer
{
	Q_DECLARE_TR_FUNCTIONS( PortAudioWriter )
public:
	PortAudioWriter( Module & );
private:
	~PortAudioWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void pause();

	QString name() const;

	bool open();

	/**/

	void close();

	PaStreamParameters outputParameters;
	PaStream *stream;
	int sample_rate;
	double outputLatency;
	bool err;
};

#define PortAudioWriterName "PortAudio Writer"
