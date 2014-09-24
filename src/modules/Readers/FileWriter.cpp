#include <FileWriter.hpp>

bool FileWriter::readyWrite() const
{
	return f.isOpen();
}

qint64 FileWriter::write( const QByteArray &arr )
{
	return f.write( arr );
}

qint64 FileWriter::size() const
{
	return f.size();
}
QString FileWriter::name() const
{
	return FileWriterName;
}

FileWriter::~FileWriter()
{
	f.close();
}

bool FileWriter::open()
{
	f.setFileName( url().mid( 7, -1 ) );
	return f.open( QIODevice::WriteOnly );
}
