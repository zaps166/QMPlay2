#include <Writer.hpp>
#include <SndPlayer.hpp>

#include <QCoreApplication>

class MediaKitWriter : public Writer
{
	Q_DECLARE_TR_FUNCTIONS( MediaKitWriter )
public:
	MediaKitWriter( Module & );
private:
	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );

	qint64 size() const;
	QString name() const;

	bool open();

	/**/

	SndPlayer player;
	bool err;
};

#define MediaKitWriterName "MediaKit Writer"
