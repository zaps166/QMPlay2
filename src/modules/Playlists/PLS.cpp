#include <PLS.hpp>

#include <Functions.hpp>
using Functions::filePath;
using Functions::Url;

#include <Reader.hpp>
#include <Writer.hpp>

QList< PLS::Entry > PLS::_read()
{
	Reader *reader = ioCtrl.rawPtr< Reader >();
	QList< Entry > list;

	QString playlistPath = filePath( reader->getUrl() );
	if ( playlistPath.left( 7 ) == "file://" )
		playlistPath.remove( "file://" );
	else
		playlistPath.clear();

	const QList< QByteArray > playlistLines = readLines();
	for ( int i = 0 ; i < playlistLines.count() ; ++i )
	{
		const QByteArray &line = playlistLines[ i ];
		if ( line.isEmpty() )
			continue;
		int idx = line.indexOf( '=' );
		if ( idx < 0 )
			continue;

		int number_idx = -1;
		for ( int i = 0 ; i < line.length() ; ++i )
		{
			if ( line[ i ] == '=' )
				break;
			if ( line[ i ] >= '0' && line[ i ] <= '9' )
			{
				number_idx = i;
				break;
			}
		}
		if ( number_idx == -1 )
			continue;

		QByteArray key = line.left( number_idx );
		QByteArray value = line.mid( idx+1 );
		int entry_idx = line.mid( number_idx, idx - number_idx ).toInt() - 1;

		prepareList( &list, entry_idx );
		if ( entry_idx < 0 || entry_idx > list.size() - 1 )
			continue;

		if ( key == "File" )
			list[ entry_idx ].url = Url( value, playlistPath );
		else if ( key == "Title" )
			list[ entry_idx ].name = value.replace( '\001', '\n' );
		else if ( key == "Length" )
			list[ entry_idx ].length = value.toInt();
		else if ( key == "QMPlay_sel" )
			list[ entry_idx ].selected = value.toInt();
		else if ( key == "QMPlay_queue" )
			list[ entry_idx ].queue = value.toInt();
		else if ( key == "QMPlay_GID" )
			list[ entry_idx ].GID = value.toInt();
		else if ( key == "QMPlay_parent" )
			list[ entry_idx ].parent = value.toInt();
	}

	return list;
}
bool PLS::_write( const QList< Entry > &list )
{
	Writer *writer = ioCtrl.rawPtr< Writer >();
	writer->write( QString( "[playlist]\r\nNumberOfEntries=" + QString::number( list.size() ) + "\r\n" ).toUtf8() );
	for ( int i = 0 ; i < list.size() ; i++ )
	{
		const Playlist::Entry &entry = list[ i ];
		QString idx = QString::number( i+1 );
		QString url = entry.url;
#ifdef Q_OS_WIN
		bool isFile = url.left( 5 ) == "file:";
#endif
		url.remove( "file://" );
#ifdef Q_OS_WIN
		if ( isFile )
			url.replace( "/", "\\" );
#endif
		if ( !url.isEmpty() )
			writer->write( QString( "File" + idx + "=" + url + "\r\n" ).toUtf8() );
		if ( !entry.name.isEmpty() )
			writer->write( QString( "Title" + idx + "=" + QString( entry.name ).replace( '\n', '\001' ) + "\r\n" ).toUtf8() );
		if ( entry.length >= 0 )
			writer->write( QString( "Length" + idx + "=" + QString::number( entry.length ) + "\r\n" ).toUtf8() );
		if ( entry.selected )
			writer->write( QString( "QMPlay_sel" + idx + "=" + QString::number( entry.selected ) + "\r\n" ).toUtf8() );
		if ( entry.queue )
			writer->write( QString( "QMPlay_queue" + idx + "=" + QString::number( entry.queue ) + "\r\n" ).toUtf8() );
		if ( entry.GID )
			writer->write( QString( "QMPlay_GID" + idx + "=" + QString::number( entry.GID ) + "\r\n" ).toUtf8() );
		if ( entry.parent )
			writer->write( QString( "QMPlay_parent" + idx + "=" + QString::number( entry.parent ) + "\r\n" ).toUtf8() );
	}
	return true;
}

/**/

void PLS::prepareList( QList< Entry > *list, int idx )
{
	if ( !list || idx <= list->size() - 1 )
		return;
	for ( int i = list->size() ; i <= idx ; i++ )
		*list += Entry();
}
