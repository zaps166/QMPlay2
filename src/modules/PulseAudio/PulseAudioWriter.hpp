#include <Writer.hpp>
#include <pulse.hpp>

#include <QCoreApplication>

class PulseAudioWriter : public Writer
{
	Q_DECLARE_TR_FUNCTIONS( PulseAudioWriter )
public:
	PulseAudioWriter( Module & );
private:
	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );

	qint64 size() const;
	QString name() const;

	bool open();

	/**/

	Pulse pulse;
	bool err;
};

#define PulseAudioWriterName "PulseAudio Writer"
