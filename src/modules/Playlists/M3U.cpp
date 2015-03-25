#include <M3U.hpp>

#include <Functions.hpp>
using Functions::filePath;
using Functions::Url;

#include <Reader.hpp>
#include <Writer.hpp>

QList< M3U::Entry > M3U::_read()
{
	Reader *reader = ioCtrl.rawPtr< Reader >();
	QList< Entry > list;

	QString playlistPath = filePath( reader->getUrl() );
	if ( playlistPath.left( 7 ) == "file://" )
		playlistPath.remove( "file://" );
	else
		playlistPath.clear();
	bool noReadLine = false;

	QByteArray line;
	QList< QByteArray > playlistLines = readLines();
	while ( !playlistLines.isEmpty() )
	{
		QStringList splitLine;

		if ( noReadLine )
			noReadLine = false;
		else
			line = playlistLines.takeFirst();

		if ( line.isEmpty() )
			continue;

		if ( line.left( 8 ) == "#EXTINF:" )
		{
			line.remove( 0, 8 );
			int idx = line.indexOf( ',' );
			if ( idx < 0 )
				continue;
			splitLine += line.left( idx );
			line.remove( 0, idx+1 );
			splitLine += line;
			line = playlistLines.takeFirst();
			if ( line.isEmpty() )
				continue;
			if ( line.left( 8 ) == "#EXTINF:" )
			{
				noReadLine = true;
				continue;
			}
			Entry entry;
			entry.length = splitLine[ 0 ].toInt();
			entry.name = splitLine[ 1 ].replace( '\001', '\n' );
			entry.url = Url( line, playlistPath );
			list += entry;
		}
	}

	return list;
}
bool M3U::_write( const QList< Entry > &list )
{
	Writer *writer = ioCtrl.rawPtr< Writer >();
	writer->write( "#EXTM3U\r\n" );
	for ( int i = 0 ; i < list.size() ; i++ )
	{
		const Entry &entry = list[ i ];
		if ( !entry.GID )
		{
			QString length = QString::number( entry.length );
			QString url = entry.url;
#ifdef Q_OS_WIN
			bool isFile = url.left( 5 ) == "file:";
#endif
			url.remove( "file://" );
#ifdef Q_OS_WIN
			if ( isFile )
				url.replace( "/", "\\" );
#endif
			writer->write( QString( "#EXTINF:" + length + "," + QString( entry.name ).replace( '\n', '\001' ) + "\r\n" + url + "\r\n" ).toUtf8() );
		}
	}
	return true;
}
