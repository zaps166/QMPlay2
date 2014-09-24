#include <Reader.hpp>

#ifdef QT_NO_OPENSSL
	#include <QTcpSocket>
#else
	#include <QSslSocket>
#endif

class MySocket : public
#ifdef QT_NO_OPENSSL
		QTcpSocket
#else
		QSslSocket
#endif
{
public:
	inline MySocket() :
		timerId( -1 ), readBS( -1 ) {}

	inline void startTimer( quint32 maxTimeout )
	{
		readBS = readBufferSize();
		timerId = QTcpSocket::startTimer( maxTimeout );
	}
	inline void killTimer()
	{
		QTcpSocket::killTimer( timerId );
		if ( readBufferSize() != readBS )
			setReadBufferSize( readBS );
		timerId = -1;
		readBS = -1;
	}
	inline bool isTimerActive()
	{
		return timerId != -1;
	}
private:
	void timerEvent( QTimerEvent * )
	{
		if ( readBufferSize() <= size() )
			setReadBufferSize( size() + 64 );
	}

	int timerId;
	qint64 readBS;
};

/**/

class HttpReader : public Reader
{
public:
	HttpReader( Module & );
private:
	~HttpReader();

	bool set();

	bool readyRead() const;
	bool canSeek() const;

	bool seek( qint64, int wh = SEEK_SET );
	QByteArray read( qint64 );
	void pause();
	bool atEnd() const;
	void abort();

	qint64 size() const;
	qint64 pos() const;
	QString name() const;

	bool open();

	/**/

	void close();

	bool conn( quint8, qint64 range = 0 );
	QByteArray read( bool, qint64 ) const;
	void disconn();

	MySocket sock;

	qint32 maxTimeout;
	qint64 _size, _pos;
	int chunkSize;
	const quint8 followLocation;
	bool _abort, _canSeek, chunked;
	mutable bool paused;
};

#define HttpReaderName "Http Reader"
