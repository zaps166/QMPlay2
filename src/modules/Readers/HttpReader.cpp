#include <HttpReader.hpp>

#include <Functions.hpp>

#include <QCoreApplication>
#include <QUrl>

HttpReader::HttpReader( Module &module ) :
	_size( 0 ), _pos( 0 ),
	followLocation( 5 ),
	_abort( false )
{
	SetModule( module );
	sock.setReadBufferSize( 0x200000 ); //2MiB
}
HttpReader::~HttpReader()
{
	close();
}

bool HttpReader::set()
{
	maxTimeout = sets().getInt( "Http/TCPTimeout" ) * 1000;
	return sets().getBool( "Http/ReaderEnabled" );
}

bool HttpReader::readyRead() const
{
	return !_abort && ( sock.state() == QAbstractSocket::ConnectedState || sock.size() );
}
bool HttpReader::canSeek() const
{
	return _size > 0 && _canSeek;
}

bool HttpReader::seek( qint64 position, int wh )
{
	if ( !canSeek() )
		return false;
	switch ( wh )
	{
		case SEEK_SET:
			position = qAbs( position );
			break;
		case SEEK_CUR:
			position = _pos + position;
			break;
		case SEEK_END:
			position = _size - qAbs( position );
			break;
		default:
			return false;
	}
	if ( position == _pos )
		return true;
	if ( !conn( followLocation, position ) )
	{
		close();
		return false;
	}
	return true;
}
QByteArray HttpReader::read( qint64 len )
{
	if ( len <= 0 )
		return QByteArray();
	if ( sock.isTimerActive() )
		sock.killTimer();
	if ( sock.readBufferSize() < len )
		sock.setReadBufferSize( len );
	while ( sock.size() < len && !( _size > -1 && _pos + sock.size() >= _size ) )
		if ( !waitForSocketReadyRead( maxTimeout ) )
			break;
	if ( _abort )
		return QByteArray();
	QByteArray arr = sock.read( len );
	_pos += arr.size();
	if ( chunked )
	{
		QByteArray arr_ret;
		do
		{
			if ( !chunkSize )
			{
				int rnIdx = arr.indexOf( "\r\n" );
				if ( rnIdx > -1 )
				{
					chunkSize = arr.mid( 0, rnIdx ).toInt( NULL, 16 );
					arr.remove( 0, rnIdx + 2 );
				}
			}
			if ( chunkSize >= arr.size() )
			{
				chunkSize -= arr.size();
				arr_ret += arr;
				break;
			}
			arr_ret += arr.left( chunkSize );
			arr.remove( 0, chunkSize + 2 );
			chunkSize = 0;
		} while ( !arr.isEmpty() );
		return arr_ret;
	}
	return arr;
}
void HttpReader::pause()
{
	sock.startTimer( maxTimeout );
}
bool HttpReader::atEnd() const
{
	return _pos == _size;
}
void HttpReader::abort()
{
	_abort = true;
}

qint64 HttpReader::size() const
{
	return _size;
}
qint64 HttpReader::pos() const
{
	return _pos;
}
QString HttpReader::name() const
{
	return HttpReaderName;
}

bool HttpReader::open()
{
	return conn( followLocation );
}

/**/

void HttpReader::close()
{
	disconn();
	_size = 0;
}

bool HttpReader::waitForSocketReadyRead( int wd )
{
	while ( !_abort && wd > 0 && !sock.waitForReadyRead( 100 ) )
		wd -= 100;
	return !( _abort || wd <= 0 );
}

bool HttpReader::conn( quint8 followLocation, qint64 range )
{
	disconn();

	bool isHttps =
#ifdef QT_NO_OPENSSL
			false;
#else
			_url.left( 6 ) == "https:";
#endif
	if ( _url.left( 5 ) != "http:" && !isHttps )
		return false;

	if ( _url.right( 1 ) == "/" )
		_url.chop( 1 );

	QUrl url = _url;
	quint16 port = url.port() == -1 ? ( isHttps ? 443 : 80 ) : url.port();
#ifndef QT_NO_OPENSSL
	if ( isHttps )
		sock.connectToHostEncrypted( url.host(), port );
	else
#endif
		sock.connectToHost( url.host(), port );

	qint32 tmpTimeout = maxTimeout;
	while ( !_abort && tmpTimeout > 0 && sock.state() != QAbstractSocket::UnconnectedState && sock.state() != QAbstractSocket::ConnectedState )
	{
		qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
		Functions::s_wait( 0.01 );
		tmpTimeout -= 10;
	}
	if ( _abort || sock.state() != QAbstractSocket::ConnectedState )
		return false;

	QString path;
	if ( url.path().isEmpty() )
		path = "/";
	else
		path = _url.mid( _url.indexOf( url.path(), _url.indexOf( url.host() ) ), -1 );
	QByteArray query;
	query += "GET " + path + " HTTP/1.1\r\n";
	query += "Host: " + url.host() + ( port == 80 ? "" : ":" + QString::number( port ) ) + "\r\n";
	query += "User-Agent: QMPlay2\r\n";
	if ( range )
		query += "Range: bytes=" + QString::number( range ) + "-\r\n";
	query += "Connection: close\r\n";
	query += "\r\n";
	sock.write( query );

	QByteArray header, headerLower;
	while ( !_abort )
	{
		if ( !waitForSocketReadyRead( maxTimeout ) )
			break;
		bool br = false;
		while ( sock.canReadLine() )
		{
			QByteArray arr = sock.readLine();
			if ( arr == "\r\n" || arr == "\n" || arr == "\n\r" )
			{
				br = true;
				break;
			}
			else
			{
				header += arr;
				headerLower += arr.toLower();
			}
		}
		if ( br )
			break;
		if ( sock.readBufferSize() <= sock.size() ) //zabezpieczenie przed za małym buforem
			sock.setReadBufferSize( sock.size()+1 );
	}

	_canSeek = headerLower.contains( "accept-ranges: bytes" ) || headerLower.contains( "content-range: bytes" );
	if ( !_canSeek && range )
		return false;

	chunked = headerLower.contains( "transfer-encoding: chunked" );
	chunkSize = 0;

	if ( followLocation )
	{
		int idx = headerLower.indexOf( "location: " );
		if ( idx > -1 )
		{
			idx += 10;
			int idx2 = header.indexOf( '\n', idx );
			if ( idx2 > -1 )
			{
				_url = header.mid( idx, idx2 - idx );
				_url.remove( '\r' );
				return conn( followLocation - 1, range );
			}
		}
	}

	int idx = headerLower.indexOf( "content-length: " );
	if ( idx > -1 )
	{
		if ( _size <= 0 )
		{
			idx += 16;
			int idx2 = header.indexOf( '\n', idx );
			if ( idx2 > -1 )
				_size = QString( header.mid( idx, idx2 - idx ) ).remove( '\r' ).toLongLong();
		}
	}
	else
		_size = -1;

	_pos = range;

	return true;
}
void HttpReader::disconn()
{
	sock.close();
	_pos = 0;
}
