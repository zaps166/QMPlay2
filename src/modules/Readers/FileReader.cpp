#include <FileReader.hpp>

bool FileReader::readyRead() const
{
	return f.isOpen();
}
bool FileReader::canSeek() const
{
	return true;
}

bool FileReader::seek( qint64 _pos, int wh )
{
	switch ( wh )
	{
		case SEEK_SET:
			return f.seek( qAbs( _pos ) );
		case SEEK_CUR:
			return f.seek( pos() + _pos );
		case SEEK_END:
			return f.seek( size() - qAbs( _pos ) );
	}
	return false;
}
QByteArray FileReader::read( qint64 len )
{
	return f.read( len );
}
qint64 FileReader::read( quint8 *buffer, qint64 maxLen )
{
	return f.read( ( char * )buffer, maxLen );
}
QByteArray FileReader::readLine()
{
	QByteArray line = f.readLine();
	return line.left( line.length() - ( line.contains( '\r' ) ? 2 : 1 ) );
}
bool FileReader::atEnd() const
{
	return f.pos() == fSize;
}

qint64 FileReader::size() const
{
	return f.size();
}
qint64 FileReader::pos() const
{
	return f.pos();
}
QString FileReader::name() const
{
	return FileReaderName;
}

FileReader::~FileReader()
{
	f.close();
}

bool FileReader::open()
{
	f.setFileName( getUrl().mid( 7, -1 ) );
	bool OK = f.open( QIODevice::ReadOnly );
	if ( OK )
	{
		fSize = f.size();
		addParam( "Local", true );
	}
	return OK;
}
