#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include <QString>
#include <QList>

class Reader;
class Writer;

class Playlist
{
public:
	class Entry
	{
	public:
		inline Entry() :
			selected( false ), length( -1 ), queue( 0 ), GID( 0 ), parent( 0 ) {}

		QString name, url;
		bool selected;
		int length, queue, GID, parent;
	};
	enum OpenMode { NoOpen, ReadOnly, WriteOnly };

	static QList< Entry > read( const QString &, QString *name = NULL );
	static bool write( const QList< Entry > &, const QString &, QString *name = NULL );
	static QString name( const QString & );
	static QStringList extensions();

	virtual QList< Entry > _read() = 0;
	virtual bool _write( const QList< Entry > & ) = 0;

	virtual ~Playlist();
private:
	static Playlist *create( const QString &, OpenMode, QString *name = NULL );
protected:
	Writer *writer;
	Reader *reader;
};

#endif
